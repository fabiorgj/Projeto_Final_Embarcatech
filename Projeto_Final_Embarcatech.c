#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

// Arquivo .pio
#include "pio_matrix.pio.h"

// Definições
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BOT_A 5
#define BOT_B 6
#define BOT_JOY 22
#define LED_COUNT 25
#define LED_PIN 7
#define SENSOR_TEMPERATURA 27
#define SENSOR_CHUVA 26
#define LIMITE_CHUVA 1000
#define LIMITE_SUP_TEMP 38
#define LIMITE_INF_TEMP 15
#define TEMPO_DE_ESPERA 15000
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C // Endereço I2C do displa
#define BUZZER_PIN 21
#define BUZZER_FREQUENCY 2000

static volatile uint32_t tempo_anterior = 0;
static volatile uint32_t tempo_anterior_espera = 0;
static PIO pio;
static uint sm;
bool flag_A = 0;
bool flag_B = 0;
bool flag_bot_emergencia = 0;
bool flag_manual = 0;
bool flag_acionamento_manual = 0;
bool flag_status_cobertura = 0; // 1 = cobertura fechada, 0 = cobertura aberta
bool flag_bot_joy = 0;
bool flag_deslocamento = 0; // 1 = está em deslocamento, 0 = parado
bool flag_temporizador_sensores = 0;

int leitura_joyX = 0;
int leitura_joyY = 0;
float temperatura = 0;
int chuva = 0;

ssd1306_t ssd;   // Estrutura do display
bool cor = true; // Cor de fundo do display

// Matriz multidimensional com os números a serem exibidos
int matriz_numeros[8][5][5][3] = {

    {// fullDark
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},

    {// X
     {{0, 0, 255}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 255}},
     {{0, 0, 0}, {0, 0, 255}, {0, 0, 0}, {0, 0, 255}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 255}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 255}, {0, 0, 0}, {0, 0, 255}, {0, 0, 0}},
     {{0, 0, 255}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 255}}},

    {//<
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}}},

    {// fullR
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}}},

    {// Aberta
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}}},

    {//>
     {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},

    {// fullG
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}}},

    {// Fechada
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}}}};

// Função para definir a intensidade de cores do LED
// A função recebe os canais na ordem rgb, mas retorna na ordem GRB, necessárias para o correto funcionamentos dos LEDs ws2812.
static uint32_t matrix_rgb(double r, double g, double b)
{
    // Multiplique pelos fatores desejados para atenuar o brilho.
    // Os valores resultantes são convertidos para unsigned char.
    unsigned char R = (unsigned char)(r * 0.025);
    unsigned char G = (unsigned char)(g * 0.025);
    unsigned char B = (unsigned char)(b * 0.025);
    return ((uint32_t)G << 24) | ((uint32_t)R << 16) | ((uint32_t)B << 8);
}

// Rotina que desenha um número na matriz de LEDs utilizando PIO
void desenho_pio(int sprite, PIO pio, uint sm)
{
    uint32_t valor_led;
    // Percorre cada linha e coluna de cada número
    for (int linha = 4; linha >= 0; linha--) // Percorre as linhas de baixo para cima. A lógica foi invertida somente para inverter a visualização na bitdoglab
    {
        for (int coluna = 0; coluna < 5; coluna++)
        {
            if (linha % 2 != 0)
            { // Se a linha for par, desenha da direita para a esquerda. A lógica foi invertida somente para inverter a visualização na bitdoglab
                valor_led = matrix_rgb(
                    matriz_numeros[sprite][linha][coluna][0],
                    matriz_numeros[sprite][linha][coluna][1],
                    matriz_numeros[sprite][linha][coluna][2]);
                pio_sm_put_blocking(pio, sm, valor_led);
            }
            else
            { // Se a linha for ímpar, desenha da direita para a esquerda
                valor_led = matrix_rgb(
                    matriz_numeros[sprite][linha][4 - coluna][0],
                    matriz_numeros[sprite][linha][4 - coluna][1],
                    matriz_numeros[sprite][linha][4 - coluna][2]);
                pio_sm_put_blocking(pio, sm, valor_led);
            }
        }
    }
}

void init_buzzer()
{
    // Configurar o pino como saída de PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void controle_deslocamento(int chamada) // 0 = chama para fechar ; 1 = chama para abrir; 2 = chama emergência; 3 = chama contrle manual
{
    switch (chamada)
    {
    case 0: // chamada para fechar
        gpio_put(LED_G, 1);
        flag_deslocamento = 1;
        printf("Chamada para fechar\n");
        ssd1306_fill(&ssd, !cor); // Limpa o displa
        ssd1306_draw_string(&ssd, "FECHANDO", 20, 25);
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2500);
        while (flag_B == 0)
        {
            pwm_set_gpio_level(BUZZER_PIN, 500);
            desenho_pio(5, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(250);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            desenho_pio(0, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(250);
            if (flag_bot_emergencia == 1)
            {
                printf("Chamada para emergência\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displa
                ssd1306_draw_string(&ssd, "EMERGENCIA", 20, 25);
                ssd1306_send_data(&ssd); // Atualiza o display
                gpio_put(LED_G, 0);
                //flag_deslocamento = 0;
                while (flag_bot_emergencia == 1)
                {
                    pwm_set_gpio_level(BUZZER_PIN, 500);
                    desenho_pio(0, pio, sm); // Chama a função para desenhar na matriz de LEDs
                    sleep_ms(100);
                    pwm_set_gpio_level(BUZZER_PIN, 0);
                    desenho_pio(1, pio, sm); // Chama a função para desenhar na matriz de LEDs
                    sleep_ms(100);
                }
                gpio_put(LED_G, 1);
                flag_deslocamento = 1;
                printf("Saiu da emergência\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displa
                ssd1306_draw_string(&ssd, "EMERGENCIA OFF", 5, 25);
                ssd1306_send_data(&ssd); // Atualiza o display
                sleep_ms(2500);
            }
        }
        flag_B = 0;
        for (int i = 0; i < 3; i++)
        {
            pwm_set_gpio_level(BUZZER_PIN, 500);
            desenho_pio(6, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(150);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            desenho_pio(0, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(150);
        }
        desenho_pio(7, pio, sm); // Chama a função para desenhar na matriz de LEDs
        flag_status_cobertura = 1;
        flag_deslocamento = 0;
        sleep_ms(2500);
        gpio_put(LED_G, 0);
        printf("Deslocamento encerrado\n");
        ssd1306_fill(&ssd, !cor); // Limpa o displa
        ssd1306_draw_string(&ssd, "DESLOCAMENTO", 20, 25);
        ssd1306_draw_string(&ssd, "ENCERRADO", 20, 40);
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2500);
        break;
    case 1: // chamada para abrir
        gpio_put(LED_R, 1);
        flag_deslocamento = 1;
        printf("Chamada para abrir\n");
        ssd1306_fill(&ssd, !cor); // Limpa o displaY
        ssd1306_draw_string(&ssd, "ABRINDO", 20, 25);
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2500);
        while (flag_A == 0)
        {
            pwm_set_gpio_level(BUZZER_PIN, 500);
            desenho_pio(2, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(250);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            desenho_pio(0, pio, sm);
            sleep_ms(250);
            if (flag_bot_emergencia == 1)
            {
                printf("Chamada para emergência\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displaY
                ssd1306_draw_string(&ssd, "EMERGENCIA", 20, 25);
                ssd1306_send_data(&ssd); // Atualiza o display
                gpio_put(LED_R, 0);
                //flag_deslocamento = 0;
                while (flag_bot_emergencia == 1)
                {
                    pwm_set_gpio_level(BUZZER_PIN, 500);
                    desenho_pio(0, pio, sm); // Chama a função para desenhar na matriz de LEDs
                    sleep_ms(100);
                    pwm_set_gpio_level(BUZZER_PIN, 0);
                    desenho_pio(1, pio, sm); // Chama a função para desenhar na matriz de LEDs
                    sleep_ms(100);
                }
                gpio_put(LED_R, 1);
                flag_deslocamento = 1;
                printf("Saiu da emergência\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displaY
                ssd1306_draw_string(&ssd, "EMERGENCIA OFF", 5, 25);
                ssd1306_send_data(&ssd); // Atualiza o display
                sleep_ms(2500);
            }
        }
        flag_A = 0;
        for (int i = 0; i < 3; i++)
        {
            pwm_set_gpio_level(BUZZER_PIN, 500);
            desenho_pio(3, pio, sm); // Chama a função para desenhar na matriz de LEDs
            sleep_ms(150);
            pwm_set_gpio_level(BUZZER_PIN, 0);
            desenho_pio(0, pio, sm);
            sleep_ms(150);
        }
        desenho_pio(4, pio, sm);
        flag_status_cobertura = 0;
        flag_deslocamento = 0;
        sleep_ms(2500);
        gpio_put(LED_R, 0);
        printf("Deslocamento encerrado\n");
        ssd1306_fill(&ssd, !cor); // Limpa o displaY
        ssd1306_draw_string(&ssd, "DESLOCAMENTO", 20, 25);
        ssd1306_draw_string(&ssd, "ENCERRADO", 20, 40);
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(2500);
        break;
    }
}

void inicializa_GPIOs(void)
{
    // Inicializa os GPIOs para os LEDs e botões
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_init(BOT_A);
    gpio_init(BOT_B);
    gpio_init(BOT_JOY);

    // Seta os GPIO's como saída para os LEDs
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);

    // Seta os GPIO's como entrada para os botões
    gpio_set_dir(BOT_A, GPIO_IN);
    gpio_set_dir(BOT_B, GPIO_IN);
    gpio_set_dir(BOT_JOY, GPIO_IN);

    // Seta os GPIO's com pull-up para os botões
    gpio_pull_up(BOT_A);
    gpio_pull_up(BOT_B);
    gpio_pull_up(BOT_JOY);

    // Inicializa o ADC
    adc_gpio_init(SENSOR_TEMPERATURA);
    adc_gpio_init(SENSOR_CHUVA);

    // Coloca os LEDs como desligados
    gpio_put(LED_R, 0);
    gpio_put(LED_G, 0);
    gpio_put(LED_B, 0);
}

// Função de callback para os botões, que altera o número a ser exibido
void call_back_dos_botoes(uint gpio, uint32_t events)
{
    uint32_t tempo_agora = to_ms_since_boot(get_absolute_time()); // Pega o tempo atual em milissegundos para debounce
    if (tempo_agora - tempo_anterior > 250)
    {
        tempo_anterior = tempo_agora; // Atualiza o tempo anterior
        if (gpio == BOT_A)
        {
            if (flag_status_cobertura == 1 && flag_deslocamento == 1 && flag_bot_emergencia == 0)
            {
                printf("Acionou A\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displaY
                ssd1306_draw_string(&ssd, "FIM DE CURSO", 20, 25);
                ssd1306_draw_string(&ssd, "DETECTADO", 20, 40);
                ssd1306_send_data(&ssd); // Atualiza o display
                flag_A = 1;
            }
        }
        else
        {
            if (gpio == BOT_B)
            {
                if (flag_status_cobertura == 0 && flag_deslocamento == 1 && flag_bot_emergencia == 0)
                {
                    printf("Acionou B\n");
                    ssd1306_fill(&ssd, !cor); // Limpa o displaY
                    ssd1306_draw_string(&ssd, "FIM DE CURSO", 20, 25);
                    ssd1306_draw_string(&ssd, "DETECTADO", 20, 40);
                    ssd1306_send_data(&ssd); // Atualiza o display
                    flag_B = 1;
                }
            }
            else
            {
                if (gpio == BOT_JOY)
                {
                    if (flag_manual == 0)
                    {
                        flag_bot_joy = !flag_bot_joy;

                        if ((flag_deslocamento == 1) || (flag_bot_emergencia == 1))
                        {
                            flag_bot_emergencia = !flag_bot_emergencia;
                        }
                    }
                    else
                    {
                        if (flag_deslocamento == 0)
                        {
                            flag_acionamento_manual = !flag_acionamento_manual;
                            printf("Acionamento manual: %d\n", flag_acionamento_manual);
                        }
                        else
                        {
                            flag_bot_emergencia = !flag_bot_emergencia;
                        }
                    }
                }
            }
        }
    }
}

void inicializa_PIO(void)
{
    // Configura variaveis para o PIO
    pio = pio0;
   // bool ok;

    // Configura o clock para 128 MHz
    //ok = set_sys_clock_khz(128000, false);

    //if (ok)
    //    printf("Clock set to %ld\n", clock_get_hz(clk_sys));

    // Configuração do PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, LED_PIN);
}

void leitura_sensores(void)
{
    if (flag_temporizador_sensores == 1)
    {
        adc_select_input(1); // Seleciona a entrada ADC do X, para fazer a medição
        leitura_joyX = adc_read();
        temperatura = 0.0076 * leitura_joyX + 10;
        printf("Temperatura: %.1f\n", temperatura);

        adc_select_input(0); // Seleciona a entrada ADC do Y, para fazer a medição
        leitura_joyY = adc_read();
        if (leitura_joyY < 2048)
        {
            chuva = 0;
        }
        else
        {
            chuva = leitura_joyY - 2048;
        }

        printf("Chuva: %.1f\n", chuva);

        char buffer[25];
        char buffer2[25];
        sprintf(buffer, "Temp:%.1f*", temperatura);
        sprintf(buffer2, "Chuva:%d", chuva);
        ssd1306_fill(&ssd, !cor); // Limpa o displa
        ssd1306_draw_string(&ssd, buffer, 20, 25);
        ssd1306_draw_string(&ssd, buffer2, 20, 40);
        ssd1306_send_data(&ssd); // Atualiza o display

        flag_temporizador_sensores = 0;
        uint32_t tempo_agora_espera = to_ms_since_boot(get_absolute_time()); // Pega o tempo atual em milissegundos        

        if ((flag_manual == 0) && (tempo_agora_espera - tempo_anterior_espera > TEMPO_DE_ESPERA))
        {
            tempo_agora_espera = to_ms_since_boot(get_absolute_time());
            if ((LIMITE_INF_TEMP > temperatura) || (temperatura > LIMITE_SUP_TEMP) || (chuva > LIMITE_CHUVA))
            {
                if (flag_status_cobertura == 0)
                {
                    printf("Modo automatico mandou: FECHAR\n");
                    ssd1306_fill(&ssd, !cor); // Limpa o displaY
                    ssd1306_draw_string(&ssd, "MODO AUTO", 20, 25);
                    ssd1306_draw_string(&ssd, "FECHAR", 20, 40);
                    ssd1306_send_data(&ssd); // Atualiza o display
                    sleep_ms(2500);
                    controle_deslocamento(0);
                }
                else
                {
                    printf("Já estava fechada, nada será feito!\n");
                }
            }
            else
            {
                if (flag_status_cobertura == 1)
                {
                    printf("Modo automatico mandou: ABRIR\n");
                    ssd1306_fill(&ssd, !cor); // Limpa o displaY
                    ssd1306_draw_string(&ssd, "MODO AUTO", 20, 25);
                    ssd1306_draw_string(&ssd, "ABRIR", 20, 40);
                    ssd1306_send_data(&ssd); // Atualiza o display
                    sleep_ms(2500);
                    controle_deslocamento(1);
                }
                else
                {
                    printf("Já estava aberta, nada será feito!\n");
                }
            }
        }
    }
}

void controle_manual(void)
{
    printf("Entrou no Modo manual\n");
    ssd1306_fill(&ssd, !cor); // Limpa o displaY
    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
    ssd1306_draw_string(&ssd, "ATIVADO", 20, 40);
    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(2500);
    bool auxiliar_manual = flag_acionamento_manual;
    while (flag_manual == 1)
    {
        if (auxiliar_manual != flag_acionamento_manual)
        {
            auxiliar_manual = flag_acionamento_manual;
            sleep_ms(2500);
            if (gpio_get(BOT_JOY) == 0)
            {
                sleep_ms(2500);
                printf("Saindo do modo manual\n");
                ssd1306_fill(&ssd, !cor); // Limpa o displaY
                ssd1306_draw_string(&ssd, "SAINDO DO", 20, 25);
                ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 40);
                ssd1306_send_data(&ssd); // Atualiza o display
                sleep_ms(2500);
                flag_manual = 0;
            }
            else
            {
                if (flag_status_cobertura == 0)
                {
                    printf("Modo manual mandou: FECHAR\n");
                    ssd1306_fill(&ssd, !cor); // Limpa o displaY
                    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
                    ssd1306_draw_string(&ssd, "FECHAR", 20, 40);
                    ssd1306_send_data(&ssd); // Atualiza o display
                    sleep_ms(2500);
                    controle_deslocamento(0);
                }
                else
                {
                    printf("Modo manual mandou: ABRIR\n");
                    ssd1306_fill(&ssd, !cor); // Limpa o displaY
                    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
                    ssd1306_draw_string(&ssd, "ABRIR", 20, 40);
                    ssd1306_send_data(&ssd); // Atualiza o display
                    sleep_ms(2500);
                    controle_deslocamento(1);
                }
            }
        }
        leitura_sensores();
    }
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    flag_temporizador_sensores = 1;
    return true;
}

void config_display(void)
{

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Função principal
int main()
{
    stdio_init_all();
    inicializa_GPIOs();
    inicializa_PIO();
    adc_init();
    config_display();
    init_buzzer();

    // Apaga Matriz de LEDs
    desenho_pio(0, pio, sm);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BOT_A, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);
    gpio_set_irq_enabled_with_callback(BOT_B, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);
    gpio_set_irq_enabled_with_callback(BOT_JOY, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);

    // Configura um timer repetitivo com intervalo de 3000 milissegundos (3 segundos)
    struct repeating_timer temporizador;
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &temporizador);

    // delay para inicializar o sistema
    sleep_ms(1000);

    // O sistema sempre inicia com a barcaça fechada
    ssd1306_fill(&ssd, !cor); // Limpa o displaY
    ssd1306_draw_string(&ssd, "FAZENDO", 20, 25);
    ssd1306_draw_string(&ssd, "AUTO-HOME", 20, 40);
    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(2500);
    printf("Fazendo auto-home\n");
    controle_deslocamento(0);
    printf("Auto-home feito\n");
    ssd1306_fill(&ssd, !cor); // Limpa o displaY
    ssd1306_draw_string(&ssd, "AUTO-HOME", 20, 25);
    ssd1306_draw_string(&ssd, "FEITO", 20, 40);
    ssd1306_send_data(&ssd); // Atualiza o display
    sleep_ms(2500);

    // Loop principal
    while (1)
    {
        if (flag_bot_joy == 1)
        {
            flag_bot_joy = 0;

            if (gpio_get(BOT_JOY) == 0)
            {
                sleep_ms(3000);
                if ((gpio_get(BOT_JOY) == 0) && flag_deslocamento == 0)
                {
                    flag_manual = 1;
                    controle_manual();
                }
            }
        }

        leitura_sensores();
        sleep_ms(10);
    }
    return 0;
}
