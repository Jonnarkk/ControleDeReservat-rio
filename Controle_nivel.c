#include <stdio.h>             // Biblioteca padrão de entrada/saída (ex: printf)
#include <string.h>            // Biblioteca para manipulação de strings
#include "pico/stdlib.h"       // Biblioteca do SDK do Raspberry Pi Pico (funções básicas de I/O, delays, etc.)

// Inclusão de arquivos de cabeçalho específicos do projeto
#include "connect_wifi.h"      // Contém a função para conectar-se a uma rede Wi-Fi
#include "server.h"            // Contém funções relacionadas à inicialização de um servidor (provavelmente HTTP ou socket)
#include "init_config.h"       // Contém funções para inicializar periféricos e configurações gerais
#include "global_manage.h"     // Pode conter variáveis globais e funções auxiliares compartilhadas
#include "led_matriz.h"           // Controle de matriz de LEDs para alertas visuais

#include "FreeRTOS.h"        // Kernel FreeRTOS
#include "task.h"            // API de criação e controle de tarefas FreeRTOS

//#include "hardware/pwm.h"       // API de PWM para controle de sinais sonoros
//#include "hardware/clocks.h"    // API de clocks do RP2040
#include "hardware/adc.h"       // API de PWM para controle de sinais sonoros
#include "hardware/pwm.h"       // API de PWM para controle de sinais sonoros
#include "hardware/clocks.h"    // API de clocks do RP2040

// buzzer.c arquivo com funções para controle do buzzer
// connect_wifi.c arquivo com funções para conexão wi-fi                            !!! edite as configurações da rede nesse arquivo !!!
// global_manage.c arquivo com funções para controle das varáveis globais
// init_config.c arquivo com funções para inicialização dos periférico
// server.c arquivo com funções relativas ao web_server
// ssd1306.c arquivo com as funções de controle de display

// Tarefa que controla o web_server
void vServerTask()
{
    int result = connect_wifi();
    if (result) return;

    start_http_server();

    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        vTaskDelay(pdMS_TO_TICKS(10));    // Esperar 100ms
    }

    cyw43_arch_deinit();
}

void vAdcTask() 
{
    adc_init_custom();
    buzzer_init();

    while (true)
    {
        int nivel = get_nivel(), max = get_max(), min = get_min();
        bool motor = get_motor();

        set_nivel((int)adc_read());

        if (nivel < min && motor==false) 
        {
            set_motor(true);

            gpio_put(GPIO_18, true);

            pwm_set_gpio_level(BUZZER_PIN, 2048);
            vTaskDelay(pdMS_TO_TICKS(900));
            pwm_set_gpio_level(BUZZER_PIN, 0);
        } 
        else if (nivel >= max && motor==true) 
        {
            set_motor(false);

            gpio_put(GPIO_18, false);

            for (int i=0;i<3;i++) 
            {
                pwm_set_gpio_level(BUZZER_PIN, 2048);
                vTaskDelay(pdMS_TO_TICKS(200));
                pwm_set_gpio_level(BUZZER_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));    // Esperar 100ms
    }
}

void vDisplayTask()
{
    bool motor_a=true, motor=false;
    ssd1306_t ssd;
    display_init(&ssd);

    while(true)
    {   
        if (motor!=motor_a) 
        {
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Controle de", centralizar_texto("Controle de"), 5);
            ssd1306_draw_string(&ssd, "Reservatorio", centralizar_texto("Reservatorio"), 15);
            ssd1306_draw_string(&ssd, "Bomba d'Agua", centralizar_texto("Bomba d'Agua"), 35);

            if(motor)
            {
                ssd1306_draw_string(&ssd, "LIGADA", centralizar_texto("LIGADA"), 45);
            }
            else
            {
                ssd1306_draw_string(&ssd, "DESLIGADA", centralizar_texto("DESLIGADA"), 45);
            }
            ssd1306_send_data(&ssd);
        }

        motor_a = motor;

        vTaskDelay(pdMS_TO_TICKS(500));

        motor = get_motor();
    }
}

void vMatrizTask(){
    bool motor_a=true, motor=false;
    PIO pio = pio0;
    uint sm = pio_init(pio);
    apagar_matriz(pio, sm);

    while(true)
    {
        if (motor!=motor_a) 
        {
            if(motor)
            {
                ligar_seta(pio, sm, 0.0, 0.0, 0.1);
            }
            else
            {
                ligar_checkmark(pio, sm, 0.0, 0.1, 0.0);
            }
        }
        
        motor_a = motor;

        vTaskDelay(pdMS_TO_TICKS(500));

        motor = get_motor();
    }
}

int main()
{
    // Inicializa todas as interfaces padrão de entrada/saída
    stdio_init_all();

    // Desativa o buzzer (define o estado como inativo)
    // desativar_buzzer();
    init_global_manage();

    // Cria tarefas com prioridades e pilhas mínimas
    xTaskCreate(vServerTask, "Server Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vAdcTask, "Adc Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vMatrizTask, "Matriz Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    // Se voltar aqui, o bootrom exibe mensagem de erro
    panic_unsupported();

    // Se tudo ocorreu bem, retorna 0 (execução bem-sucedida)
    return 0;
}
