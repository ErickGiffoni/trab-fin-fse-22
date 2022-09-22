#ifndef GPIO_H
#define GPIO_H

void configura_gpio();
int get_temperatura_gpio();
int get_umidade_gpio();
void troca_gpio_led_estado();
void atualiza_frequencia_buzzer(float temperatura);
void acionaBuzzer();
float get_frequencia_buzzer();

#endif