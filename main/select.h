#ifndef SELECT_H
#define SELECT_H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "player.h"
#include "volum.h"

void select_init();
void show_menu();
void m_select();
void select_read_stdio();
void select_read_serial();
void socket_select_recv_data();
int parse_message(char *msg, char *cmd);

#endif