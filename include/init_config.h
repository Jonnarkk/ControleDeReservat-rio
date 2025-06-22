#ifndef INIT_CONFIG_H
#define INIT_CONFIG_H

#include "ssd1306.h"

#define ADC_CHANNEL 2
#define ADC_PIN (26 + ADC_CHANNEL)

#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 100

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

#define GPIO_18 18

void display_init(ssd1306_t *ssd);

void buzzer_init();

void adc_init_custom();

void init_global_manage();

#endif