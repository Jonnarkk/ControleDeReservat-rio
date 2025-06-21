/* ATENÇÃO: MODIFICAR O SSID E SENHA NO ARQUIVO CONNECT_WIFI.C E O PATH DO FREERTOS NO CMAKELIST!! */

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
#include "ssd1306.h"         // Biblioteca para controle do display OLED
#include "led_matriz.h"      // Bibiloteca para controle da matriz de LED's
#include "pico/bootrom.h"    // Biblioteca para utilizar bootsel no botão B

//#include "hardware/pwm.h"       // API de PWM para controle de sinais sonoros
//#include "hardware/clocks.h"    // API de clocks do RP2040
#include "hardware/adc.h"       // API de PWM para controle de sinais sonoros

#define ADC_CHANNEL 0
#define ADC_PIN (26 + ADC_CHANNEL)
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define BOTAOB 6

// Variáveis Globais
ssd1306_t ssd;

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
        vTaskDelay(pdMS_TO_TICKS(10));    // Esperar 100ms
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
}

void vDisplayTask(){
    // Inicialização do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    while(true){
        bool motor = get_motor();
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "Controle de", centralizar_texto("Controle de"), 5);
        ssd1306_draw_string(&ssd, "Reservatorio", centralizar_texto("Reservatorio"), 15);
        ssd1306_draw_string(&ssd, "Bomba d'Agua", centralizar_texto("Bomba d'Agua"), 35);

        if(motor){
            ssd1306_draw_string(&ssd, "LIGADA", centralizar_texto("LIGADA"), 45);
        }
        else{
            ssd1306_draw_string(&ssd, "DESLIGADA", centralizar_texto("DESLIGADA"), 45);
        }
        ssd1306_send_data(&ssd);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vMatrizTask(){
    PIO pio = pio0;
    uint sm = pio_init(pio);

    while(true){
        bool motor = get_motor();
        if(motor){
            ligar_seta(pio, sm, 0.0, 0.0, 0.1);
        }
        else{
            ligar_checkmark(pio, sm, 0.0, 0.1, 0.0);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vAdcTask() 
{
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(ADC_CHANNEL);

    while (true)
    {
        int nivel = get_nivel(), max = get_max(), min = get_min();
        bool motor = get_motor();

        set_nivel((int)adc_read());

        if (nivel < min && motor==false) set_motor(true); 
        else if (nivel > max && motor==true) set_motor(false);


        // printf("Task adc rodando: %d\nAdc: %d\n", adc_read(), get_nivel());
        vTaskDelay(pdMS_TO_TICKS(500));    // Esperar 100ms
    }
}

void gpio_irq_handler(uint gpio, uint32_t events){
    PIO pio = pio0;
    uint sm = pio_init(pio);

    apagar_matriz(pio, sm);
    
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    reset_usb_boot(0, 0);
}

int main()
{
    // Inicializa todas as interfaces padrão de entrada/saída
    stdio_init_all();
    gpio_init(BOTAOB);
    gpio_set_dir(BOTAOB, GPIO_IN);
    gpio_pull_up(BOTAOB);
    gpio_set_irq_enabled_with_callback(BOTAOB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Desativa o buzzer (define o estado como inativo)
    // desativar_buzzer();
    set_max(100);
    set_min(0);
    set_nivel(0);
    set_motor(false);

    // Cria tarefas com prioridades e pilhas mínimas
    xTaskCreate(vServerTask, "Server Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Display Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    // xTaskCreate(vBuzzerTask, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vMatrizTask, "Matriz Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vAdcTask, "Adc Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    // Se voltar aqui, o bootrom exibe mensagem de erro
    panic_unsupported();

    // Se tudo ocorreu bem, retorna 0 (execução bem-sucedida)
    return 0;
}
