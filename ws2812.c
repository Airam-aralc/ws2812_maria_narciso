#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define led_red 13    //Led vermelho do RGB
#define botao_A 5     // Botão A da BitDogLab
#define botao_B 6     // Botão B da BitDogLab
#define IS_RGBW false // Define se os LEDs têm um canal branco (RGBW) ou apenas RGB
#define NUM_PIXELS 25 // Total de LEDs na matriz 5x5
#define WS2812_PIN 7  //Leds da matrix 5X5
#define interval 100  //intervalo de 100 ms

static volatile uint32_t last_time = 0;  // Para debouncing, armazena o tempo do último evento
static void gpio_irq_handler(uint gpio, uint32_t events); //Prototipação da função de interrupção

bool numeros[10][NUM_PIXELS] = 
{    //NÚMERO 0
    { 0, 1, 1, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 1, 1, 0 },  
    //NÚMERO 1
    { 0, 1, 0, 0, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 0, 0, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 0, 0, 0 },
    //NÚMERO 2
    { 0, 1, 1, 1, 0,  
      0, 1, 0, 0, 0,  
      0, 1, 1, 1, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0 }, 
    //NÚMERO 3
    { 0, 1, 1, 1, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0 },
    //NÚMERO 4
    { 0, 1, 0, 0, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 0, 1, 0 }, 
    //NÚMERO 5
    { 0, 1, 1, 1, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0,  
      0, 1, 0, 0, 0,  
      0, 1, 1, 1, 0 },
    //NÚMERO 6
    { 0, 1, 1, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 1, 1, 0,  
      0, 1, 0, 0, 0,  
      0, 1, 1, 1, 0 },
    //NÚMERO 7
    { 0, 1, 0, 0, 0,  
      0, 0, 0, 1, 0,
      0, 1, 0, 0, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0 }, 
    //NÚMERO 8
    { 0, 1, 1, 1, 0,
      0, 1, 0, 1, 0,
      0, 1, 1, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 1, 1, 0 }, 
    //NÚMERO 9
    { 0, 1, 1, 1, 0,  
      0, 0, 0, 1, 0,  
      0, 1, 1, 1, 0,  
      0, 1, 0, 1, 0,  
      0, 1, 1, 1, 0 } 
};

uint32_t led_buffer[NUM_PIXELS] = {0}; // Buffer para armazenar as cores de todos os LEDs
uint8_t numero_atual = 0;              // Número atual exibido na matriz


//FUNÇÕES PARA CONTROLE DOS LEDS 
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void update_led_buffer() {//Atualiza o buffer dos Leds
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = numeros[numero_atual][i] ? urgb_u32(90, 0, 70) : 0; // (R G B) Cor roxo quase rosa
    }
}

void set_leds_from_buffer() { //Envia os dados do buffer para a matriz WS2812
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(led_buffer[i]);
    }
}


//INTERRUPÇÃO
void gpio_irq_handler(uint gpio, uint32_t events) 
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 200000) // 200 ms de debouncing
    { 
        last_time = current_time;
        
        if (gpio == botao_A) {
            numero_atual = (numero_atual + 1) % 10; // Incrementa um número e volta para 0 se o próximo for 10
        } 
        else if (gpio == botao_B) {
            numero_atual = (numero_atual + 9) % 10; // Decrementa e volta para 9 se o próximo for 0 
        }
    }
}


//FUNÇÃO PRINCIPAL
int main() 
{
    gpio_init(led_red);                 //inicializa o led vermelho
    gpio_set_dir(led_red, GPIO_OUT);   //configura como saída
    gpio_put(led_red, 0);             // Escreve uma tensão de 0 para o led iniciar apagado

    gpio_init(botao_A);               //inicializa o botão A
    gpio_set_dir(botao_A, GPIO_IN);   //configura como entrada
    gpio_pull_up(botao_A);            //pull up ativado

    gpio_init(botao_B);             //inicializa o botão B
    gpio_set_dir(botao_B, GPIO_IN);
    gpio_pull_up(botao_B);

    //Inicializa o PIO para controlar os LEDs WS2812.
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW); //taxa de transmissão em 800kHz.

    //Configura as interrupções para detectar quando os botões forem pressionados
    gpio_set_irq_enabled_with_callback(botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Obtém o tempo absoluto atual do sistema e adiciona um intervalo de tempo em microsegundos. 
    absolute_time_t next_wake_time = delayed_by_us(get_absolute_time(), interval * 1000);

    bool led_on = false; //variável boleana 

    while (1) 
    {
      //Faz o led piscar 5 vezes a cada segundo
        if(time_reached(next_wake_time))
       {
          led_on = !led_on;
          gpio_put(led_red, led_on); 
          next_wake_time = delayed_by_us(next_wake_time, interval * 1000);
       }

      //Exibir os números na matriz de leds
        update_led_buffer();
        set_leds_from_buffer();
        sleep_ms(100);
    }

    return 0;
}