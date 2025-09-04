#ifndef SOCKET_H
#define SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <sys/shm.h>
#include "player.h"
#include "driver.h"

#define PORT 10001
#define SERVERADDR "121.196.244.2" // 服务器公网ip地址
// #define SERVERADDR "192.168.232.134" //tcp自测

void send_server(int sig);
int socket_init();
void socket_send_data(json_object *j);
void socket_recv_data(char *msg);
void upload_music_list();
int socket_start_play();
void socket_stop_play();
void socket_suspend_play();
void socket_continue_play();
void socket_prior_play();
void socket_next_play();
void socket_voice_up();
void socket_voice_dowm();
void socket_circle_play();
void socket_sequence_play();

#endif