/* Extracted from subprojects/miniaudio/miniaudio.h -- Vita SDK audio backend + threading.
 * All original miniaudio copyrights apply (see LICENSE in subprojects/miniaudio/).
 *
 * Pulled out as standalone reference for writing a new PSP driver. Vita and PSP
 * share similar SDK patterns (sceKernel*, SceUID, etc.).
 *
 * Sections extracted:
 *   1. MA_VITA platform define              (miniaudio.h:~4394)
 *   2. Vita threading backend types         (miniaudio.h:~5024-5086)
 *   3. Vita threading primitives            (miniaudio.h:~18725-18982)
 *   4. Vita backend config types            (miniaudio.h:~7734-7759)
 *   5. Vita backend implementation          (miniaudio.h:~49119-49457)
 *   6. MA_HAS_VITA backend enable           (miniaudio.h:~20756-20758)
 *   7. Vita backend registration            (miniaudio.h:~50494-50496, ~50528)
 */

/* ============================================================================
 * SECTION 1 — Platform autodetection (miniaudio.h:~4394)
 * ============================================================================
 * Triggered by compiler defining __vita__.
 *
 * Once MA_VITA is set, MA_THREADING_BACKEND_VITA is chosen (section 2) and
 * MA_HAS_VITA is defined if MA_SUPPORT_VITA && !MA_NO_VITA (section 6).
 */
#if defined(__vita__)
#  define MA_VITA
#endif

/* ============================================================================
 * SECTION 2 — Threading backend type selection (miniaudio.h:~5024-5086)
 * ============================================================================
 * MA_THREADING_BACKEND_VITA is selected when MA_VITA is defined.
 * All Vita threading primitives are just SceUID int handles.
 */
#ifndef MA_NO_THREADING
#  if defined(MA_VITA)
#    define MA_THREADING_BACKEND_VITA
#  endif
#else
#  define MA_THREADING_BACKEND_NONE
#endif

/* These typedefs mirror how miniaudio defines its portable thread types.
 * Under MA_THREADING_BACKEND_VITA they become simple int handles. */
#if defined(MA_THREADING_BACKEND_VITA)
typedef int ma_thread;
#else
typedef int ma_thread;          /* Fallback. */
#endif

#if defined(MA_THREADING_BACKEND_VITA)
typedef int ma_mutex;
#else
typedef ma_spinlock ma_mutex;   /* Fallback. */
#endif

#if defined(MA_THREADING_BACKEND_VITA)
typedef int ma_event;
#else
typedef ma_uint32 ma_event;     /* Fallback. */
#endif

#if defined(MA_THREADING_BACKEND_VITA)
typedef int ma_semaphore;
#else
typedef ma_uint32 ma_semaphore; /* Fallback. */
#endif


/* ============================================================================
 * SECTION 3 — Vita threading primitives (miniaudio.h:~18725-18982)
 * ============================================================================
 * Maps miniaudio's threading abstraction to the Vita SDK
 * (psp2/kernel/threadmgr/).
 *
 * KEY DIFFERENCES FROM PSP:
 *   - Vita's sceKernelCreateThread has 7 params vs PSP's 6
 *     (Vita adds cpuAffinityMask between attr and optParam)
 *   - Vita's sceKernelWaitThreadEnd has 3 params vs PSP's 2
 *     (Vita adds a stat output param)
 *   - Vita uses psp2/kernel/threadmgr/{thread,mutex,eventflag,semaphore}.h
 *     vs PSP's pspkernel.h / pspthreadman.h
 */
#if defined(MA_THREADING_BACKEND_VITA)
#include <psp2/kernel/threadmgr/thread.h>
#include <psp2/kernel/threadmgr/mutex.h>
#include <psp2/kernel/threadmgr/eventflag.h>
#include <psp2/kernel/threadmgr/semaphore.h>

/* --- Thread proxy (miniaudio wraps the entry point to capture return value) --- */
typedef struct {
    ma_thread_entry_proc entryProc;
    void* pData;
} ma_vita_thread_proxy_data;

static int ma_vita_thread_proxy(SceSize dataSize, void* pData)
{
    ma_vita_thread_proxy_data* pProxyData = (ma_vita_thread_proxy_data*)pData;
    (void)dataSize;
    return (int)pProxyData->entryProc(pProxyData->pData);
}

static ma_result ma_thread_create__vita(ma_thread* pThread, ma_thread_priority priority, size_t stackSize, ma_thread_entry_proc entryProc, void* pData)
{
    int thd;
    const char* pNameBase = "ma_thd_";
    size_t nameBaseLen = strlen(pNameBase);
    char name[64];
    static ma_uint32 counter = 0;

    if (pThread == NULL) return MA_INVALID_ARGS;

    ma_strcpy_s(name, sizeof(name), pNameBase);
    ma_itoa_s((int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed), name + nameBaseLen, sizeof(name) - nameBaseLen, 10);

    (void)priority;
    if (stackSize == 0) stackSize = 0x10000;

    /* NOTE: 7-param sceKernelCreateThread with cpuAffinityMask=0 */
    thd = sceKernelCreateThread(name, ma_vita_thread_proxy, 0x10000100, (SceSize)stackSize, 0, 0, NULL);
    if (thd < 0) return MA_ERROR;

    {   /* Proxy data must outlive sceKernelStartThread; copied into thread stack */
        ma_vita_thread_proxy_data data;
        data.entryProc = entryProc;
        data.pData     = pData;
        sceKernelStartThread(thd, sizeof(data), &data);
    }

    *pThread = thd;
    return MA_SUCCESS;
}

static void ma_thread_wait__vita(ma_thread* pThread)
{
    if (pThread == NULL) return;
    sceKernelWaitThreadEnd(*pThread, NULL, NULL);   /* 3-param (Vita) vs 2-param (PSP) */
    sceKernelDeleteThread(*pThread);
}

/* --- Mutex --- */
static ma_result ma_mutex_init__vita(ma_mutex* pMutex)
{
    int mtx;
    char name[64];
    static ma_uint32 counter = 0;

    if (pMutex == NULL) return MA_INVALID_ARGS;
    ma_snprintf(name, sizeof(name), "ma_mtx_%d", (int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed));

    mtx = sceKernelCreateMutex(name, 0, 0, NULL);
    if (mtx < 0) return MA_ERROR;

    *pMutex = mtx;
    return MA_SUCCESS;
}

static void ma_mutex_uninit__vita(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelDeleteMutex(*pMutex);
}

static void ma_mutex_lock__vita(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelLockMutex(*pMutex, 1, NULL);
}

static void ma_mutex_unlock__vita(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelUnlockMutex(*pMutex, 1);
}

/* --- Event (using event flags) --- */
static ma_result ma_event_init__vita(ma_event* pEvent)
{
    int e;
    char name[64];
    static ma_uint32 counter = 0;

    if (pEvent == NULL) return MA_INVALID_ARGS;
    ma_snprintf(name, sizeof(name), "ma_evt_%d", (int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed));

    e = sceKernelCreateEventFlag(name, SCE_EVENT_WAITMULTIPLE, 0, NULL);
    if (e < 0) return MA_ERROR;

    *pEvent = e;
    return MA_SUCCESS;
}

static void ma_event_uninit__vita(ma_event* pEvent)
{
    if (pEvent == NULL) return;
    sceKernelDeleteEventFlag(*pEvent);
}

static ma_result ma_event_wait__vita(ma_event* pEvent)
{
    unsigned int bits;
    if (pEvent == NULL) return MA_INVALID_ARGS;

    int result = sceKernelWaitEventFlag(*pEvent, 1, SCE_EVENT_WAITAND | SCE_EVENT_WAITCLEAR, &bits, NULL);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

static ma_result ma_event_signal__vita(ma_event* pEvent)
{
    if (pEvent == NULL) return MA_INVALID_ARGS;
    int result = sceKernelSetEventFlag(*pEvent, 1);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

/* --- Semaphore --- */
static ma_result ma_semaphore_init__vita(int initialValue, ma_semaphore* pSemaphore)
{
    int sem;
    char name[64];
    static ma_uint32 counter = 0;

    if (pSemaphore == NULL) return MA_INVALID_ARGS;
    ma_snprintf(name, sizeof(name), "ma_sem_%d", (int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed));

    sem = sceKernelCreateSema(name, 0, initialValue, 0x7FFFFFFF, NULL);
    if (sem < 0) return MA_ERROR;

    *pSemaphore = sem;
    return MA_SUCCESS;
}

static void ma_semaphore_uninit__vita(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return;
    sceKernelDeleteSema(*pSemaphore);
}

static ma_result ma_semaphore_wait__vita(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return MA_INVALID_ARGS;
    int result = sceKernelWaitSema(*pSemaphore, 1, NULL);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

static ma_result ma_semaphore_release__vita(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return MA_INVALID_ARGS;
    int result = sceKernelSignalSema(*pSemaphore, 1);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}
#endif  /* MA_THREADING_BACKEND_VITA */


/* ============================================================================
 * SECTION 4 — Vita backend config types (miniaudio.h:~7734-7759, ~8194, ~8336)
 * ============================================================================
 * These types are embedded in ma_device_config and ma_context_config.
 */
extern ma_device_backend_vtable* ma_device_backend_vita;
MA_API ma_device_backend_vtable* ma_vita_get_vtable(void);

typedef struct
{
    int _unused;
} ma_context_config_vita;

MA_API ma_context_config_vita ma_context_config_vita_init(void);

typedef enum
{
    MA_VITA_PORT_TYPE_BGM  = 0,
    MA_VITA_PORT_TYPE_MAIN = 1
} ma_vita_port_type;

typedef struct
{
    ma_vita_port_type portType;
} ma_device_config_vita;

MA_API ma_device_config_vita ma_device_config_vita_init(void);


/* ============================================================================
 * SECTION 5 — Vita audio backend (miniaudio.h:~49119-49457)
 * ============================================================================
 * Full miniaudio backend using <psp2/audioout.h>.
 *
 * KEY Vita SDK API calls:
 *   - sceAudioOutOpenPort(portType, period, sampleRate, mode)  → port handle
 *   - sceAudioOutOutput(port, buffer)                           → blocking write
 *   - sceAudioOutReleasePort(port)
 *   - Port types: SCE_AUDIO_OUT_PORT_TYPE_BGM, _MAIN
 *   - Modes: SCE_AUDIO_OUT_MODE_MONO, _STEREO
 *   - Constraints: SCE_AUDIO_MIN_LEN, SCE_AUDIO_MAX_LEN
 *
 * PATTERN: sceAudioOutOutput() is always blocking, so a dedicated audio
 * thread is used. Two sub-buffers, the main thread fills one via
 * ma_device_handle_backend_data_callback() while the audio thread
 * outputs the other.
 *
 * Compare with PSP's sceAudioOutput2OutputBlocking() which also blocks,
 * and sceAudioOutput2Reserve() which replaces sceAudioOutOpenPort().
 * PSP does NOT have sceAudioOutReleasePort() — it uses sceAudioOutput2Release().
 */
#if defined(MA_HAS_VITA)
#include <psp2/audioout.h>

typedef struct ma_context_state_vita
{
    int _unused;
} ma_context_state_vita;

typedef struct ma_device_state_vita
{
    int port;
    ma_bool32 isRunning;
    ma_thread thread;
    ma_uint32 subBufferIndex;
    ma_uint32 validSubBufferCount;
    void* pSubBuffers;
} ma_device_state_vita;


static ma_context_state_vita* ma_context_get_backend_state__vita(ma_context* pContext)
{
    return (ma_context_state_vita*)ma_context_get_backend_state(pContext);
}

static ma_device_state_vita* ma_device_get_backend_state__vita(ma_device* pDevice)
{
    return (ma_device_state_vita*)ma_device_get_backend_state(pDevice);
}


static void ma_backend_info__vita(ma_device_backend_info* pBackendInfo)
{
    pBackendInfo->pName = "PlayStation Vita";
}

static ma_result ma_context_init__vita(ma_context* pContext, const void* pContextBackendConfig, void** ppContextState)
{
    ma_context_state_vita* pContextStateVita =
        (ma_context_state_vita*)ma_calloc(sizeof(*pContextStateVita),
                                          ma_context_get_allocation_callbacks(pContext));
    if (pContextStateVita == NULL) return MA_OUT_OF_MEMORY;

    (void)pContextBackendConfig;
    (void)pContext;
    *ppContextState = pContextStateVita;
    return MA_SUCCESS;
}

static void ma_context_uninit__vita(ma_context* pContext)
{
    ma_context_state_vita* pContextStateVita = ma_context_get_backend_state__vita(pContext);
    ma_free(pContextStateVita, ma_context_get_allocation_callbacks(pContext));
}

static ma_result ma_context_enumerate_devices__vita(ma_context* pContext,
    ma_enum_devices_callback_proc callback, void* pCallbackUserData)
{
    ma_context_state_vita* pContextStateVita = ma_context_get_backend_state__vita(pContext);
    ma_device_info deviceInfo;
    ma_device_enumeration_result enumerationResult;

    (void)pContextStateVita;

    MA_ZERO_OBJECT(&deviceInfo);
    deviceInfo.isDefault = MA_TRUE;
    deviceInfo.id.custom.i = 0;
    ma_strncpy_s(deviceInfo.name, sizeof(deviceInfo.name), "Default Playback Device", (size_t)-1);

    /* Only s16 and mono/stereo is natively supported. */
    ma_device_info_add_native_data_format(&deviceInfo, ma_format_s16, 1, 2, 8000, 48000);

    enumerationResult = callback(ma_device_type_playback, &deviceInfo, pCallbackUserData);
    if (enumerationResult == MA_DEVICE_ENUMERATION_ABORT) return MA_SUCCESS;

    return MA_SUCCESS;
}

static void* ma_device_get_sub_buffer__vita(ma_device* pDevice, ma_uint32 subBufferIndex)
{
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);
    return ma_offset_ptr(pDeviceStateVita->pSubBuffers,
        pDevice->playback.internalPeriodSizeInFrames
        * ma_get_bytes_per_frame(pDevice->playback.internalFormat,
                                 pDevice->playback.internalChannels)
        * subBufferIndex);
}

static ma_thread_result MA_THREADCALL ma_device_audio_thread__vita(void* pUserData)
{
    ma_device* pDevice = (ma_device*)pUserData;
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);

    while (ma_atomic_load_explicit_32(&pDeviceStateVita->isRunning, ma_atomic_memory_order_relaxed)) {
        if (ma_atomic_load_explicit_32(&pDeviceStateVita->validSubBufferCount,
                                       ma_atomic_memory_order_acquire) > 0)
        {
            ma_uint32 subBufferIndex = ma_atomic_load_explicit_32(
                &pDeviceStateVita->subBufferIndex, ma_atomic_memory_order_relaxed);

            sceAudioOutOutput(pDeviceStateVita->port,
                              ma_device_get_sub_buffer__vita(pDevice, subBufferIndex));

            ma_atomic_store_explicit_32(&pDeviceStateVita->subBufferIndex,
                (subBufferIndex + 1) & 1, ma_atomic_memory_order_relaxed);
            ma_atomic_fetch_sub_explicit_32(&pDeviceStateVita->validSubBufferCount,
                1, ma_atomic_memory_order_release);
        } else {
            ma_sleep(10);   /* Don't peg the CPU. */
        }
    }

    return (ma_thread_result)0;
}

static ma_result ma_device_init__vita(ma_device* pDevice, const void* pDeviceBackendConfig,
    ma_device_descriptor* pDescriptorPlayback, ma_device_descriptor* pDescriptorCapture,
    void** ppDeviceState)
{
    ma_result result;
    ma_device_state_vita* pDeviceStateVita;
    ma_device_config_vita* pDeviceConfigVita = (ma_device_config_vita*)pDeviceBackendConfig;
    ma_context_state_vita* pContextStateVita =
        ma_context_get_backend_state__vita(ma_device_get_context(pDevice));
    ma_device_config_vita defaultConfig;
    ma_device_type deviceType = ma_device_get_type(pDevice);
    ma_log* pLog = ma_device_get_log(pDevice);
    ma_format format;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ma_uint32 periodSizeInFrames;
    SceAudioOutPortType portType;

    (void)pContextStateVita;
    (void)pDescriptorCapture;

    if (pDeviceConfigVita == NULL) {
        defaultConfig = ma_device_config_vita_init();
        pDeviceConfigVita = &defaultConfig;
    }

    if (deviceType != ma_device_type_playback)
        return MA_DEVICE_TYPE_NOT_SUPPORTED;

    pDeviceStateVita = (ma_device_state_vita*)ma_calloc(sizeof(*pDeviceStateVita),
        ma_device_get_allocation_callbacks(pDevice));
    if (pDeviceStateVita == NULL) return MA_OUT_OF_MEMORY;

    /* Port type: BGM (default) or Main */
    portType = SCE_AUDIO_OUT_PORT_TYPE_BGM;
    if (pDeviceConfigVita->portType == MA_VITA_PORT_TYPE_MAIN)
        portType = SCE_AUDIO_OUT_PORT_TYPE_MAIN;

    /* Format is always s16. */
    format = ma_format_s16;

    /* Channels: mono or stereo. Default to stereo. */
    channels = pDescriptorPlayback->channels;
    if (channels != 1 && channels != 2)
        channels = 1;

    /* Sample rate: BGM port only supports specific rates. */
    sampleRate = 0;
    if (pDescriptorPlayback->sampleRate != 0) {
        if (portType == SCE_AUDIO_OUT_PORT_TYPE_BGM) {
            ma_uint32 bgmSampleRates[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
            for (ma_uint32 i = 0; i < ma_countof(bgmSampleRates); i++) {
                if (pDescriptorPlayback->sampleRate == bgmSampleRates[i]) {
                    sampleRate = bgmSampleRates[i];
                }
            }
        }
    }
    if (sampleRate == 0)
        sampleRate = 48000;

    /* Period size: must be multiple of 64 and clamped to [SCE_AUDIO_MIN_LEN, SCE_AUDIO_MAX_LEN]. */
    periodSizeInFrames = ma_calculate_buffer_size_in_frames_from_descriptor(pDescriptorPlayback, sampleRate);
    periodSizeInFrames = ma_clamp((periodSizeInFrames + 63) & ~63, SCE_AUDIO_MIN_LEN, SCE_AUDIO_MAX_LEN);

    /* Two sub-buffers (double-buffering). */
    pDeviceStateVita->pSubBuffers = ma_malloc(
        periodSizeInFrames * ma_get_bytes_per_frame(format, channels) * 2,
        ma_device_get_allocation_callbacks(pDevice));
    if (pDeviceStateVita->pSubBuffers == NULL) return MA_OUT_OF_MEMORY;

    pDeviceStateVita->port = sceAudioOutOpenPort(
        portType, (int)periodSizeInFrames, (int)sampleRate,
        (channels == 1) ? SCE_AUDIO_OUT_MODE_MONO : SCE_AUDIO_OUT_MODE_STEREO);
    if (pDeviceStateVita->port < 0) {
        ma_free(pDeviceStateVita->pSubBuffers, ma_device_get_allocation_callbacks(pDevice));
        ma_log_postf(pLog, MA_LOG_LEVEL_ERROR, "[Vita] Failed to open port.");
        return MA_ERROR;
    }

    ma_atomic_store_explicit_32(&pDeviceStateVita->isRunning, 1, ma_atomic_memory_order_relaxed);

    /* Create audio thread (sceAudioOutOutput is blocking). */
    result = ma_thread_create(&pDeviceStateVita->thread, ma_thread_priority_default, 0,
                              ma_device_audio_thread__vita, pDevice,
                              ma_device_get_allocation_callbacks(pDevice));
    if (result != MA_SUCCESS) {
        sceAudioOutReleasePort(pDeviceStateVita->port);
        ma_free(pDeviceStateVita->pSubBuffers, ma_device_get_allocation_callbacks(pDevice));
        ma_log_postf(pLog, MA_LOG_LEVEL_ERROR, "[Vita] Failed to create audio thread.");
        return MA_ERROR;
    }

    /* Update descriptor with actual internal settings. */
    pDescriptorPlayback->format             = format;
    pDescriptorPlayback->channels           = channels;
    pDescriptorPlayback->sampleRate         = sampleRate;
    pDescriptorPlayback->periodSizeInFrames = periodSizeInFrames;
    pDescriptorPlayback->periodCount        = 2;

    *ppDeviceState = pDeviceStateVita;
    return MA_SUCCESS;
}

static void ma_device_uninit__vita(ma_device* pDevice)
{
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);

    ma_atomic_store_explicit_32(&pDeviceStateVita->isRunning, 0, ma_atomic_memory_order_relaxed);
    ma_thread_wait(&pDeviceStateVita->thread);

    sceAudioOutReleasePort(pDeviceStateVita->port);
    ma_free(pDeviceStateVita->pSubBuffers, ma_device_get_allocation_callbacks(pDevice));
    ma_free(pDeviceStateVita, ma_device_get_allocation_callbacks(pDevice));
}

static ma_result ma_device_start__vita(ma_device* pDevice)
{
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);

    ma_atomic_store_explicit_32(&pDeviceStateVita->subBufferIndex,      0, ma_atomic_memory_order_relaxed);
    ma_atomic_store_explicit_32(&pDeviceStateVita->validSubBufferCount, 0, ma_atomic_memory_order_relaxed);

    return MA_SUCCESS;
}

static ma_result ma_device_stop__vita(ma_device* pDevice)
{
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);

    while (ma_atomic_load_explicit_32(&pDeviceStateVita->validSubBufferCount,
                                      ma_atomic_memory_order_relaxed) > 0)
    {
        ma_sleep(1);
    }

    return MA_SUCCESS;
}

static ma_result ma_device_step__vita(ma_device* pDevice, ma_blocking_mode blockingMode)
{
    ma_device_state_vita* pDeviceStateVita = ma_device_get_backend_state__vita(pDevice);

    for (;;) {
        if (!ma_device_is_started(pDevice))
            return MA_DEVICE_NOT_STARTED;

        if (ma_atomic_load_explicit_32(&pDeviceStateVita->validSubBufferCount,
                                       ma_atomic_memory_order_acquire) < 2)
        {
            ma_uint32 subBufferIndex = ma_atomic_load_explicit_32(
                &pDeviceStateVita->subBufferIndex, ma_atomic_memory_order_relaxed);
            ma_device_handle_backend_data_callback(pDevice,
                ma_device_get_sub_buffer__vita(pDevice, subBufferIndex), NULL,
                pDevice->playback.internalPeriodSizeInFrames);
            ma_atomic_fetch_add_explicit_32(&pDeviceStateVita->validSubBufferCount,
                1, ma_atomic_memory_order_release);
            return MA_SUCCESS;
        }

        if (blockingMode == MA_BLOCKING_MODE_NON_BLOCKING)
            return MA_SUCCESS;

        ma_sleep(1);
    }
}

static void ma_device_wakeup__vita(ma_device* pDevice)
{
    (void)pDevice;
}

static ma_device_backend_vtable ma_gDeviceBackendVTable_Vita =
{
    ma_backend_info__vita,
    ma_context_init__vita,
    ma_context_uninit__vita,
    ma_context_enumerate_devices__vita,
    ma_device_init__vita,
    ma_device_uninit__vita,
    ma_device_start__vita,
    ma_device_stop__vita,
    ma_device_step__vita,
    ma_device_wakeup__vita
};

ma_device_backend_vtable* ma_device_backend_vita = &ma_gDeviceBackendVTable_Vita;
#else
ma_device_backend_vtable* ma_device_backend_vita = NULL;
#endif  /* MA_HAS_VITA */


/* ============================================================================
 * SECTION 6 — MA_HAS_VITA backend enable logic (miniaudio.h:~20756-20758)
 * ============================================================================
 * The HAS define controls whether the backend implementation is compiled.
 * It requires MA_SUPPORT_VITA to be set by the user/project, and MA_NO_VITA
 * to not be set. MA_ENABLE_ONLY_SPECIFIC_BACKENDS can restrict further.
 *
 * Usage: -DMA_SUPPORT_VITA or #define MA_SUPPORT_VITA in your build.
 */
#if defined(MA_SUPPORT_VITA) && !defined(MA_NO_VITA) && \
    (!defined(MA_ENABLE_ONLY_SPECIFIC_BACKENDS) || defined(MA_ENABLE_VITA))
#  define MA_HAS_VITA
#endif


/* ============================================================================
 * SECTION 7 — Backend registration (miniaudio.h:~50494-50496, ~50528)
 * ============================================================================
 * During ma_context_init(), the Vita vtable is looked up by pointer equality
 * to find the Vita-specific config inside ma_conext_config:
 *
 *   if (pVTable == ma_device_backend_vita) {
 *       return &pConfig->vita;
 *   }
 *
 * In ma_get_stock_device_backends(), Vita is included after xaudio:
 *
 *   pBackends[count++] = ma_device_backend_config_init(ma_device_backend_vita, NULL);
 */


/* ============================================================================
 * SDK API COMPARISON: Vita vs PSP
 * ============================================================================
 *
 * Concept              Vita SDK                          PSP SDK (Audio2)
 * -------------------  --------------------------------  --------------------------------
 * Open audio port      sceAudioOutOpenPort(...)          sceAudioOutput2Reserve(period)
 * Output (blocking)    sceAudioOutOutput(port, buf)      sceAudioOutput2OutputBlocking(vol, buf)
 * Close port           sceAudioOutReleasePort(port)      sceAudioOutput2Release()
 * BGM vs Main port     SCE_AUDIO_OUT_PORT_TYPE_BGM/_MAIN N/A (single output)
 * Sample rate list     {8000-48000} specific set         Single rate at reservation time
 * Channel modes        MONO / STEREO enum                Implicit from channel count
 * Period alignment     64-byte multiple                  No strict alignment (PSP needs 64b buf)
 * Period constraints   [SCE_AUDIO_MIN_LEN, MAX_LEN]      Lib-specific limits
 * Thread create        7 params (+cpuAffinityMask)       6 params
 * Thread wait end      3 params (+stat)                  2 params (no stat)
 * Mutex                sceKernelCreateMutex(...)          sceKernelCreateMutex(...)
 * Event flag           sceKernelCreateEventFlag(...)      sceKernelCreateEventFlag(...)
 * Semaphore            sceKernelCreateSema(...)           sceKernelCreateSema(...)
 *
 * SHARED PATTERNS (same API on both):
 *   sceKernelStartThread(id, argSize, argp)
 *   sceKernelDeleteThread(id)
 *   sceKernelDeleteMutex(id)
 *   sceKernelLockMutex(id, count, timeout)
 *   sceKernelUnlockMutex(id, count)
 *   sceKernelWaitSema(id, count, timeout)
 *   sceKernelSignalSema(id, count)
 *   sceKernelSetEventFlag(id, bits)
 *   sceKernelWaitEventFlag(id, bits, op, outBits, timeout)
 *   sceKernelDeleteEventFlag(id)
 *   sceKernelDeleteSema(id)
 *   SceUID / SceKernel* types
 *   __attribute__((aligned(N))) for buffer alignment
 */
