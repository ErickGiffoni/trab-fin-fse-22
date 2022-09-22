#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"

#include "wifi.h"
#include "http_client.h"
#include "mqtt.h"
#include "gpio.h"

#define TAG "main"

xSemaphoreHandle conexaoWifiSemaphore;
xSemaphoreHandle conexaoMQTTSemaphore;

float temperatura = 0.0;
float umidade     = 0.0;
float freq_buzzer = 0.0;

void conectadoWifi(void *params)
{
  while (true)
  {
    if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      // Processamento Internet
      mqtt_start();
    }
  }
}

void trataSensores(){
  temperatura = get_temperatura_gpio();
  umidade     = get_umidade_gpio();
  freq_buzzer = get_frequencia_buzzer();

  //BUZZER
  atualiza_frequencia_buzzer(temperatura);
  
  ESP_LOGI(TAG, "\nTemperatura: %f\nUmidade: %f%%\nFrequencia do buzzer: %.2f", temperatura, umidade, freq_buzzer);

  return;
}

void trataComunicacaoComServidor(void *params)
{
  char mensagem[75];
  char json[200];
  if (xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    while (true)
    {
      //  float temperatura = 20.0 + (float)rand()/(float)(RAND_MAX/10.0);
      trataSensores();

      sprintf(mensagem, "{\"temperature\": %.2f,\n\"umidade\": %.2f,\n\"frequencia_buzzer\": %.2f}", temperatura, umidade, freq_buzzer);
      // sprintf(mensagem, "{\"temperature\": %.2f,\n\"umidade\": %.2f}", temperatura, umidade);
      mqtt_envia_mensagem("v1/devices/me/telemetry", mensagem);

      sprintf(json, "{\"teste\": 123,\n\"teste2\": 456}");
      mqtt_envia_mensagem("v1/devices/me/attributes", json);

      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
  else
  {
    while(1)
    {
      trataSensores();
    }
  }
}

void app_main(void)
{
  printf("Iniciando.............\n");
  // Inicializa o NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  conexaoWifiSemaphore = xSemaphoreCreateBinary();
  conexaoMQTTSemaphore = xSemaphoreCreateBinary();
  wifi_start();

  configura_gpio();

  xTaskCreate(&conectadoWifi, "Conexão ao MQTT", 4096, NULL, 1, NULL);
  xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
  xTaskCreate(&acionaBuzzer, "Acionamento do buzzer", 4096, NULL, 1, NULL);
}
