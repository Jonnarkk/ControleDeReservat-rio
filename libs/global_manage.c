// Inclui o cabeçalho que declara funções e variáveis globais utilizadas neste módulo
#include "global_manage.h"

Nivel nivel;

bool motor;

void set_max(int max) 
{
    nivel.max = max; 
}

int get_max() 
{
    return nivel.max;
}

void set_min(int min) 
{
    nivel.min = min;
}

int get_min() 
{
    return nivel.min;
}

void set_nivel(int niv_adc) 
{
    nivel.nivel_atual = (niv_adc/4095)*100;  
}

int get_nivel() 
{
    return nivel.nivel_atual;
}

void set_motor(bool estado) 
{
    motor = estado;
}

bool get_motor() 
{
    return motor;
}