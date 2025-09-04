#ifndef VOLUM_H
#define VOLUM_H

#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

int open_master(void);
int get_volume_percent(void);
void set_volume_percent(int percent);

#endif
