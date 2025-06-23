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

// connect_wifi.c arquivo com funções para conexão wi-fi                            !!! edite as configurações da rede nesse arquivo !!!
// global_manage.c arquivo com funções para controle das varáveis globais
// init_config.c arquivo com funções para inicialização dos periférico
// server.c arquivo com funções relativas ao web_server
// ssd1306.c arquivo com as funções de controle de display

bool is_connected = false;

// Tarefa que controla o web_server
void vServerTask()
{
    int result = connect_wifi();
    if (result) return;

    start_http_server();

    is_connected = true;

    while (true)
    {
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        vTaskDelay(pdMS_TO_TICKS(100));    // Esperar 100ms
    }

    is_connected = false;
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

    char buffer[32];

    motor = get_motor();

    while(true)
    {
        if (is_connected)
        {
            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
            ssd1306_line(&ssd, 3, 15, 123, 15, true); // linha horizontal - primeira
            ssd1306_line(&ssd, 3, 40, 123, 40, true); // linha horizontal - segunda
            ssd1306_line(&ssd, 53, 15, 53, 40, true); // linha vertical
            ssd1306_line(&ssd, 85, 40, 85, 63, true); // linha vertical 2
            ssd1306_draw_string(&ssd, "Reservatorio", 9, 6);
            ssd1306_draw_string(&ssd, "Bomba", 9, 20);
            ssd1306_draw_string(&ssd, "Nivel", 9, 30);
            ssd1306_draw_string(&ssd, "Nivel Min", 9, 42);
            ssd1306_draw_string(&ssd, "Nivel Max", 9, 53);
            ssd1306_send_data(&ssd);

            sprintf(buffer, "%d%%", get_min());
            ssd1306_draw_string(&ssd, buffer, 90, 42);
            ssd1306_send_data(&ssd);

            sprintf(buffer, "%d%%", get_max());
            ssd1306_draw_string(&ssd, buffer, 90, 53);
            ssd1306_send_data(&ssd);

            sprintf(buffer, "%d%%", get_nivel());
            ssd1306_draw_string(&ssd, buffer, 64, 30);
            ssd1306_send_data(&ssd);

            if(motor)
            {
                ssd1306_draw_string(&ssd, "Lig", 64, 20);
            }
            else
            {
                ssd1306_draw_string(&ssd, "Desl", 64, 20);
            }
            ssd1306_send_data(&ssd);

            motor = get_motor();
        }
        else
        {
            ssd1306_fill(&ssd, false);
            ssd1306_draw_string(&ssd, "Conectando", centralizar_texto("Conectando"), 5);
            ssd1306_draw_string(&ssd, "ao WiFi", centralizar_texto("ao WiFi"), 15);
            ssd1306_draw_string(&ssd, "e criando", centralizar_texto("e criando"), 24);
            ssd1306_draw_string(&ssd, "o servidor", centralizar_texto("o servidor"), 34);
            ssd1306_send_data(&ssd);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vMatrizTask()
{
    PIO pio = pio0;
    uint sm = pio_init(pio);

    while(true)
    {
        if (is_connected)
        {
            if(get_motor())
            {
                ligar_seta(pio, sm, 0.0, 0.0, 0.1);
            }
            else
            {
                ligar_checkmark(pio, sm, 0.0, 0.1, 0.0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main()
{
    // Inicializa todas as interfaces padrão de entrada/saída
    stdio_init_all();

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
