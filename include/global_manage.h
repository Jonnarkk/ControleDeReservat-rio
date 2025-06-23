#ifndef GLOBAL_MANAGE_H
#define GLOBAL_MANAGE_H

#include "pico/stdlib.h"

typedef struct
{
    int max;
    int min;
    int nivel_atual;
} Nivel;

void set_max(int max);

int get_max();

void set_min(int min);

int get_min();

void set_nivel(int niv_adc);

int get_nivel();

void set_motor(bool estado);

bool get_motor();

#endif
