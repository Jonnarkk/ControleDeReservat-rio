#include <stdio.h>             // Biblioteca padrão de entrada/saída (ex: printf)
#include <string.h>            // Biblioteca para manipulação de strings
#include "pico/stdlib.h"       // Biblioteca do SDK do Raspberry Pi Pico (funções básicas de I/O, delays, etc.)

// Inclusão de arquivos de cabeçalho específicos do projeto
#include "connect_wifi.h"      // Contém a função para conectar-se a uma rede Wi-Fi
#include "server.h"            // Contém funções relacionadas à inicialização de um servidor (provavelmente HTTP ou socket)
//#include "init_config.h"       // Contém funções para inicializar periféricos e configurações gerais
#include "global_manage.h"     // Pode conter variáveis globais e funções auxiliares compartilhadas
//#include "matriz.h"           // Controle de matriz de LEDs para alertas visuais

#include "FreeRTOS.h"        // Kernel FreeRTOS
#include "task.h"            // API de criação e controle de tarefas FreeRTOS

//#include "hardware/pwm.h"       // API de PWM para controle de sinais sonoros
//#include "hardware/clocks.h"    // API de clocks do RP2040
#include "hardware/adc.h"       // API de PWM para controle de sinais sonoros

#define ADC_CHANNEL 0
#define ADC_PIN (26 + ADC_CHANNEL)

// buzzer.c arquivo com funções para controle do buzzer
// connect_wifi.c arquivo com funções para conexão wi-fi                            !!! edite as configurações da rede nesse arquivo !!!
// global_manage.c arquivo com funções para controle das varáveis globais
// init_config.c arquivo com funções para inicialização dos periférico
// server.c arquivo com funções relativas ao web_server
// ssd1306.c arquivo com as funções de controle de display

// Tarefa que controla o web_server
void vServerTask()
{
    // Conecta ao Wi-Fi; se falhar, encerra o programa retornando 
    int result = connect_wifi();
    if (result) return;

    start_http_server();

    while (true)
    {
        //printf("Task wi-fi rodando\n");
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        //vTaskDelay(pdMS_TO_TICKS(100));    // Esperar 100ms
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
}

// Tarefa que controla o display
// void vDisplayTask()
// {
//     // Inicializa o display (OLED/SSD1306)
//     display_init(get_ssd_pointer());

//     while (true)
//     {
//         desenhar_prioridade(get_ssd_pointer()); // Atualiza o display com o novo estado de prioridade
//         vTaskDelay(pdMS_TO_TICKS(100));    // Esperar 100ms
//     }
// }

// Tarefa que controla o buzzer e o led rgb
// void vBuzzerTask()
// {
//     // Inicializa o buzzer
//     buzzer_init();
//     // Inicializa os pinos GPIO dos leds
//     init_pins();

//     while (true)
//     {
//         if (get_buzzer()==BUZZER_ATIVADO) // se o o buzzer estiver ativo aciona o buzzer e o led rgb
//         {
//             //gpio_put(RED_PIN, 0);
//             gpio_put(BLUE_PIN, 1); // acende led azul
//             pwm_set_gpio_level(BUZZER_PIN, 2048);  // ativa buzzer
//             vTaskDelay(pdMS_TO_TICKS(100));    // Esperar 100ms  

//             gpio_put(BLUE_PIN, 0); // apaga led azul
//             gpio_put(RED_PIN, 1); // acende led vermelho
//             pwm_set_gpio_level(BUZZER_PIN, 0);  // desativa buzzer
//             vTaskDelay(pdMS_TO_TICKS(100));    // Esperar 100ms 
//             gpio_put(RED_PIN, 0);
//         }
//     }
// }

// Tarefa que controla a matriz
// void vMatrizTask()
// {
//     PIO pio = pio0;
//     uint sm = pio_init(pio); // Inicializa State Machine para PIO
    
//     while (true)
//     {
//         if (get_buzzer()==BUZZER_ATIVADO) // se o o buzzer estiver ativo a matriz também deve ser ativada
//         {
//             desenhar_alerta(pio, sm);  // desenha uma exclamação na matriz
//             vTaskDelay(pdMS_TO_TICKS(200));    // Esperar 100ms  

//             apagar_matriz(pio, sm);  // apaga a matriz
//             vTaskDelay(pdMS_TO_TICKS(200));    // Esperar 100ms 
//         }
//     }
// }

void vAdcTask() 
{
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    while (true)
    {
        set_nivel(adc_read());
        printf("Task adc rodando: %d\nAdc: %d\n", adc_read(), get_nivel());
        vTaskDelay(pdMS_TO_TICKS(500));    // Esperar 100ms
    }
}

int main()
{
    // Inicializa todas as interfaces padrão de entrada/saída
    stdio_init_all();

    // Desativa o buzzer (define o estado como inativo)
    // desativar_buzzer();
    set_max(100);
    set_min(0);
    set_nivel(0);
    set_motor(false);

    // Cria tarefas com prioridades e pilhas mínimas
    xTaskCreate(vServerTask, "Server Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(vMatrizTask, "Matriz Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vAdcTask, "Adc Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    // Se voltar aqui, o bootrom exibe mensagem de erro
    panic_unsupported();

    // Se tudo ocorreu bem, retorna 0 (execução bem-sucedida)
    return 0;
}
