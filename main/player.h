#ifndef PLAYER_H
#define PLAYER_H

#include <sys/types.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <sys/wait.h>
#include "socket.h"
#include "link.h"
#include "volum.h"

#define SHM_SEQUENCY 1 // 顺序播放
#define SHM_CYCLE 2    // 循环播放

#define URL "http://121.196.244.2/music/"
// #define URL "/home/gec/music/"   //ubuntu自测
#define DEF_VOL 100

#define SHMKEY 1234
#define SHMSIZE 4096
#define PCM_PLAYBACK_DEV "plughw:0,0"
#define PCM_CHANNELS_TWO 2
#define PCM_RATE 8000
#define PCM_PLAYBACK_MIXER_DEV "hw:0"

static snd_pcm_t *pcm = NULL;                         // pcm 句柄
static void *buf = NULL;                              // 指向应用程序缓冲区指针
static snd_pcm_uframes_t period_size = 1024;          // 周期大小
static unsigned int period = 16;                      // 周期数
static unsigned int buf_byte;                         // 应用程序缓冲区大小
static int fd = -1;                                   // 音频文件描述符
static snd_mixer_elem_t *playback_vol_element = NULL; // 控制<音量>元素
static snd_mixer_t *mixer = NULL;                     // 声卡句柄
static int frame_byte = 4;                            // 一帧字节大小 采样格式 + 声道数

/*
 * 共享内存数据结构，用于父子/爷孙进程间同步播放状态。
 * 包含当前播放歌曲名、播放模式、子进程和孙进程的 PID。
 */
typedef struct shm
{
    int parent_id;       // 父进程pid
    int child_id;        // 子进程pid
    int grand_id;        // 孙进程pid
    char cur_music[128]; // 当前播放歌曲名字
    int mod;             // 音乐播放器模式
} SHM;

int shm_init(void);
void get_shm(SHM *s);
void get_volum(int *volum);
void get_music(const char *singer);
int start_play();
void play_music(char *n);
void child_process(char *n);
void grand_get_shm(SHM *s);
void grand_set_shm(SHM s);
void child_quit();
void stop_play();
void write_fifo(const char *cmd);
void suspend_play();
void continue_play();
void next_play();
void set_shm(SHM s);
void prior_play();
void circle_play();
void sequence_play();
void singer_play(const char *singer);

#endif