/* Blink Example
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "dht11.h"

// #include "cJSON.h"
#include "gpio.h"
#include "mqtt.h"

#define GPIO_LED_PIN 2
#define GPIO_BOTAO_PIN 0
#define GPIO_BUZZER_PIN 13
#define GPIO_TEMPERATURA_PIN 4
#define TAG "GPIO"

xQueueHandle filaDeInterrupcao;

int estado_led = 0;
float frequencia_buzzer = 1000; // 1s

static void IRAM_ATTR gpio_isr_handler_botao(void *args)
{
    int pino = (int)args;
    xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}

void trataInterrupcaoBotao(void *params)
{
    int pino;

    while (true)
    {
#if CONFIG_LOW_POWER_ENABLE

        if (rtc_gpio_get_level(GPIO_BOTAO_PIN) == 0)
        {
            if (xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
            {
                ESP_LOGI(TAG, "Botao acionado");
                troca_gpio_led_estado();
            }
            ESP_LOGI(TAG, "Esperando botao ser solto...");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

#else

        if (xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Botao acionado");
            // troca_gpio_led_estado();
        }

#endif
    }
}

float get_frequencia_buzzer(){
    return frequencia_buzzer;
}

void acionaBuzzer(){
    while (1)
    {
        gpio_set_level(GPIO_BUZZER_PIN, 1);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_BUZZER_PIN, 0);
        vTaskDelay(frequencia_buzzer / portTICK_PERIOD_MS);
    }
}

void atualiza_frequencia_buzzer(float temperatura)
{
    if (temperatura <= 15)
    {
        // gpio_set_level(GPIO_BUZZER_PIN, 1);
        frequencia_buzzer = 2000; // 2s
        ESP_LOGI(TAG, "Buzzer em 2s\n");
    }
    else if (temperatura >= 75)
    {
        frequencia_buzzer = 200; // 0.2s
        ESP_LOGI(TAG, "Buzzer em 0.2s\n");
    }
    else
    {
        frequencia_buzzer = 2000 - (((temperatura/5.0) - 3) * 150);
        ESP_LOGI(TAG, "Buzzer em %.2fs\n", frequencia_buzzer);
    }
    return;
}

void configura_gpio()
{
    gpio_pad_select_gpio(GPIO_LED_PIN);
    gpio_set_direction(GPIO_LED_PIN, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(GPIO_BOTAO_PIN);
    gpio_set_direction(GPIO_BOTAO_PIN, GPIO_MODE_INPUT);

    gpio_pulldown_en(GPIO_BOTAO_PIN);
    gpio_pullup_dis(GPIO_BOTAO_PIN);

    gpio_set_intr_type(GPIO_BOTAO_PIN, GPIO_INTR_POSEDGE);

    DHT11_init(GPIO_TEMPERATURA_PIN);

    // BUZZER
    gpio_pad_select_gpio(GPIO_BUZZER_PIN);
    gpio_set_direction(GPIO_BUZZER_PIN, GPIO_MODE_OUTPUT);

#if CONFIG_LOW_POWER_ENABLE

    gpio_wakeup_enable(GPIO_BOTAO_PIN, GPIO_INTR_LOW_LEVEL);

#endif

    filaDeInterrupcao = xQueueCreate(10, sizeof(int));
    xTaskCreate(trataInterrupcaoBotao, "TrataBotao", 4096, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BOTAO_PIN, gpio_isr_handler_botao, (void *)GPIO_BOTAO_PIN);

    gpio_set_level(GPIO_LED_PIN, estado_led);
}

void piscaLed(){
    while (1)
    {
        troca_gpio_led_estado();
    }
}

void troca_gpio_led_estado()
{
    estado_led = !estado_led;
    gpio_set_level(GPIO_LED_PIN, estado_led);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // mqtt_envia_estado_botao(estado_led);
}

int get_temperatura_gpio()
{
    struct dht11_reading tmp = DHT11_read();
    return tmp.temperature;
}

int get_umidade_gpio()
{
    struct dht11_reading tmp = DHT11_read();
    return tmp.humidity;
}