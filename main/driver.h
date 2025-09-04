#ifndef DRIVER_H
#define DRIVER_H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include "volum.h"

int driver_init(void);
void start_buzzer();
int get_key_id();
int init_serial();

#endif