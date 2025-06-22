#include "init_config.h"        // Declarações de pinos, constantes e protótipos de funções de inicialização

#include "hardware/pwm.h"       // API de PWM para controle de sinais sonoros
#include "hardware/clocks.h"    // API de clocks do RP2040
#include "led_matriz.h"
#include "global_manage.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

// Configura e inicializa o display SSD1306 via I2C
void display_init(ssd1306_t *ssd)
{
    // Inicializa o barramento I2C na porta definida a 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);

    // Configura pinos SDA e SCL para modo I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    // Habilita resistores de pull-up internos nas linhas I2C
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Chama rotina da biblioteca para inicializar o SSD1306 (endereço, dimensões, I2C)
    ssd1306_init(ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    // Configura parâmetros internos do display (scroll, inversão, etc.)
    ssd1306_config(ssd);
    // Envia buffer de inicialização ao display
    ssd1306_send_data(ssd);

    // Limpa a tela (preenche com pixels apagados) e envia ao display
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}

// Configura o PWM no pino do buzzer para gerar o tom desejado
void buzzer_init() 
{
    // Associa o pino do buzzer à função PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Recupera o slice de PWM correspondente ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Carrega a configuração padrão de PWM
    pwm_config config = pwm_get_default_config();
    // Ajusta o divisor de clock para que a frequência seja BUZZER_FREQUENCY com 12-bit de resolução
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
    // Inicializa o slice de PWM com a configuração e já habilita o módulo
    pwm_init(slice_num, &config, true);

    // Define nível inicial zero (silêncio)
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// volatile void gpio_irq_handler(uint gpio, uint32_t events){
//     PIO pio = pio0;
//     uint sm = pio_init(pio);

//     apagar_matriz(pio, sm);
    
//     ssd1306_fill(ssd, false);
//     ssd1306_send_data(ssd);
//     reset_usb_boot(0, 0);
// }

// // Configura o pino do botão como entrada com pull-up
// void button_init()
// {
//     gpio_init(BOTAOB);
//     gpio_set_dir(BOTAOB, GPIO_IN);
//     gpio_pull_up(BOTAOB);
//     gpio_set_irq_enabled_with_callback(BOTAOB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
// }

// Configura os pinos dos LEDs como saída
void adc_init_custom()
{
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);
}

void init_global_manage() 
{
    set_max(100);
    set_min(0);
    set_nivel(0);
    set_motor(false);
}