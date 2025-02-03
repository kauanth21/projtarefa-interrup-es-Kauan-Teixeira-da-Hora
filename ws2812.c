#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define BTN_A 5
#define BTN_B 6
#define LED_R 13
#define LED_G 12
#define LED_B 11

volatile int numero_atual = 0; // Número exibido na matriz
volatile bool estado_led_r = false; // Estado do LED vermelho

// Prototipação da função put_pixel
static inline void put_pixel(uint32_t pixel_grb);

// Buffer da matriz 5x5
bool led_buffer[NUM_PIXELS] = {0};

// Mapeamento de números 0 a 9 em formato 5x5 
const bool numeros[10][NUM_PIXELS] = {
    {1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1, 1,0,0,0,1, 1,1,1,1,1}, // 0
    {0,1,1,1,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,0,0, 0,0,1,0,0}, // 1
    {1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,0, 0,0,0,0,1, 1,1,1,1,0}, // 2
    {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1}, // 3
    {1,0,0,0,0, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,0,0,0,1}, // 4
    {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}, // 5
    {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,0, 1,1,1,1,1}, // 6
    {1,0,0,0,0, 0,0,0,0,1, 1,0,0,0,0, 1,0,0,0,1, 1,1,1,1,1}, // 7
    {1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}, // 8
    {1,1,1,1,1, 0,0,0,0,1, 1,1,1,1,1, 1,0,0,0,1, 1,1,1,1,1}  // 9
};

// Atualiza o buffer da matriz com o número atual
void atualizar_matriz() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = numeros[numero_atual][i];
    }
}

// Envia os pixels para a matriz WS2812
void exibir_leds(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = ((uint32_t)r << 8) | ((uint32_t)g << 16) | (uint32_t)b;
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(led_buffer[i] ? color : 0);
    }
}

// Envia cor para a WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função de interrupção para os botões
void gpio_callback(uint gpio, uint32_t events) {
    static uint64_t ultimo_tempo_a = 0;
    static uint64_t ultimo_tempo_b = 0;
    uint64_t tempo_atual = time_us_64();

    if (gpio == BTN_A && tempo_atual - ultimo_tempo_a > 200000) { // Debounce de 200ms
        if (numero_atual < 9) numero_atual++;
        atualizar_matriz();
        ultimo_tempo_a = tempo_atual;
    } 
    else if (gpio == BTN_B && tempo_atual - ultimo_tempo_b > 200000) { // Debounce de 200ms
        if (numero_atual > 0) numero_atual--;
        atualizar_matriz();
        ultimo_tempo_b = tempo_atual;
    }
}

// Pisca o LED vermelho do RGB
void piscar_led_r() {
    gpio_put(LED_R, estado_led_r);
    estado_led_r = !estado_led_r;
}

int main() {
    stdio_init_all();

    // Configuração dos LEDs RGB
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_put(LED_R, 0); // Garante que o LED começa apagado

    // Configuração da matriz WS2812
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    // Configuração dos botões
    gpio_init(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_pull_up(BTN_B);

    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);

    atualizar_matriz(); // Atualiza matriz com o primeiro número

    while (1) {
        exibir_leds(0, 0, 255);  // Exibe o número em azul
        piscar_led_r(); // Pisca o LED vermelho a 5 Hz
        sleep_ms(100);
    }

    return 0;
}
