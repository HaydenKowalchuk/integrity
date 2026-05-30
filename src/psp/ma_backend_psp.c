/*
 * miniaudio custom backend for PlayStation Portable (PSP)
 *
 * Implements ma_backend_callbacks using the PSP SDK's sceAudio functions.
 * The PSP audio hardware provides:
 *   - Fixed 16-bit signed PCM (s16) format
 *   - 44100 Hz native sample rate (or SRC for other rates)
 *   - Up to 8 hardware channels (stereo or mono)
 *   - Double-buffered blocking output
 */

#include <miniaudio.h>
#include <pspaudio.h>
#include <string.h>

#define MA_PSP_NUM_BUFFERS 2
#define MA_PSP_DEFAULT_PERIOD_SIZE 512
#define MA_PSP_DEFAULT_CHANNELS 2
#define MA_PSP_DEFAULT_SAMPLE_RATE 44100

typedef struct {
  ma_context context;
  int channel;
} ma_context_psp;

typedef struct {
  ma_device device;
  int ch;
  ma_uint32 periodSizeInFrames;
  ma_uint32 channels;
  ma_uint32 sampleRate;
  ma_uint8** ppBuffers;
  ma_uint32 currentBuffer;
  ma_bool32 isStarted;
} ma_device_psp;

static ma_result ma_context_on_uninit__psp(ma_context* pContext) {
  ma_context_psp* pContextPSP = (ma_context_psp*)pContext;

  if (pContextPSP->channel >= 0) {
    sceAudioChRelease(pContextPSP->channel);
    pContextPSP->channel = -1;
  }

  return MA_SUCCESS;
}

static ma_result ma_context_on_enumerate_devices__psp(ma_context* pContext, ma_enum_devices_callback_proc callback, void* pUserData) {
  ma_device_info deviceInfo;
  (void)pContext;

  memset(&deviceInfo, 0, sizeof(deviceInfo));
  deviceInfo.isDefault = MA_TRUE;
  strncpy(deviceInfo.name, "PSP Audio Device", sizeof(deviceInfo.name) - 1);
  deviceInfo.id.custom.i = 0;

  if (!callback(pContext, ma_device_type_playback, &deviceInfo, pUserData)) {
    return MA_CANCELLED;
  }

  return MA_SUCCESS;
}

static ma_result ma_context_on_get_device_info__psp(ma_context* pContext, ma_device_type deviceType, const ma_device_id* pDeviceID, ma_device_info* pDeviceInfo) {
  (void)pContext;
  (void)pDeviceID;

  if (deviceType != ma_device_type_playback) {
    return MA_NO_DEVICE;
  }

  memset(pDeviceInfo, 0, sizeof(*pDeviceInfo));
  pDeviceInfo->isDefault = MA_TRUE;
  strncpy(pDeviceInfo->name, "PSP Audio Device", sizeof(pDeviceInfo->name) - 1);
  pDeviceInfo->id.custom.i = 0;

  pDeviceInfo->nativeDataFormatCount = 1;
  pDeviceInfo->nativeDataFormats[0].format = ma_format_s16;
  pDeviceInfo->nativeDataFormats[0].channels = 2;
  pDeviceInfo->nativeDataFormats[0].sampleRate = MA_PSP_DEFAULT_SAMPLE_RATE;
  pDeviceInfo->nativeDataFormats[0].flags = 0;

  return MA_SUCCESS;
}

static ma_result ma_device_on_init__psp(ma_device* pDevice, const ma_device_config* pConfig, ma_device_descriptor* pDescriptorPlayback, ma_device_descriptor* pDescriptorCapture) {
  ma_device_psp* pDevicePSP = (ma_device_psp*)pDevice;
  ma_context_psp* pContextPSP = (ma_context_psp*)pDevice->pContext;
  int format;
  int sampleCount;
  int ch;
  (void)pDescriptorCapture;

  if (pConfig->deviceType == ma_device_type_capture || pConfig->deviceType == ma_device_type_duplex) {
    return MA_NO_DEVICE;
  }

  pDevicePSP->sampleRate = (pConfig->sampleRate != 0) ? pConfig->sampleRate : MA_PSP_DEFAULT_SAMPLE_RATE;
  pDevicePSP->channels = (pConfig->playback.channels != 0) ? pConfig->playback.channels : MA_PSP_DEFAULT_CHANNELS;

  if (pConfig->periodSizeInFrames != 0) {
    pDevicePSP->periodSizeInFrames = pConfig->periodSizeInFrames;
  } else if (pConfig->periodSizeInMilliseconds != 0) {
    pDevicePSP->periodSizeInFrames = (ma_uint32)(pConfig->periodSizeInMilliseconds * pDevicePSP->sampleRate / 1000);
  } else {
    pDevicePSP->periodSizeInFrames = MA_PSP_DEFAULT_PERIOD_SIZE;
  }

  sampleCount = (int)PSP_AUDIO_SAMPLE_ALIGN(pDevicePSP->periodSizeInFrames);
  if (sampleCount < PSP_AUDIO_SAMPLE_MIN || sampleCount > PSP_AUDIO_SAMPLE_MAX) {
    sampleCount = MA_PSP_DEFAULT_PERIOD_SIZE;
  }

  format = (pDevicePSP->channels == 1) ? PSP_AUDIO_FORMAT_MONO : PSP_AUDIO_FORMAT_STEREO;

  ch = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, sampleCount, format);
  if (ch < 0) {
    return MA_FAILED_TO_OPEN_BACKEND_DEVICE;
  }

  pDevicePSP->ch = ch;

  pDevicePSP->ppBuffers = (ma_uint8**)ma_malloc(sizeof(ma_uint8*) * MA_PSP_NUM_BUFFERS, &pDevice->pContext->allocationCallbacks);
  if (pDevicePSP->ppBuffers == NULL) {
    sceAudioChRelease(ch);
    return MA_OUT_OF_MEMORY;
  }

  for (ma_uint32 i = 0; i < MA_PSP_NUM_BUFFERS; i++) {
    pDevicePSP->ppBuffers[i] = (ma_uint8*)ma_malloc(sampleCount * pDevicePSP->channels * sizeof(ma_int16), &pDevice->pContext->allocationCallbacks);
    if (pDevicePSP->ppBuffers[i] == NULL) {
      for (ma_uint32 j = 0; j < i; j++) {
        ma_free(pDevicePSP->ppBuffers[j], &pDevice->pContext->allocationCallbacks);
      }
      ma_free(pDevicePSP->ppBuffers, &pDevice->pContext->allocationCallbacks);
      sceAudioChRelease(ch);
      return MA_OUT_OF_MEMORY;
    }
  }

  pDevicePSP->currentBuffer = 0;
  pDevicePSP->isStarted = MA_FALSE;

  if (pDescriptorPlayback != NULL) {
    pDescriptorPlayback->channels = pDevicePSP->channels;
    pDescriptorPlayback->sampleRate = pDevicePSP->sampleRate;
    pDescriptorPlayback->format = ma_format_s16;
    memset(pDescriptorPlayback->channelMap, 0, sizeof(pDescriptorPlayback->channelMap));
    pDescriptorPlayback->periodSizeInFrames = pDevicePSP->periodSizeInFrames;
    pDescriptorPlayback->periodCount = MA_PSP_NUM_BUFFERS;
  }

  return MA_SUCCESS;
}

static ma_result ma_device_on_uninit__psp(ma_device* pDevice) {
  ma_device_psp* pDevicePSP = (ma_device_psp*)pDevice;

  pDevicePSP->isStarted = MA_FALSE;

  if (pDevicePSP->ch >= 0) {
    sceAudioChRelease(pDevicePSP->ch);
    pDevicePSP->ch = -1;
  }

  if (pDevicePSP->ppBuffers != NULL) {
    for (ma_uint32 i = 0; i < MA_PSP_NUM_BUFFERS; i++) {
      if (pDevicePSP->ppBuffers[i] != NULL) {
        ma_free(pDevicePSP->ppBuffers[i], &pDevice->pContext->allocationCallbacks);
      }
    }
    ma_free(pDevicePSP->ppBuffers, &pDevice->pContext->allocationCallbacks);
    pDevicePSP->ppBuffers = NULL;
  }

  return MA_SUCCESS;
}

static ma_result ma_device_on_start__psp(ma_device* pDevice) {
  ma_device_psp* pDevicePSP = (ma_device_psp*)pDevice;
  pDevicePSP->isStarted = MA_TRUE;
  return MA_SUCCESS;
}

static ma_result ma_device_on_stop__psp(ma_device* pDevice) {
  ma_device_psp* pDevicePSP = (ma_device_psp*)pDevice;
  pDevicePSP->isStarted = MA_FALSE;
  return MA_SUCCESS;
}

static ma_result ma_device_on_data_loop__psp(ma_device* pDevice) {
  ma_device_psp* pDevicePSP = (ma_device_psp*)pDevice;
  ma_uint32 bufferSizeInFrames = pDevicePSP->periodSizeInFrames;

  while (pDevicePSP->isStarted) {
    ma_uint8* pBuffer = pDevicePSP->ppBuffers[pDevicePSP->currentBuffer];
    ma_result result;

    result = ma_device_handle_backend_data_callback(pDevice, pBuffer, NULL, bufferSizeInFrames);
    if (result != MA_SUCCESS) {
      break;
    }

    if (pDevicePSP->channels == 2) {
      sceAudioOutputPannedBlocking(pDevicePSP->ch, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, pBuffer);
    } else {
      sceAudioOutputBlocking(pDevicePSP->ch, PSP_AUDIO_VOLUME_MAX, pBuffer);
    }

    pDevicePSP->currentBuffer = (pDevicePSP->currentBuffer + 1) % MA_PSP_NUM_BUFFERS;
  }

  return MA_SUCCESS;
}

static ma_result ma_device_on_data_loop_wakeup__psp(ma_device* pDevice) {
  (void)pDevice;
  return MA_SUCCESS;
}

ma_result ma_context_on_init__psp(ma_context* pContext, const ma_context_config* pConfig, ma_backend_callbacks* pCallbacks) {
  ma_context_psp* pContextPSP = (ma_context_psp*)pContext;
  (void)pConfig;

  pContextPSP->channel = -1;

  pCallbacks->onContextUninit = ma_context_on_uninit__psp;
  pCallbacks->onContextEnumerateDevices = ma_context_on_enumerate_devices__psp;
  pCallbacks->onContextGetDeviceInfo = ma_context_on_get_device_info__psp;
  pCallbacks->onDeviceInit = ma_device_on_init__psp;
  pCallbacks->onDeviceUninit = ma_device_on_uninit__psp;
  pCallbacks->onDeviceStart = ma_device_on_start__psp;
  pCallbacks->onDeviceStop = ma_device_on_stop__psp;
  pCallbacks->onDeviceRead = NULL;
  pCallbacks->onDeviceWrite = NULL;
  pCallbacks->onDeviceDataLoop = ma_device_on_data_loop__psp;
  pCallbacks->onDeviceDataLoopWakeup = ma_device_on_data_loop_wakeup__psp;
  pCallbacks->onDeviceGetInfo = NULL;

  return MA_SUCCESS;
}
