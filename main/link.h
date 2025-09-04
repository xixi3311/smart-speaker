#ifndef LINK_H
#define LINK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socket.h"
#include "player.h"

#define music_number 5

/*
    链表节点
*/
typedef struct Node
{
    char music_name[64];
    struct Node *next;
    struct Node *prior;
} Node;

int link_init();
void create_link(char *s);
int insert_link(const char *name);
void traverse_link();
int find_next_music(char *cur, int mode, char *next);
void clear_link();
void update_music();
void get_singer(char *s);
void find_prior_music(char *cur, char *prior);

#endif