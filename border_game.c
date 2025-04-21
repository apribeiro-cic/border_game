#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/numbers.h"
#include "border_game.pio.h"

#define I2C_PORT i2c1 // Define o barramento I2C
#define I2C_SDA 14 // Define o pino SDA
#define I2C_SCL 15 // Define o pino SCL
#define endereco 0x3C // Endereço do display OLED

#define buzzer_a_pin 10 // Buzzer A
#define buzzer_b_pin 21 // Buzzer B

#define num_pixels 25 // Número de pixels da matriz de LEDs
#define matrix_pin 7 // Pino da matriz de LEDs

const uint led_pin_red = 13; // LED vermelho
const uint led_pin_blue = 12; // LED azul
const uint led_pin_green = 11;  // LED verde

const uint joystick_x_pin = 27; // Pino do eixo X do joystick
const uint joystick_y_pin = 26; // Pino do eixo Y do joystick

const uint btn_j = 22; // Pino do botão do joystick
const uint btn_a = 5; // Pino do botão A
const uint btn_b = 6; // Pino do botão B

uint32_t last_time = 0; // Variável para armazenar o tempo do último evento para o debouncing

static uint8_t current_screen = 0; // Variável para armazenar a borda atual

static volatile uint pontos = 0; // Variável para armazenar os pontos

char buffer[32]; // Buffer para armazenar valores como string

// Variáveis para controle de cor e ícone exibido na matriz de LEDs
double red = 255.0, green = 255.0 , blue = 255.0; // Variáveis para controle de cor
int number = 0; //Armazena o número atualmente exibido
double* numbers[16] = {number_zero, number_one, number_two, number_three, number_four, number_five, number_six, number_seven, number_eight, number_nine, number_ten, number_eleven, number_twelve, number_thirteen, number_fourteen, number_fifteen}; //Ponteiros para os desenhos dos números

static volatile uint16_t border_x_max = 128; // Borda máxima do eixo X
static volatile uint16_t border_y_max = 64; // Borda máxima do eixo Y

static volatile uint16_t border_x = 32; // Borda máxima do eixo X
static volatile uint16_t border_y = 32; // Borda máxima do eixo Y

static volatile uint16_t border_x_start = 0; // Borda inicial do eixo X
static volatile uint16_t border_y_start = 0; // Borda inicial do eixo Y

static volatile uint16_t interval_ms = 1000; // Intervalo de tempo para o jogo

ssd1306_t ssd; // Estrutura para o display OLED

// Função para configurar as GPIOs
void setup_GPIOs() { 
    // Inicializa os LEDs e define como saída
    gpio_init(led_pin_red); 
    gpio_set_dir(led_pin_red, GPIO_OUT);
    gpio_init(led_pin_blue);
    gpio_set_dir(led_pin_blue, GPIO_OUT);
    gpio_init(led_pin_green);
    gpio_set_dir(led_pin_green, GPIO_OUT);
    
    // Inicializa os botões e define como entrada
    gpio_init(btn_a);
    gpio_set_dir(btn_a, GPIO_IN);
    gpio_pull_up(btn_a);

    gpio_init(btn_b);
    gpio_set_dir(btn_b, GPIO_IN);
    gpio_pull_up(btn_b);

    gpio_init(btn_j);
    gpio_set_dir(btn_j, GPIO_IN);
    gpio_pull_up(btn_j);

    gpio_init(buzzer_a_pin);  
    gpio_set_dir(buzzer_a_pin, GPIO_OUT);
    gpio_init(buzzer_b_pin);  
    gpio_set_dir(buzzer_b_pin, GPIO_OUT);
}

// Função para configurar o PWM
void pwm_setup_gpio(uint gpio, uint freq) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);  // Define o pino como saída PWM
    uint slice_num = pwm_gpio_to_slice_num(gpio);  // Obtém o slice do PWM

    if (freq == 0) {
        pwm_set_enabled(slice_num, false);  // Desabilita o PWM
        gpio_put(gpio, 0);  // Desliga o pino
    } else {
        uint32_t clock_div = 4; // Define o divisor do clock
        uint32_t wrap = (clock_get_hz(clk_sys) / (clock_div * freq)) - 1; // Calcula o valor de wrap

        // Configurações do PWM (clock_div, wrap e duty cycle) e habilita o PWM
        pwm_set_clkdiv(slice_num, clock_div); 
        pwm_set_wrap(slice_num, wrap);  
        pwm_set_gpio_level(gpio, wrap / 5);  
        pwm_set_enabled(slice_num, true);  
    }
}

// Função para mapear ADC (0-4095) para coordenadas do display (0-128, 0-64)
int map_adc_to_display(int adc_value, int max_adc, int max_display) {
    return (adc_value * max_display) / max_adc;
}

// Gera bordas aleatórias para o display
void generate_borders() {
    border_x_start = rand() % (128 - 32);
    border_y_start = rand() % (64 - 32);
}

// Função de callback para tratamento de interrupção dos botões
void gpio_irq_handler(uint gpio, uint32_t events) { 
    uint32_t current_time = to_us_since_boot(get_absolute_time()); // Pega o tempo atual em ms
    if (current_time - last_time > 250000) { // Debouncing de 250ms
        last_time = current_time;
        if (gpio == btn_a) { // Verifica se o botão A foi pressionado 
            printf("Botão A pressionado!\n");
            switch (current_screen) {
                case 0: // Se estiver na tela inicial, inicia o jogo
                    current_screen = 1; // Muda para a tela de jogo
                    break;
                case 1: // Se estiver jogando, não faz nada
                    break;
                case 2: // Se estiver na tela de game over, reinicia o jogo
                    current_screen = 0; // Muda para a tela inicial
                    break;
                case 3: // Se estiver na tela de vitória, reinicia o jogo
                    current_screen = 0; // Muda para a tela inicial
                    break;
            }
        } else if (gpio == btn_b) { // Verifica se o botão B foi pressionado e envia feedback para UART
            printf("Botão B pressionado!\n");
        } else if (gpio == btn_j) { // Verifica se o botão do joystick foi pressionado e entra no modo bootsel
            printf("Botão do joystick pressionado!\n");
            reset_usb_boot(0, 0);
        }
    }
}

// Desenha a tela atual no display e altera leds
void draw_screen(uint16_t adc_value_x, uint16_t adc_value_y, int display_x, int display_y) {
    ssd1306_fill(&ssd, false); // Limpa o display

    switch(current_screen) {
        case 0:
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); 
            ssd1306_draw_string(&ssd, "PRESSIONE", 28, 24); 
            ssd1306_draw_string(&ssd, "BOTAO A", 36, 34); 
            turn_off_leds(led_pin_blue, false); // Desliga todos os LEDs exceto o azul
            break;
        case 1:
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_rect(&ssd, display_y, display_x, 8, 8, true, true); // Desenha um retângulo preenchido
            ssd1306_rect(&ssd, border_y_start, border_x_start, border_x, border_y, true, false); // Desenha um retângulo preenchido         
            turn_off_leds(0, true); // Desliga todos os LEDs   
            break;
        case 2:
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); 
            ssd1306_draw_string(&ssd, "GAME OVER", 29, 6); 
            ssd1306_draw_string(&ssd, "PONTUACAO", 4, 18); 
            sprintf(buffer, "%d", pontos); 
            ssd1306_draw_string(&ssd, buffer, 4, 28); 
            ssd1306_draw_string(&ssd, "PRESSIONE", 28, 42); 
            ssd1306_draw_string(&ssd, "BOTAO A", 36, 52); 
            turn_off_leds(led_pin_red, false); // Desliga todos os LEDs exceto o vermelho
            break;
        case 3:
            ssd1306_rect(&ssd, 0, 0, 128, 64, true, false); 
            ssd1306_draw_string(&ssd, "YOU WIN", 35, 6); 
            ssd1306_draw_string(&ssd, "PONTUACAO", 4, 18); 
            sprintf(buffer, "%d", pontos); 
            ssd1306_draw_string(&ssd, buffer, 4, 28); 
            ssd1306_draw_string(&ssd, "PRESSIONE", 28, 42); 
            ssd1306_draw_string(&ssd, "BOTAO A", 36, 52); 
            turn_off_leds(led_pin_green, false); // Desliga todos os LEDs exceto o verde
            break;
    }
}

// Rotina para definição da intensidade de cores do led
uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R, G, B;
    R = r * red;
    G = g * green;
    B = b * blue;
    return (G << 24) | (R << 16) | (B << 8);
}

// Rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b) {
    for (int16_t i = 0; i < num_pixels; i++) {
        valor_led = matrix_rgb(desenho[24-i], desenho[24-i], desenho[24-i]);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Função para desligar os LEDs (Todos ou todos menos um específico)
void turn_off_leds(uint gpio, bool turn_off_all) {
    if (turn_off_all) {
        gpio_put(led_pin_red, 0);
        gpio_put(led_pin_blue, 0);
        gpio_put(led_pin_green, 0);
    } else {
        gpio_put(led_pin_red, gpio == led_pin_red ? 1 : 0);
        gpio_put(led_pin_blue, gpio == led_pin_blue ? 1 : 0);
        gpio_put(led_pin_green, gpio == led_pin_green ? 1 : 0);
    }
}

int main()
{
    stdio_init_all();

    setup_GPIOs(); // Configura os GPIOs

    // Configura as interrupções para os botões
    gpio_set_irq_enabled_with_callback(btn_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(btn_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(btn_j, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa o ADC e configura os pinos do joystick
    adc_init();
    adc_gpio_init(joystick_x_pin);
    adc_gpio_init(joystick_y_pin);

    // Inicializa o I2C e configura os pinos SDA e SCL para o display OLED 
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    //Configurações da PIO
    PIO pio = pio0; 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, matrix_pin);

    // Inicializa e configura o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
    ssd1306_config(&ssd);

    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    uint16_t adc_value_x = 0; // Variável para armazenar o valor do ADC do eixo X
    uint16_t adc_value_y = 0; // Variável para armazenar o valor do ADC do eixo Y

    srand(time(0));

    while (true) {    
        // Lê os valores do ADC dos eixos X e Y
        adc_select_input(1);
        adc_value_x = adc_read();
        adc_select_input(0);
        adc_value_y = adc_read();

        uint32_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual

        static bool buzzer_on = false; // Variável para controlar o estado do buzzer
        static uint32_t last_beep_time = 0; // Armazena o tempo do último beep

        // Mapeia os valores do ADC para as coordenadas do display considerando o tamanho do quadrado (8x8) e a borda
        int display_x = map_adc_to_display(adc_value_x, 4095, border_x_max - 8) + (128 - border_x_max) / 2; // Centraliza o display e garante que o retângulo não ultrapasse a borda
        int display_y = border_y_max - 8 - map_adc_to_display(adc_value_y, 4095, border_y_max - 8) + (64 - border_y_max) / 2; // Centraliza o display e garante que o retângulo não ultrapasse a borda máxima do display, invertendo o eixo Y

        draw_screen(adc_value_x, adc_value_y, display_x, display_y); // Desenha a tela atual

        // Verifica se está na tela de jogo
        if (current_screen == 1) {
            static int count = 0; // Contador para o número de beeps
            if (!buzzer_on && (current_time - last_beep_time >= interval_ms)) { // Verifica se o buzzer está desligado e se o intervalo de tempo foi atingido
                last_beep_time = current_time;
                buzzer_on = true;

                if (count == 3) { // Se o contador atingir 3, toca o buzzer e avança para a próxima borda
                    pwm_setup_gpio(buzzer_a_pin, 1000); // Liga o buzzer 
                    count = 0;

                    // Verifica se o quadrado está dentro da borda
                    if (border_x_start < display_x && display_x < border_x_start + 32 && border_y_start < display_y && display_y < border_y_start + 32) {
                        generate_borders(); // Alterna entre as bordas
                        pontos++; // Incrementa os pontos
                        number++; // Incrementa o número exibido

                        if (pontos == 15) { // Se atingir 15 pontos, toca o buzzer e avança para a tela de vitória
                            pwm_setup_gpio(buzzer_a_pin,250); 
                            sleep_ms(500); // Aguarda 1 segundo
                            pwm_setup_gpio(buzzer_a_pin,500); 
                            sleep_ms(500); // Aguarda 1 segundo
                            pwm_setup_gpio(buzzer_a_pin,750); 
                            sleep_ms(500); // Aguarda 1 segundo
                            pwm_setup_gpio(buzzer_a_pin,1000); 
                            current_screen = 3; // Tela de vitória
                            pwm_setup_gpio(buzzer_a_pin, 0); // Desliga o buzzer
                        }
                        printf("Pontuacao: %d\n", pontos); // Imprime os pontos
                    } else { // Se o quadrado não estiver dentro da borda, toca o buzzer e avança para a tela de game over
                        pwm_setup_gpio(buzzer_a_pin,750); 
                        sleep_ms(500); // Aguarda 1 segundo
                        pwm_setup_gpio(buzzer_a_pin,500); 
                        sleep_ms(500); // Aguarda 1 segundo
                        pwm_setup_gpio(buzzer_a_pin,250); 
                        sleep_ms(500); // Aguarda 1 segundo
                        pwm_setup_gpio(buzzer_a_pin,0); // Desliga o buzzer
                        current_screen++;
                    }
                    // Diminui o intervalo de tempo e a largura da borda
                    if (interval_ms > 50) {
                        interval_ms -= 75; // Diminui o intervalo de tempo
                    }
                    // Diminui a largura e altura da borda
                    if (border_x > 16 && border_y > 16) {
                        border_x -= 2; // Diminui a largura da borda
                        border_y -= 2; // Diminui a altura da borda
                    } 
                } else { // Se o contador não atingir 3, toca o buzzer e incrementa o contador
                    pwm_setup_gpio(buzzer_a_pin, 500); // Desliga o buzzer
                    count++;
                } 
            }

            // Desliga o buzzer após 100ms
            if (buzzer_on && (current_time - last_beep_time >= 100)) {
                buzzer_on = false;
                pwm_setup_gpio(buzzer_a_pin, 0); // Desliga o buzzer após 100ms
            }
        }

        // Verifica se o jogo terminou
        if (current_screen == 0) {
            pwm_setup_gpio(buzzer_a_pin, 0); // Desliga o buzzer
            interval_ms = 1000; // Reseta o intervalo de tempo
            border_x = 32; // Reseta a largura da borda
            border_y = 32; // Reseta a altura da borda
            pontos = 0; // Reseta os pontos
            number = 0;
        }

        desenho_pio(numbers[number], 0, pio, sm, red, green, blue); // Desenha o ícone na matriz de LEDs

        printf("X: %d, Y: %d\n", display_x, display_y); // Imprime os valores do joystick

        ssd1306_send_data(&ssd); // Envia os dados para o display    

        sleep_ms(50); // Aguarda 50ms para o próximo loop
    }
}
