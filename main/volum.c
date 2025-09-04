#include "volum.h"

snd_mixer_t *g_mixer = NULL;
snd_mixer_elem_t *g_elem = NULL;

/* 打开 Master */
int open_master(void)
{
    snd_mixer_selem_id_t *sid;

    if (snd_mixer_open(&g_mixer, 0) < 0)
        return -1;
    if (snd_mixer_attach(g_mixer, "default") < 0)
        goto fail;
    snd_mixer_selem_register(g_mixer, NULL, NULL);
    snd_mixer_load(g_mixer);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master"); /* 无 Master 可改 PCM */
    g_elem = snd_mixer_find_selem(g_mixer, sid);
    if (!g_elem || !snd_mixer_selem_has_playback_volume(g_elem))
        goto fail;
    return 0;

fail:
    snd_mixer_close(g_mixer);
    g_mixer = NULL;
    return -1;
}

/* 获取音量百分比 ----------------------------------------------------------*/
int get_volume_percent(void)
{
    long min, max, vol;

    // 手动刷新 mixer 状态（关键）
    snd_mixer_handle_events(g_mixer);

    snd_mixer_selem_get_playback_volume_range(g_elem, &min, &max);
    snd_mixer_selem_get_playback_volume(g_elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    return (int)((vol - min) * 100 / (max - min));
}

/* 设置音量百分比 ----------------------------------------------------------*/
void set_volume_percent(int percent)
{
    long min, max;
    snd_mixer_selem_get_playback_volume_range(g_elem, &min, &max);
    long vol = min + (max - min) * percent / 100;
    if (vol < min)
        vol = min;
    if (vol > max)
        vol = max;
    snd_mixer_selem_set_playback_volume_all(g_elem, vol);
}
