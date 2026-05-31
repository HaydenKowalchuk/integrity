/* PSP miniaudio backend — adapted from sys_vita_sound.c
 *
 * Uses PSP Audio2 API (sceAudioOutput2*) for audio output and
 * PSP sceKernel* threading primitives.
 *
 * Key differences from the Vita backend:
 *   - sceKernelCreateThread: 6 params (no cpuAffinityMask)
 *   - sceKernelWaitThreadEnd:  2 params (no stat output)
 *   - Audio2 API (no port type, no format selection, always stereo 16-bit)
 *   - Buffer alignment: 64-byte boundary required
 *   - Period size range: 17–4111 frames
 *
 * Sections follow the same structure as the Vita reference:
 *   1. MA_PSP platform define
 *   2. Threading backend types
 *   3. Threading primitives
 *   4. Backend config types
 *   5. Audio backend implementation
 *   6. MA_HAS_PSP enable logic
 *   7. Backend registration
 */

/* ============================================================================
 * SECTION 1 — Platform autodetection
 * ============================================================================
 * Triggered by the compiler defining __psp__.
 */
#if defined(__psp__) || defined(PSP)
#  define MA_PSP
#endif


/* ============================================================================
 * SECTION 2 — Threading backend type selection
 * ============================================================================
 * MA_THREADING_BACKEND_PSP is selected when MA_PSP is defined.
 */
#ifndef MA_NO_THREADING
#  if defined(MA_PSP)
#    define MA_THREADING_BACKEND_PSP
#  endif
#else
#  define MA_THREADING_BACKEND_NONE
#endif

#if defined(MA_THREADING_BACKEND_PSP)
typedef int ma_thread;
#else
typedef int ma_thread;
#endif

#if defined(MA_THREADING_BACKEND_PSP)
typedef int ma_mutex;
#else
typedef ma_spinlock ma_mutex;
#endif

#if defined(MA_THREADING_BACKEND_PSP)
typedef int ma_event;
#else
typedef ma_uint32 ma_event;
#endif

#if defined(MA_THREADING_BACKEND_PSP)
typedef int ma_semaphore;
#else
typedef ma_uint32 ma_semaphore;
#endif


/* ============================================================================
 * SECTION 3 — PSP threading primitives
 * ============================================================================
 * Maps miniaudio's threading abstraction to the PSP SDK.
 *
 * PSP uses <pspthreadman.h> for all thread/sema/event-flag primitives.
 *
 * NOTE: sceKernelCreateMutex is NOT exported by PSP's ThreadManForUser;
 * we implement ma_mutex as a binary semaphore instead.
 */

#if defined(MA_THREADING_BACKEND_PSP)
#include <pspthreadman.h>

/* --- Thread proxy --- */
typedef struct {
    ma_thread_entry_proc entryProc;
    void* pData;
} ma_psp_thread_proxy_data;

static int ma_psp_thread_proxy(SceSize dataSize, void* pData)
{
    ma_psp_thread_proxy_data* pProxyData = (ma_psp_thread_proxy_data*)pData;
    (void)dataSize;
    return (int)pProxyData->entryProc(pProxyData->pData);
}

static ma_result ma_thread_create__psp(ma_thread* pThread, ma_thread_priority priority, size_t stackSize, ma_thread_entry_proc entryProc, void* pData)
{
    SceUID thd;
    const char* pNameBase = "ma_thd_";
    size_t nameBaseLen = strlen(pNameBase);
    char name[64];
    static ma_uint32 counter = 0;

    if (pThread == NULL) return MA_INVALID_ARGS;

    ma_strcpy_s(name, sizeof(name), pNameBase);
    ma_itoa_s((int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed), name + nameBaseLen, sizeof(name) - nameBaseLen, 10);

    (void)priority;
    if (stackSize == 0) stackSize = 0x10000;

    /* 6-param sceKernelCreateThread (PSP) vs 7-param (Vita) */
    thd = sceKernelCreateThread(name, ma_psp_thread_proxy, 0x10000100, (int)stackSize, 0, NULL);
    if (thd < 0) return MA_ERROR;

    {
        ma_psp_thread_proxy_data data;
        data.entryProc = entryProc;
        data.pData     = pData;
        sceKernelStartThread(thd, sizeof(data), &data);
    }

    *pThread = thd;
    return MA_SUCCESS;
}

static void ma_thread_wait__psp(ma_thread* pThread)
{
    if (pThread == NULL) return;
    sceKernelWaitThreadEnd(*pThread, NULL);  /* 2-param (PSP) vs 3-param (Vita) */
    sceKernelDeleteThread(*pThread);
}

/* --- Mutex (binary semaphore) --- */
static ma_result ma_mutex_init__psp(ma_mutex* pMutex)
{
    int sem;
    char name[64];
    static ma_uint32 counter = 0;

    if (pMutex == NULL) return MA_INVALID_ARGS;
    ma_snprintf(name, sizeof(name), "ma_mtx_%d", (int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed));

    sem = sceKernelCreateSema(name, 0, 1, 1, NULL);
    if (sem < 0) return MA_ERROR;

    *pMutex = sem;
    return MA_SUCCESS;
}

static void ma_mutex_uninit__psp(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelDeleteSema(*pMutex);
}

static void ma_mutex_lock__psp(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelWaitSema(*pMutex, 1, NULL);
}

static void ma_mutex_unlock__psp(ma_mutex* pMutex)
{
    if (pMutex == NULL) return;
    sceKernelSignalSema(*pMutex, 1);
}

/* --- Event (using event flags) --- */
static ma_result ma_event_init__psp(ma_event* pEvent)
{
    int e;
    char name[64];
    static ma_uint32 counter = 0;

    if (pEvent == NULL) return MA_INVALID_ARGS;
    ma_snprintf(name, sizeof(name), "ma_evt_%d", (int)ma_atomic_fetch_add_explicit_32(&counter, 1, ma_atomic_memory_order_relaxed));

    e = sceKernelCreateEventFlag(name, PSP_EVENT_WAITMULTIPLE, 0, NULL);
    if (e < 0) return MA_ERROR;

    *pEvent = e;
    return MA_SUCCESS;
}

static void ma_event_uninit__psp(ma_event* pEvent)
{
    if (pEvent == NULL) return;
    sceKernelDeleteEventFlag(*pEvent);
}

static ma_result ma_event_wait__psp(ma_event* pEvent)
{
    unsigned int bits;
    if (pEvent == NULL) return MA_INVALID_ARGS;

    int result = sceKernelWaitEventFlag(*pEvent, 1, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &bits, NULL);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

static ma_result ma_event_signal__psp(ma_event* pEvent)
{
    if (pEvent == NULL) return MA_INVALID_ARGS;
    int result = sceKernelSetEventFlag(*pEvent, 1);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

/* --- Semaphore --- */
static ma_result ma_semaphore_init__psp(int initialValue, ma_semaphore* pSemaphore)
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

static void ma_semaphore_uninit__psp(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return;
    sceKernelDeleteSema(*pSemaphore);
}

static ma_result ma_semaphore_wait__psp(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return MA_INVALID_ARGS;
    int result = sceKernelWaitSema(*pSemaphore, 1, NULL);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}

static ma_result ma_semaphore_release__psp(ma_semaphore* pSemaphore)
{
    if (pSemaphore == NULL) return MA_INVALID_ARGS;
    int result = sceKernelSignalSema(*pSemaphore, 1);
    if (result < 0) return MA_ERROR;
    return MA_SUCCESS;
}
#endif  /* MA_THREADING_BACKEND_PSP */


/* ============================================================================
 * SECTION 4 — PSP backend enable logic
 * ============================================================================
 * Define MA_SUPPORT_PSP in your build to compile this backend.
 */
#if defined(MA_SUPPORT_PSP) && !defined(MA_NO_PSP) && \
    (!defined(MA_ENABLE_ONLY_SPECIFIC_BACKENDS) || defined(MA_ENABLE_PSP))
#  define MA_HAS_PSP
#endif


/* ============================================================================
 * SECTION 5 — PSP backend config types
 * ============================================================================
 * These types are embedded in ma_device_config and ma_context_config.
 */
extern ma_device_backend_vtable* ma_device_backend_psp;
MA_API ma_device_backend_vtable* ma_psp_get_vtable(void);

typedef struct
{
    int _unused;
} ma_context_config_psp;

MA_API ma_context_config_psp ma_context_config_psp_init(void);

typedef struct
{
    int _unused;
} ma_device_config_psp;

MA_API ma_device_config_psp ma_device_config_psp_init(void);


/* ============================================================================
 * SECTION 6 — PSP audio backend
 * ============================================================================
 * Uses the Audio2 API (<pspaudio.h>):
 *   - sceAudioOutput2Reserve(samplecount)       — init output
 *   - sceAudioOutput2OutputBlocking(vol, buf)    — blocking write
 *   - sceAudioOutput2Release()                   — cleanup
 *
 * Audio2 constraints:
 *   - Always stereo 16-bit PCM
 *   - Period size: 17–4111 frames
 *   - Buffer must be 64-byte aligned
 *   - No sample rate selection (hardware default, usually 44100)
 *
 * PATTERN: sceAudioOutput2OutputBlocking() blocks, so a dedicated audio
 * thread is used. Two sub-buffers (double-buffering): main thread fills one
 * via ma_device_handle_backend_data_callback() while audio thread outputs
 * the other.
 */

#if defined(MA_HAS_PSP)
#include <pspaudio.h>

/* PSP Audio2 period size constraints */
#define PSP_AUDIO2_MIN_PERIOD    17
#define PSP_AUDIO2_MAX_PERIOD    4111

/* Alignment for PSP audio buffers */
#define PSP_AUDIO_BUF_ALIGNMENT  64

typedef struct ma_context_state_psp
{
    int _unused;
} ma_context_state_psp;

typedef struct ma_device_state_psp
{
    ma_bool32 isRunning;
    ma_thread thread;
    ma_uint32 subBufferIndex;
    ma_uint32 validSubBufferCount;
    ma_uint32 subBufferSizeInBytes;
    void* pSubBuffers;
    void* pSubBufferAligned[2];  /* 64-byte aligned pointers within pSubBuffers */
} ma_device_state_psp;


static ma_context_state_psp* ma_context_get_backend_state__psp(ma_context* pContext)
{
    return (ma_context_state_psp*)ma_context_get_backend_state(pContext);
}

static ma_device_state_psp* ma_device_get_backend_state__psp(ma_device* pDevice)
{
    return (ma_device_state_psp*)ma_device_get_backend_state(pDevice);
}


static void ma_backend_info__psp(ma_device_backend_info* pBackendInfo)
{
    pBackendInfo->pName = "PlayStation Portable";
}

static ma_result ma_context_init__psp(ma_context* pContext, const void* pContextBackendConfig, void** ppContextState)
{
    ma_context_state_psp* pContextStatePsp =
        (ma_context_state_psp*)ma_calloc(sizeof(*pContextStatePsp),
                                          ma_context_get_allocation_callbacks(pContext));
    if (pContextStatePsp == NULL) return MA_OUT_OF_MEMORY;

    (void)pContextBackendConfig;
    (void)pContext;
    *ppContextState = pContextStatePsp;
    return MA_SUCCESS;
}

static void ma_context_uninit__psp(ma_context* pContext)
{
    ma_context_state_psp* pContextStatePsp = ma_context_get_backend_state__psp(pContext);
    ma_free(pContextStatePsp, ma_context_get_allocation_callbacks(pContext));
}

static ma_result ma_context_enumerate_devices__psp(ma_context* pContext,
    ma_enum_devices_callback_proc callback, void* pCallbackUserData)
{
    ma_context_state_psp* pContextStatePsp = ma_context_get_backend_state__psp(pContext);
    ma_device_info deviceInfo;
    ma_device_enumeration_result enumerationResult;

    (void)pContextStatePsp;

    MA_ZERO_OBJECT(&deviceInfo);
    deviceInfo.isDefault = MA_TRUE;
    deviceInfo.id.custom.i = 0;
    ma_strncpy_s(deviceInfo.name, sizeof(deviceInfo.name), "Default Playback Device", (size_t)-1);

    /* PSP Audio2 only supports stereo 16-bit at the system sample rate (usually 44100). */
    ma_device_info_add_native_data_format(&deviceInfo, ma_format_s16, 2, 2, 44100, 44100);

    enumerationResult = callback(ma_device_type_playback, &deviceInfo, pCallbackUserData);
    if (enumerationResult == MA_DEVICE_ENUMERATION_ABORT) return MA_SUCCESS;

    return MA_SUCCESS;
}

/* Round up to nearest multiple of alignment. */
static ma_uint32 psp_align_up(ma_uint32 val, ma_uint32 alignment)
{
    return (val + alignment - 1) & ~(alignment - 1);
}

static void* psp_align_ptr(void* ptr, ma_uint32 alignment)
{
    ma_uintptr addr = (ma_uintptr)ptr;
    ma_uintptr aligned = (addr + alignment - 1) & ~(ma_uintptr)(alignment - 1);
    return (void*)aligned;
}

static void* ma_device_get_sub_buffer__psp(ma_device* pDevice, ma_uint32 subBufferIndex)
{
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);
    return ma_offset_ptr(pDeviceStatePsp->pSubBufferAligned[0],
        pDeviceStatePsp->subBufferSizeInBytes * subBufferIndex);
}

static ma_thread_result MA_THREADCALL ma_device_audio_thread__psp(void* pUserData)
{
    ma_device* pDevice = (ma_device*)pUserData;
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);

    while (ma_atomic_load_explicit_32(&pDeviceStatePsp->isRunning, ma_atomic_memory_order_relaxed)) {
        if (ma_atomic_load_explicit_32(&pDeviceStatePsp->validSubBufferCount,
                                       ma_atomic_memory_order_acquire) > 0)
        {
            ma_uint32 subBufferIndex = ma_atomic_load_explicit_32(
                &pDeviceStatePsp->subBufferIndex, ma_atomic_memory_order_relaxed);

            sceAudioOutput2OutputBlocking(PSP_AUDIO_VOLUME_MAX,
                ma_device_get_sub_buffer__psp(pDevice, subBufferIndex));

            ma_atomic_store_explicit_32(&pDeviceStatePsp->subBufferIndex,
                (subBufferIndex + 1) & 1, ma_atomic_memory_order_relaxed);
            ma_atomic_fetch_sub_explicit_32(&pDeviceStatePsp->validSubBufferCount,
                1, ma_atomic_memory_order_release);
        } else {
            ma_sleep(1);
        }
    }

    return (ma_thread_result)0;
}

static ma_result ma_device_init__psp(ma_device* pDevice, const void* pDeviceBackendConfig,
    ma_device_descriptor* pDescriptorPlayback, ma_device_descriptor* pDescriptorCapture,
    void** ppDeviceState)
{
    ma_result result;
    ma_device_state_psp* pDeviceStatePsp;
    ma_context_state_psp* pContextStatePsp =
        ma_context_get_backend_state__psp(ma_device_get_context(pDevice));
    ma_device_type deviceType = ma_device_get_type(pDevice);
    ma_log* pLog = ma_device_get_log(pDevice);
    ma_format format;
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ma_uint32 periodSizeInFrames;
    ma_uint32 periodSizeInBytes;
    size_t allocSize;
    void* pRaw;

    (void)pContextStatePsp;
    (void)pDeviceBackendConfig;
    (void)pDescriptorCapture;

    if (deviceType != ma_device_type_playback)
        return MA_DEVICE_TYPE_NOT_SUPPORTED;

    pDeviceStatePsp = (ma_device_state_psp*)ma_calloc(sizeof(*pDeviceStatePsp),
        ma_device_get_allocation_callbacks(pDevice));
    if (pDeviceStatePsp == NULL) return MA_OUT_OF_MEMORY;

    /* PSP Audio2: always 16-bit stereo PCM. */
    format = ma_format_s16;
    channels = 2;

    /* Audio2 has no sample-rate control; hardware default is 44100. */
    sampleRate = 44100;

    /* Calculate and clamp period size to Audio2 range. */
    periodSizeInFrames = ma_calculate_buffer_size_in_frames_from_descriptor(pDescriptorPlayback, sampleRate);
    periodSizeInFrames = ma_clamp(periodSizeInFrames, PSP_AUDIO2_MIN_PERIOD, PSP_AUDIO2_MAX_PERIOD);

    /* Sub-buffer size in bytes (one period). */
    periodSizeInBytes = periodSizeInFrames * ma_get_bytes_per_frame(format, channels);

    /* Allocate with extra room for 64-byte alignment (2 sub-buffers). */
    allocSize = periodSizeInBytes * 2 + PSP_AUDIO_BUF_ALIGNMENT;
    pRaw = ma_malloc(allocSize, ma_device_get_allocation_callbacks(pDevice));
    if (pRaw == NULL) {
        ma_free(pDeviceStatePsp, ma_device_get_allocation_callbacks(pDevice));
        return MA_OUT_OF_MEMORY;
    }

    pDeviceStatePsp->pSubBuffers = pRaw;
    pDeviceStatePsp->pSubBufferAligned[0] = psp_align_ptr(pRaw, PSP_AUDIO_BUF_ALIGNMENT);
    pDeviceStatePsp->subBufferSizeInBytes = periodSizeInBytes;

    /* Reserve Audio2 output. */
    if (sceAudioOutput2Reserve((int)periodSizeInFrames) < 0) {
        ma_free(pRaw, ma_device_get_allocation_callbacks(pDevice));
        ma_free(pDeviceStatePsp, ma_device_get_allocation_callbacks(pDevice));
        ma_log_postf(pLog, MA_LOG_LEVEL_ERROR, "[PSP] sceAudioOutput2Reserve failed.");
        return MA_ERROR;
    }

    ma_atomic_store_explicit_32(&pDeviceStatePsp->isRunning, 1, ma_atomic_memory_order_relaxed);

    /* Create the audio output thread (sceAudioOutput2OutputBlocking is blocking). */
    result = ma_thread_create(&pDeviceStatePsp->thread, ma_thread_priority_default, 0,
                              ma_device_audio_thread__psp, pDevice,
                              ma_device_get_allocation_callbacks(pDevice));
    if (result != MA_SUCCESS) {
        sceAudioOutput2Release();
        ma_free(pRaw, ma_device_get_allocation_callbacks(pDevice));
        ma_free(pDeviceStatePsp, ma_device_get_allocation_callbacks(pDevice));
        ma_log_postf(pLog, MA_LOG_LEVEL_ERROR, "[PSP] Failed to create audio thread.");
        return MA_ERROR;
    }

    /* Update descriptor with actual internal settings. */
    pDescriptorPlayback->format             = format;
    pDescriptorPlayback->channels           = channels;
    pDescriptorPlayback->sampleRate         = sampleRate;
    pDescriptorPlayback->periodSizeInFrames = periodSizeInFrames;
    pDescriptorPlayback->periodCount        = 2;

    *ppDeviceState = pDeviceStatePsp;
    return MA_SUCCESS;
}

static void ma_device_uninit__psp(ma_device* pDevice)
{
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);

    ma_atomic_store_explicit_32(&pDeviceStatePsp->isRunning, 0, ma_atomic_memory_order_relaxed);
    ma_thread_wait(&pDeviceStatePsp->thread);

    sceAudioOutput2Release();
    ma_free(pDeviceStatePsp->pSubBuffers, ma_device_get_allocation_callbacks(pDevice));
    ma_free(pDeviceStatePsp, ma_device_get_allocation_callbacks(pDevice));
}

static ma_result ma_device_start__psp(ma_device* pDevice)
{
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);

    ma_atomic_store_explicit_32(&pDeviceStatePsp->subBufferIndex,      0, ma_atomic_memory_order_relaxed);
    ma_atomic_store_explicit_32(&pDeviceStatePsp->validSubBufferCount, 0, ma_atomic_memory_order_relaxed);

    return MA_SUCCESS;
}

static ma_result ma_device_stop__psp(ma_device* pDevice)
{
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);

    while (ma_atomic_load_explicit_32(&pDeviceStatePsp->validSubBufferCount,
                                      ma_atomic_memory_order_relaxed) > 0)
    {
        ma_sleep(1);
    }

    return MA_SUCCESS;
}

static ma_result ma_device_step__psp(ma_device* pDevice, ma_blocking_mode blockingMode)
{
    ma_device_state_psp* pDeviceStatePsp = ma_device_get_backend_state__psp(pDevice);

    for (;;) {
        if (!ma_device_is_started(pDevice))
            return MA_DEVICE_NOT_STARTED;

        if (ma_atomic_load_explicit_32(&pDeviceStatePsp->validSubBufferCount,
                                       ma_atomic_memory_order_acquire) < 2)
        {
            ma_uint32 subBufferIndex = ma_atomic_load_explicit_32(
                &pDeviceStatePsp->subBufferIndex, ma_atomic_memory_order_relaxed);
            ma_device_handle_backend_data_callback(pDevice,
                ma_device_get_sub_buffer__psp(pDevice, subBufferIndex), NULL,
                pDevice->playback.internalPeriodSizeInFrames);
            ma_atomic_fetch_add_explicit_32(&pDeviceStatePsp->validSubBufferCount,
                1, ma_atomic_memory_order_release);
            return MA_SUCCESS;
        }

        if (blockingMode == MA_BLOCKING_MODE_NON_BLOCKING)
            return MA_SUCCESS;

        ma_sleep(1);
    }
}

static void ma_device_wakeup__psp(ma_device* pDevice)
{
    (void)pDevice;
}

static ma_device_backend_vtable ma_gDeviceBackendVTable_Psp =
{
    ma_backend_info__psp,
    ma_context_init__psp,
    ma_context_uninit__psp,
    ma_context_enumerate_devices__psp,
    ma_device_init__psp,
    ma_device_uninit__psp,
    ma_device_start__psp,
    ma_device_stop__psp,
    ma_device_step__psp,
    ma_device_wakeup__psp
};

ma_device_backend_vtable* ma_device_backend_psp = &ma_gDeviceBackendVTable_Psp;
#else
ma_device_backend_vtable* ma_device_backend_psp = NULL;
#endif  /* MA_HAS_PSP */


/* ============================================================================
 * SECTION 7 — Init function implementations & vtable accessor
 * ============================================================================
 */

MA_API ma_device_backend_vtable* ma_psp_get_vtable(void)
{
    return ma_device_backend_psp;
}

MA_API ma_context_config_psp ma_context_config_psp_init(void)
{
    ma_context_config_psp config;
    MA_ZERO_OBJECT(&config);
    return config;
}

MA_API ma_device_config_psp ma_device_config_psp_init(void)
{
    ma_device_config_psp config;
    MA_ZERO_OBJECT(&config);
    return config;
}


/* ============================================================================
 * SECTION 8 — Backend registration
 * ============================================================================
 * Register in ma_context_init() and ma_get_stock_device_backends():
 *
 *   if (pVTable == ma_device_backend_psp) {
 *       return &pConfig->psp;
 *   }
 *
 *   pBackends[count++] = ma_device_backend_config_init(ma_device_backend_psp, NULL);
 */


/* ============================================================================
 * APPENDIX — SDK API COMPARISON: PSP Audio2 vs Vita Audio
 * ============================================================================
 *
 * Concept              Vita SDK                          PSP SDK (Audio2)
 * -------------------  --------------------------------  --------------------------------
 * Open/reserve         sceAudioOutOpenPort(...)          sceAudioOutput2Reserve(period)
 * Output (blocking)    sceAudioOutOutput(port, buf)      sceAudioOutput2OutputBlocking(vol, buf)
 * Close/release        sceAudioOutReleasePort(port)      sceAudioOutput2Release()
 * Port type            BGM / MAIN                        N/A (single output)
 * Sample rate          Configurable set                  Fixed (hardware default, ~44100)
 * Channel modes        MONO / STEREO enum                Stereo only
 * Period range         [SCE_AUDIO_MIN_LEN, MAX_LEN]      17–4111
 * Buffer alignment     64-byte (AICA)                    64-byte
 * Thread create        7 params (+cpuAffinityMask)       6 params
 * Thread wait end      3 params (+stat)                  2 params (no stat)
 * Mutex                sceKernelCreateMutex               Binary semaphore (sceKernelCreateSema)
 * Event flag           sceKernelCreateEventFlag          sceKernelCreateEventFlag
 * Semaphore            sceKernelCreateSema               sceKernelCreateSema
 *
 * SHARED PATTERNS (same API on both):
 *   sceKernelStartThread(id, argSize, argp)
 *   sceKernelDeleteThread(id)
 *   sceKernelWaitSema(id, count, timeout)
 *   sceKernelSignalSema(id, count)
 *   sceKernelSetEventFlag(id, bits)
 *   sceKernelWaitEventFlag(id, bits, op, outBits, timeout)
 *   sceKernelDeleteEventFlag(id)
 *   sceKernelDeleteSema(id)
 */
