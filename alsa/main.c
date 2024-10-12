#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <alsa/output.h>
#include <alsa/input.h>
#include <alsa/conf.h>
#include <alsa/global.h>
#include <alsa/timer.h>
#include <alsa/pcm.h>
#include <linux/soundcard.h>
#include <json-c/json.h>

#ifdef QX1000
#include <pulse/pulseaudio.h>
#endif

#ifdef UNITTEST
#include "unity_fixture.h"
#endif

#include "detour.h"

#ifdef MMIYOO
#include "mi_ao.h"
#include "mi_sys.h"
#include "mi_common_datatype.h"
#endif

#define PREFIX              "[SND] "
#define FREQ                44100
#define PERIOD              2048
#define CHANNELS            2

#if defined(PANDORA)
    #define SAMPLES         2048
#else
    #define SAMPLES         8192
#endif

#define QUEUE_SIZE          (2 * 1024 * 1024)

#ifdef MMIYOO
    #define MAX_VOLUME      20
    #define MIN_RAW_VALUE   -60
    #define MAX_RAW_VALUE   30
    #define MI_AO_SETVOLUME 0x4008690b
    #define MI_AO_GETVOLUME 0xc008690c
    #define MI_AO_SETMUTE   0x4008690d
#endif

#define JSON_APP_FILE       "/appconfigs/system.json"
#define JSON_CFG_FILE       "resources/settings.json"
#define JSON_VOL_KEY        "vol"
#define JSON_AUTO_STATE     "auto_state"
#define JSON_AUTO_SLOT      "auto_slot"
#define JSON_HALF_VOL       "half_vol"

typedef struct {
    int size;
    int read;
    int write;
    uint8_t *buffer;
    pthread_mutex_t lock;
} queue_t;

static pthread_t thread;
static queue_t queue = {0};

#ifdef QX1000
typedef struct {
    pa_threaded_mainloop *mainloop;
    pa_context *context;
    pa_mainloop_api *api;
    pa_stream *stream;
    pa_sample_spec spec;
    pa_buffer_attr attr;
} pa_t;

static pa_t pa = {0};
#endif

#ifdef MMIYOO
    static MI_AO_CHN AoChn = 0;
    static MI_AUDIO_DEV AoDevId = 0;
    static MI_AUDIO_Attr_t stSetAttr = {0};
    static MI_AUDIO_Attr_t stGetAttr = {0};
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    static int dsp_fd = -1;
#endif

static int half_vol = 0;
static int auto_slot = 0;
static int auto_state = 1;
static int cur_volume = 0;
static int pcm_ready = 0;
static int pcm_buf_len = 0;
static uint8_t *pcm_buf = NULL;
static struct json_object *jfile = NULL;

#ifdef QX1000
static void context_state_cb(pa_context *context, void *userdata)
{
    if (context) {
        switch (pa_context_get_state(context)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(pa.mainloop, 0);
            break;
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
        }
    }
}

static void stream_state_cb(pa_stream *stream, void *userdata)
{
    if (stream) {
        switch (pa_stream_get_state(stream)) {
        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(pa.mainloop, 0);
            break;
        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
            break;
        }
    }
}

static void stream_latency_update_cb(pa_stream *stream, void *userdata)
{
    if (stream) {
        pa_threaded_mainloop_signal(pa.mainloop, 0);
    }
}

static void stream_request_cb(pa_stream *stream, size_t length, void *userdata)
{
    if (stream) {
        pa_threaded_mainloop_signal(pa.mainloop, 0);
    }
}
#endif

#if !defined(UNITTEST)
void neon_memcpy(void *dest, const void *src, size_t n);
#endif

#if defined(PANDORA)
int snd_lib_error_set_handler(int handler)
{
    return 0;
}
#endif

#ifdef MMIYOO
static int set_volume_raw(int value, int add)
{
    int fd = -1;
    int buf2[] = {0, 0}, prev_value = 0;
    uint64_t buf1[] = {sizeof(buf2), (uintptr_t)buf2};

    if ((fd = open("/dev/mi_ao", O_RDWR)) < 0) {
        return 0;
    }

    ioctl(fd, MI_AO_GETVOLUME, buf1);
    prev_value = buf2[1];

    if (add) {
        value = prev_value + add;
    }
    else {
        value += MIN_RAW_VALUE;
    }

    if (value > MAX_RAW_VALUE) {
        value = MAX_RAW_VALUE;
    }
    else if (value < MIN_RAW_VALUE) {
        value = MIN_RAW_VALUE;
    }

    if (value == prev_value) {
        close(fd);
        return prev_value;
    }

    buf2[1] = value;
    ioctl(fd, MI_AO_SETVOLUME, buf1);
    if (prev_value <= MIN_RAW_VALUE && value > MIN_RAW_VALUE) {
        buf2[1] = 0;
        ioctl(fd, MI_AO_SETMUTE, buf1);
    }
    else if (prev_value > MIN_RAW_VALUE && value <= MIN_RAW_VALUE) {
        buf2[1] = 1;
        ioctl(fd, MI_AO_SETMUTE, buf1);
    }
    close(fd);
    return value;
}

static int set_volume(int volume)
{
    int volume_raw = 0;
    int div = half_vol ? 2 : 1;

    if (volume > MAX_VOLUME) {
        volume = MAX_VOLUME;
    }
    else if (volume < 0) {
        volume = 0;
    }

    if (volume != 0) {
        volume_raw = round(48 * log10(1 + volume));
    }

    set_volume_raw(volume_raw / div, 0);
    return volume;
}

int volume_inc(void)
{
    if (cur_volume < MAX_VOLUME) {
        cur_volume+= 1;
        set_volume(cur_volume);
    }
    return cur_volume;
}

int volume_dec(void)
{
    if (cur_volume > 0) {
        cur_volume-= 1;
        set_volume(cur_volume);
    }
    return cur_volume;
}
#endif

#if defined(TRIMUI)
int volume_inc(void)
{
    return 0;
}

int volume_dec(void)
{
    return 0;
}
#endif

void snd_nds_reload_config(void)
{
    jfile = json_object_from_file(JSON_CFG_FILE);
    if (jfile != NULL) {
        struct json_object *jval = NULL;

        json_object_object_get_ex(jfile, JSON_AUTO_STATE, &jval);
        auto_state = json_object_get_int(jval);

        json_object_object_get_ex(jfile, JSON_AUTO_SLOT, &jval);
        auto_slot = json_object_get_int(jval);

        json_object_object_get_ex(jfile, JSON_HALF_VOL, &jval);
        half_vol = json_object_get_int(jval) ? 1 : 0;

        json_object_put(jfile);
    }
}

static void queue_init(queue_t *q, size_t s)
{
    q->buffer = (uint8_t *)malloc(s);
    q->size = s;
    q->read = q->write = 0;
    pthread_mutex_init(&q->lock, NULL);
}

static void queue_destroy(queue_t *q)
{
    if (q->buffer) {
        free(q->buffer);
    }
    pthread_mutex_destroy(&q->lock);
}

static int queue_size_for_read(queue_t *q)
{
    if (q->read == q->write) {
        return 0;
    }
    else if(q->read < q->write){
        return q->write - q->read;
    }
    return (QUEUE_SIZE - q->read) + q->write;
}

static int queue_size_for_write(queue_t *q)
{
    if (q->write == q->read) {
        return QUEUE_SIZE;
    }
    else if(q->write < q->read){
        return q->read - q->write;
    }
    return (QUEUE_SIZE - q->write) + q->read;
}

static int queue_put(queue_t *q, uint8_t *buffer, size_t size)
{
    int r = 0, tmp = 0, avai = 0;

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_write(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->write >= q->read) && ((q->write + size) > QUEUE_SIZE)) {
            tmp = QUEUE_SIZE - q->write;
            size-= tmp;
#if !defined(UNITTEST)
            neon_memcpy(&q->buffer[q->write], buffer, tmp);
            neon_memcpy(q->buffer, &buffer[tmp], size);
#endif
            q->write = size;
        }
        else {
#if !defined(UNITTEST)
            neon_memcpy(&q->buffer[q->write], buffer, size);
#endif
            q->write += size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

static size_t queue_get(queue_t *q, uint8_t *buffer, size_t max)
{
    int r = 0, tmp = 0, avai = 0, size = max;

    pthread_mutex_lock(&q->lock);
    avai = queue_size_for_read(q);
    if (size > avai) {
        size = avai;
    }
    r = size;

    if (size > 0) {
        if ((q->read > q->write) && (q->read + size) > QUEUE_SIZE) {
            tmp = QUEUE_SIZE - q->read;
            size-= tmp;
#if !defined(UNITTEST)
            neon_memcpy(buffer, &q->buffer[q->read], tmp);
            neon_memcpy(&buffer[tmp], q->buffer, size);
#endif
            q->read = size;
        }
        else {
#if !defined(UNITTEST)
            neon_memcpy(buffer, &q->buffer[q->read], size);
#endif
            q->read+= size;
        }
    }
    pthread_mutex_unlock(&q->lock);
    return r;
}

static void *audio_handler(void *threadid)
{
#ifdef MMIYOO
    MI_AUDIO_Frame_t aoTestFrame = {0};
#endif
    int r = 0, len = pcm_buf_len, idx = 0;

    while (pcm_ready) {
        r = queue_get(&queue, &pcm_buf[idx], len);
        if (r > 0) {
            idx+= r;
            len-= r;
            if (len == 0) {
                idx = 0;
                len = pcm_buf_len;
#if defined(MMIYOO) && !defined(UNITTEST)
                aoTestFrame.eBitwidth = stGetAttr.eBitwidth;
                aoTestFrame.eSoundmode = stGetAttr.eSoundmode;
                aoTestFrame.u32Len = pcm_buf_len;
                aoTestFrame.apVirAddr[0] = pcm_buf;
                aoTestFrame.apVirAddr[1] = NULL;
                MI_AO_SendFrame(AoDevId, AoChn, &aoTestFrame, 1);
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
                write(dsp_fd, pcm_buf, pcm_buf_len);
#endif

#ifdef QX1000
                if (pa.mainloop) {
                    pa_threaded_mainloop_lock(pa.mainloop);
                    pa_stream_write(pa.stream, pcm_buf, pcm_buf_len, NULL, 0, PA_SEEK_RELATIVE);
                    pa_threaded_mainloop_unlock(pa.mainloop);
                }
#endif
            }
        }
        usleep(10);
    }
    pthread_exit(NULL);
}

snd_pcm_sframes_t snd_pcm_avail(snd_pcm_t *pcm)
{
    return 0; // pcm_buf_len;
}

int snd_pcm_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

int snd_pcm_hw_params_any(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    return 0;
}

void snd_pcm_hw_params_free(snd_pcm_hw_params_t *obj)
{
}

int snd_pcm_hw_params_malloc(snd_pcm_hw_params_t **ptr)
{
    return 0;
}

int snd_pcm_hw_params_set_access(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access)
{
    return 0;
}

int snd_pcm_hw_params_set_buffer_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val)
{
    *val = SAMPLES * 2 * CHANNELS;
    return 0;
}

int snd_pcm_hw_params_set_channels(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val)
{
    return 0;
}

int snd_pcm_hw_params_set_format(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val)
{
    if (val != SND_PCM_FORMAT_S16_LE) {
        return -1;
    }
    return 0;
}

int snd_pcm_hw_params_set_period_size_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir)
{
    *val = PERIOD;
    return 0;
}

int snd_pcm_hw_params_set_rate_near(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir)
{
    *val = FREQ;
    return 0;
}

int snd_pcm_open(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode)
{
    printf(PREFIX"Use customized ALSA\n");
    if (stream != SND_PCM_STREAM_PLAYBACK) {
        return -1;
    }
    return 0;
}

int snd_pcm_prepare(snd_pcm_t *pcm)
{
    return 0;
}

snd_pcm_sframes_t snd_pcm_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
    return 0;
}

int snd_pcm_recover(snd_pcm_t *pcm, int err, int silent)
{
    return 0;
}

int snd_pcm_start(snd_pcm_t *pcm)
{
#ifdef MMIYOO
    MI_S32 miret = 0;
    MI_SYS_ChnPort_t stAoChn0OutputPort0;
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    int arg = 0;
#endif

    queue_init(&queue, (size_t)QUEUE_SIZE);
    if (queue.buffer == NULL) {
        return -1;
    }
    memset(queue.buffer, 0, QUEUE_SIZE);

    pcm_buf_len = SAMPLES * 2 * CHANNELS;
    pcm_buf = malloc(pcm_buf_len);
    if (pcm_buf == NULL) {
        return -1;
    }
    memset(pcm_buf, 0, pcm_buf_len);

#if !defined(PANDORA)
    jfile = json_object_from_file(JSON_APP_FILE);
    if (jfile != NULL) {
        struct json_object *volume = NULL;

        json_object_object_get_ex(jfile, JSON_VOL_KEY, &volume);
        cur_volume = json_object_get_int(volume);
        //json_object_object_add(jfile, JSON_VOL_KEY, json_object_new_int(2));
        //json_object_to_file(JSON_APP_FILE, jfile);
        json_object_put(jfile);
    }
#endif

#if defined(MMIYOO) && !defined(UNITTEST)
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = SAMPLES;
    stSetAttr.u32ChnCnt = CHANNELS;
    stSetAttr.eSoundmode = CHANNELS == 2 ? E_MI_AUDIO_SOUND_MODE_STEREO : E_MI_AUDIO_SOUND_MODE_MONO;
    stSetAttr.eSamplerate = (MI_AUDIO_SampleRate_e)FREQ;

    miret = MI_AO_SetPubAttr(AoDevId, &stSetAttr);
    if(miret != MI_SUCCESS) {
        printf(PREFIX"Failed to set PubAttr\n");
        return -1;
    }

    miret = MI_AO_GetPubAttr(AoDevId, &stGetAttr);
    if(miret != MI_SUCCESS) {
        printf(PREFIX"Failed to get PubAttr\n");
        return -1;
    }

    miret = MI_AO_Enable(AoDevId);
    if(miret != MI_SUCCESS) {
        printf(PREFIX"Failed to enable AO\n");
        return -1;
    }

    miret = MI_AO_EnableChn(AoDevId, AoChn);
    if(miret != MI_SUCCESS) {
        printf(PREFIX"Failed to enable Channel\n");
        return -1;
    }

    stAoChn0OutputPort0.eModId = E_MI_MODULE_ID_AO;
    stAoChn0OutputPort0.u32DevId = AoDevId;
    stAoChn0OutputPort0.u32ChnId = AoChn;
    stAoChn0OutputPort0.u32PortId = 0;
    MI_SYS_SetChnOutputPortDepth(&stAoChn0OutputPort0, 12, 13);
    set_volume(cur_volume);
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    dsp_fd = open("/dev/dsp", O_WRONLY);
    if (dsp_fd < 0) {
        printf(PREFIX"Failed to open /dev/dsp device\n");
        return -1;
    }

    arg = 16;
    ioctl(dsp_fd, SOUND_PCM_WRITE_BITS, &arg);

    arg = CHANNELS;
    ioctl(dsp_fd, SOUND_PCM_WRITE_CHANNELS, &arg);

    arg = FREQ;
    ioctl(dsp_fd, SOUND_PCM_WRITE_RATE, &arg);
#endif

#ifdef QX1000
    pa.mainloop = pa_threaded_mainloop_new();
    if (pa.mainloop == NULL) {
        printf(PREFIX"Failed to open PulseAudio device\n");
        return -1;
    }

    pa.api = pa_threaded_mainloop_get_api(pa.mainloop);
    pa.context = pa_context_new(pa.api, "DraStic");
    pa_context_set_state_callback(pa.context, context_state_cb, &pa);
    pa_context_connect(pa.context, NULL, 0, NULL);
    pa_threaded_mainloop_lock(pa.mainloop);
    pa_threaded_mainloop_start(pa.mainloop);

    while (pa_context_get_state(pa.context) != PA_CONTEXT_READY) {
        pa_threaded_mainloop_wait(pa.mainloop);
    }

    pa.spec.format = PA_SAMPLE_S16LE;
    pa.spec.channels = CHANNELS;
    pa.spec.rate = FREQ;
    pa.attr.tlength = pa_bytes_per_second(&pa.spec) / 5;
    pa.attr.maxlength = pa.attr.tlength * 3;
    pa.attr.minreq = pa.attr.tlength / 3;
    pa.attr.prebuf = pa.attr.tlength;

    pa.stream = pa_stream_new(pa.context, "DraStic", &pa.spec, NULL);
    pa_stream_set_state_callback(pa.stream, stream_state_cb, &pa);
    pa_stream_set_write_callback(pa.stream, stream_request_cb, &pa);
    pa_stream_set_latency_update_callback(pa.stream, stream_latency_update_cb, &pa);
    pa_stream_connect_playback(pa.stream, NULL, &pa.attr, 
        PA_STREAM_ADJUST_LATENCY | PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
    
    while (pa_context_get_state(pa.context) != PA_CONTEXT_READY) {
        pa_threaded_mainloop_wait(pa.mainloop);
    }

    pa_threaded_mainloop_unlock(pa.mainloop);
#endif

    pcm_ready = 1;
    pthread_create(&thread, NULL, audio_handler, (void *)NULL);
    return 0;
}

int snd_pcm_close(snd_pcm_t *pcm)
{
    void *ret = NULL;

    if (auto_state > 0) {
        dtr_savestate(auto_slot);
    }

    pcm_ready = 0;
    pthread_join(thread, &ret);
    if (pcm_buf != NULL) {
        free(pcm_buf);
        pcm_buf = NULL;
    }
    queue_destroy(&queue);

#if defined(MMIYOO) && !defined(UNITTEST)
    MI_AO_DisableChn(AoDevId, AoChn);
    MI_AO_Disable(AoDevId);
#endif

#if defined(TRIMUI) || defined(FUNKEYS) || defined(PANDORA)
    if (dsp_fd > 0) {
        close(dsp_fd);
        dsp_fd = -1;
    }
#endif

#ifdef QX1000
    if (pa.mainloop) {
        pa_threaded_mainloop_stop(pa.mainloop);
    }
    if (pa.stream) {
        pa_stream_unref(pa.stream);
        pa.stream = NULL;
    }
    if (pa.context) {
        pa_context_disconnect(pa.context);
        pa_context_unref(pa.context);
        pa.context = NULL;
    }
    if (pa.mainloop) {
        pa_threaded_mainloop_stop(pa.mainloop);
        pa.mainloop = NULL;
    }
#endif
    return 0;
}

int snd_pcm_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

int snd_pcm_sw_params_current(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
    return 0;
}

void snd_pcm_sw_params_free(snd_pcm_sw_params_t *obj)
{
}

int snd_pcm_sw_params_malloc(snd_pcm_sw_params_t **ptr)
{
    return 0;
}

snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
    if ((size > 1) && (size != pcm_buf_len)) {
        queue_put(&queue, (uint8_t*)buffer, size * 2 * CHANNELS);
    }
    return size;
}

#ifdef UNITTEST
TEST_GROUP(alsa);

TEST_SETUP(alsa)
{
}

TEST_TEAR_DOWN(alsa)
{
}

TEST(alsa, snd_pcm_writei)
{
    TEST_ASSERT_EQUAL_INT(snd_pcm_writei(NULL, NULL, 0), 0);
}

TEST_GROUP_RUNNER(alsa)
{
    RUN_TEST_CASE(alsa, snd_pcm_writei);
}
#endif

