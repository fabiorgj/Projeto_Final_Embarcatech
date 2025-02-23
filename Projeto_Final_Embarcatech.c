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

// Arquivo .pio com o programa para controlar a matriz de LEDs
#include "pio_matrix.pio.h"

// Definições de pinos e configurações
#define LED_R 13               // LED vermelho
#define LED_G 11               // LED verde
#define LED_B 12               // LED azul
#define BOT_A 5                // Botão A
#define BOT_B 6                // Botão B
#define BOT_JOY 22             // Botão do joystick
#define LED_COUNT 25           // Número de LEDs na matriz
#define LED_PIN 7              // Pino de controle dos LEDs
#define SENSOR_TEMPERATURA 27  // Pino do sensor de temperatura
#define SENSOR_CHUVA 26        // Pino do sensor de chuva
#define LIMITE_CHUVA 1000      // Valor limite para chuva
#define LIMITE_SUP_TEMP 38     // Limite superior de temperatura
#define LIMITE_INF_TEMP 15     // Limite inferior de temperatura
#define TEMPO_DE_ESPERA 15000  // Tempo de espera em milissegundos para ação automática
#define I2C_PORT i2c1          // Porta I2C utilizada
#define I2C_SDA 14             // Pino SDA do I2C
#define I2C_SCL 15             // Pino SCL do I2C
#define endereco 0x3C          // Endereço I2C do display SSD1306
#define BUZZER_PIN 21          // Pino do buzzer
#define BUZZER_FREQUENCY 2000  // Frequência do buzzer em Hz

// Variáveis globais
static volatile uint32_t tempo_anterior = 0;
static volatile uint32_t tempo_anterior_espera = 0;
static PIO pio;      // Instância do PIO utilizado
static uint sm;      // State Machine do PIO

// Flags de controle do sistema
bool flag_A = 0;
bool flag_B = 0;
bool flag_bot_emergencia = 0;
bool flag_manual = 0;
bool flag_acionamento_manual = 0;
bool flag_status_cobertura = 0; // 1 = cobertura fechada, 0 = cobertura aberta
bool flag_bot_joy = 0;
bool flag_deslocamento = 0;     // 1 = em deslocamento, 0 = parado
bool flag_temporizador = 0;

// Variáveis para leitura dos sensores
int leitura_joyX = 0;
int leitura_joyY = 0;
float temperatura = 0;
int chuva = 0;

ssd1306_t ssd;    // Estrutura do display
bool cor = true;  // Cor de fundo do display

// Matriz multidimensional com os sprites a serem exibidos na matriz de LEDs
// Cada sprite é composto por 5 linhas, 5 colunas e 3 valores (RGB) por pixel
int matriz_numeros[8][5][5][3] = {
    {// fullDark (tudo apagado)
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
    
    {// < (seta para a esquerda)
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}, {0, 0, 0}}},
    
    {// fullR (tudo vermelho)
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}}},
    
    {// Aberta (ex.: desenho indicando abertura)
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}},
     {{255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}, {255, 0, 0}}},
    
    {// > (seta para a direita)
     {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}},
     {{0, 0, 0}, {0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}}},
    
    {// fullG (tudo verde)
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}}},
    
    {// Fechada (ex.: desenho indicando fechamento)
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 255, 0}},
     {{0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}, {0, 255, 0}}}
};

//====================================================================
// FUNÇÕES AUXILIARES
//====================================================================

/**
 * Converte os valores RGB para o formato GRB necessário para os LEDs ws2812,
 * atenuando o brilho multiplicando cada valor por 0.025.
 */
static uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R = (unsigned char)(r * 0.025);
    unsigned char G = (unsigned char)(g * 0.025);
    unsigned char B = (unsigned char)(b * 0.025);
    return ((uint32_t)G << 24) | ((uint32_t)R << 16) | ((uint32_t)B << 8);
}

/**
 * Desenha um sprite (índice 'sprite' da matriz) na matriz de LEDs via PIO.
 * Percorre as linhas e colunas do sprite e envia os valores convertidos para o PIO.
 * A lógica inverte a ordem em linhas pares para ajustar a visualização.
 */
void desenho_pio(int sprite, PIO pio, uint sm) {
    uint32_t valor_led;
    // Percorre as linhas de baixo para cima (para inverter a visualização)
    for (int linha = 4; linha >= 0; linha--) {
        // Percorre as colunas
        for (int coluna = 0; coluna < 5; coluna++) {
            if (linha % 2 != 0) {
                // Linha ímpar: envia os pixels na ordem direta
                valor_led = matrix_rgb(
                    matriz_numeros[sprite][linha][coluna][0],
                    matriz_numeros[sprite][linha][coluna][1],
                    matriz_numeros[sprite][linha][coluna][2]);
                pio_sm_put_blocking(pio, sm, valor_led);
            } else {
                // Linha par: inverte a ordem (da direita para a esquerda)
                valor_led = matrix_rgb(
                    matriz_numeros[sprite][linha][4 - coluna][0],
                    matriz_numeros[sprite][linha][4 - coluna][1],
                    matriz_numeros[sprite][linha][4 - coluna][2]);
                pio_sm_put_blocking(pio, sm, valor_led);
            }
        }
    }
}

/**
 * Inicializa o buzzer configurando o pino com função PWM e definindo a frequência.
 */
void init_buzzer(void) {
    // Configura o pino do buzzer para função PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    // Obtém o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    // Configura o PWM com os parâmetros padrão e ajusta o divisor de clock
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096));
    pwm_init(slice_num, &config, true);
    // Inicia com o nível baixo (buzzer desligado)
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

void sinalizacao(int sinal) {
    switch (sinal)
    {
    case 0:
        pwm_set_gpio_level(BUZZER_PIN, 0);
        desenho_pio(0, pio, sm);
        break;
    case 1:
        pwm_set_gpio_level(BUZZER_PIN, 750);
        desenho_pio(1, pio, sm);
        break;
    
    case 2: // Seta > (direita)(fechar cobertura)
        pwm_set_gpio_level(BUZZER_PIN, 750);
        desenho_pio(5, pio, sm);
        break;
    
    case 3: // Full R (tudo vermelho)
        pwm_set_gpio_level(BUZZER_PIN, 750);
        desenho_pio(6, pio, sm);
        break;
    
    case 4: // Retângulo R (cobertura aberta)
        desenho_pio(7, pio, sm);
        pwm_set_gpio_level(BUZZER_PIN, 0);
        break;
    
    case 5: // Seta < (esquerda)(abrir cobertura)
        pwm_set_gpio_level(BUZZER_PIN, 750);
        desenho_pio(2, pio, sm);
        break;
    
    case 6: // Full G (tudo verde)
        pwm_set_gpio_level(BUZZER_PIN, 750);
        desenho_pio(3, pio, sm);
        break;
    
    case 7: // Retângulo G (cobertura fechada)
        desenho_pio(4, pio, sm);
        pwm_set_gpio_level(BUZZER_PIN, 0);
        break;
    
    default:
        break;
    }

}

/**
 * Controla o deslocamento (fechamento ou abertura) da cobertura.
 *0 = fechar; 1 = abrir.
 * Inclui tratamento para emergência, com atualização dos LEDs, display e buzzer.
 */
void controle_deslocamento(int chamada) {
    switch (chamada) {
        case 0: // Fechar cobertura
            gpio_put(LED_G, 1);      // Liga LED verde
            flag_deslocamento = 1;
            printf("Chamada para fechar\n");
            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, "FECHANDO", 20, 25);
            ssd1306_send_data(&ssd);
            sleep_ms(2500);
            while (flag_B == 0) {
                sinalizacao(2);
                sleep_ms(250);
                sinalizacao(0);
                sleep_ms(250);
                // Se for acionada emergência, muda a rotina
                if (flag_bot_emergencia == 1) {
                    printf("Chamada para emergência\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "EMERGENCIA", 20, 25);
                    ssd1306_send_data(&ssd);
                    gpio_put(LED_G, 0);
                    while (flag_bot_emergencia == 1) {
                        sinalizacao(1);
                        sleep_ms(100);
                        sinalizacao(0);
                        sleep_ms(100);
                    }
                    gpio_put(LED_G, 1);
                    flag_deslocamento = 1;
                    printf("Saiu da emergência\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "EMERGENCIA OFF", 5, 25);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                }
            }
            flag_B = 0;
            // Sequência final de fechamento
            for (int i = 0; i < 3; i++) {
                sinalizacao(3);
                sleep_ms(150);
                sinalizacao(0);
                sleep_ms(150);
            }
            sinalizacao(4);
            flag_status_cobertura = 1; // Indica que a cobertura está fechada
            flag_deslocamento = 0;
            sleep_ms(2500);
            gpio_put(LED_G, 0);
            printf("Deslocamento encerrado\n");
            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, "DESLOCAMENTO", 20, 25);
            ssd1306_draw_string(&ssd, "ENCERRADO", 20, 40);
            ssd1306_send_data(&ssd);
            sleep_ms(2500);
            break;
        case 1: // Abrir cobertura
            gpio_put(LED_R, 1);      // Liga LED vermelho
            flag_deslocamento = 1;
            printf("Chamada para abrir\n");
            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, "ABRINDO", 20, 25);
            ssd1306_send_data(&ssd);
            sleep_ms(2500);
            while (flag_A == 0) {
                sinalizacao(5);
                sleep_ms(250);
                sinalizacao(0);
                sleep_ms(250);
                if (flag_bot_emergencia == 1) {
                    printf("Chamada para emergência\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "EMERGENCIA", 20, 25);
                    ssd1306_send_data(&ssd);
                    gpio_put(LED_R, 0);
                    while (flag_bot_emergencia == 1) {
                        sinalizacao(1);
                        sleep_ms(100);
                        sinalizacao(0);
                        sleep_ms(100);
                    }
                    gpio_put(LED_R, 1);
                    flag_deslocamento = 1;
                    printf("Saiu da emergência\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "EMERGENCIA OFF", 5, 25);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                }
            }
            flag_A = 0;
            // Sequência final de abertura
            for (int i = 0; i < 3; i++) {
                sinalizacao(6);
                sleep_ms(150);
                sinalizacao(0);
                sleep_ms(150);
            }
            sinalizacao(7);
            flag_status_cobertura = 0; // Indica que a cobertura está aberta
            flag_deslocamento = 0;
            sleep_ms(2500);
            gpio_put(LED_R, 0);
            printf("Deslocamento encerrado\n");
            ssd1306_fill(&ssd, !cor);
            ssd1306_draw_string(&ssd, "DESLOCAMENTO", 20, 25);
            ssd1306_draw_string(&ssd, "ENCERRADO", 20, 40);
            ssd1306_send_data(&ssd);
            sleep_ms(2500);
            break;
    }
}

/**
 * Inicializa os GPIOs configurando:
 *  - LEDs como saída.
 *  - Botões como entrada com pull-up.
 *  - ADCs para os sensores de temperatura e chuva.
 */
void inicializa_GPIOs(void) {
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_init(BOT_A);
    gpio_init(BOT_B);
    gpio_init(BOT_JOY);

    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);

    gpio_set_dir(BOT_A, GPIO_IN);
    gpio_set_dir(BOT_B, GPIO_IN);
    gpio_set_dir(BOT_JOY, GPIO_IN);

    gpio_pull_up(BOT_A);
    gpio_pull_up(BOT_B);
    gpio_pull_up(BOT_JOY);

    adc_gpio_init(SENSOR_TEMPERATURA);
    adc_gpio_init(SENSOR_CHUVA);

    // Garante que os LEDs iniciem desligados
    gpio_put(LED_R, 0);
    gpio_put(LED_G, 0);
    gpio_put(LED_B, 0);
}

/**
 * Callback para interrupções dos botões.
 * Realiza debounce e, de acordo com o pino acionado, atualiza flags e exibe mensagens no display.
 */
void call_back_dos_botoes(uint gpio, uint32_t events) {
    uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());
    if (tempo_agora - tempo_anterior > 250) { // Debounce de 250ms
        tempo_anterior = tempo_agora;
        if (gpio == BOT_A) {
            if (flag_status_cobertura == 1 && flag_deslocamento == 1 && flag_bot_emergencia == 0) {
                printf("Acionou A\n");
                ssd1306_fill(&ssd, !cor);
                ssd1306_draw_string(&ssd, "FIM DE CURSO", 20, 25);
                ssd1306_draw_string(&ssd, "DETECTADO", 20, 40);
                ssd1306_send_data(&ssd);
                flag_A = 1;
            }
        } else if (gpio == BOT_B) {
            if (flag_status_cobertura == 0 && flag_deslocamento == 1 && flag_bot_emergencia == 0) {
                printf("Acionou B\n");
                ssd1306_fill(&ssd, !cor);
                ssd1306_draw_string(&ssd, "FIM DE CURSO", 20, 25);
                ssd1306_draw_string(&ssd, "DETECTADO", 20, 40);
                ssd1306_send_data(&ssd);
                flag_B = 1;
            }
        } else if (gpio == BOT_JOY) {
            if (flag_manual == 0) {
                flag_bot_joy = !flag_bot_joy;
                if ((flag_deslocamento == 1) || (flag_bot_emergencia == 1)) {
                    flag_bot_emergencia = !flag_bot_emergencia;
                }
            } else {
                if (flag_deslocamento == 0) {
                    flag_acionamento_manual = !flag_acionamento_manual;
                    printf("Acionamento manual: %d\n", flag_acionamento_manual);
                } else {
                    flag_bot_emergencia = !flag_bot_emergencia;
                }
            }
        }
    }
}

/**
 * Inicializa o PIO:
 *  - Seleciona o pio0.
 *  - Carrega o programa para a matriz de LEDs.
 *  - Configura a state machine utilizada.
 */
void inicializa_PIO(void) {
    pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, LED_PIN);
}

/// Função para leitura dos sensores e retorno de parâmetros condicionais
bool leitura_sensores(void){
        // Leitura do sensor de temperatura (entrada ADC 1)
        adc_select_input(1);
        leitura_joyX = adc_read();
        temperatura = 0.0076 * leitura_joyX + 10;
        printf("Temperatura: %.1f\n", temperatura);

        // Leitura do sensor de chuva (entrada ADC 0)
        adc_select_input(0);
        leitura_joyY = adc_read();
        if (leitura_joyY < 2048) {
            chuva = 0;
        } else {
            chuva = leitura_joyY - 2048;
        }
        printf("Chuva: %.1f\n", chuva);

        // Atualiza o display com os valores dos sensores
        char buffer[25];
        char buffer2[25];
        sprintf(buffer, "Temp:%.1f*", temperatura);
        sprintf(buffer2, "Chuva:%d", chuva);
        ssd1306_fill(&ssd, !cor);
        ssd1306_draw_string(&ssd, buffer, 20, 25);
        ssd1306_draw_string(&ssd, buffer2, 20, 40);
        ssd1306_send_data(&ssd);

        if((LIMITE_INF_TEMP < temperatura) && (temperatura < LIMITE_SUP_TEMP) && (chuva < LIMITE_CHUVA)){
            return 1;
        } else {
            return 0;
        }
}

/**
 * Realiza a leitura dos sensores de temperatura e chuva via ADC.
 * Atualiza o display com os valores medidos e, se necessário, aciona o modo automático.
 */
void controle_automatico(void) {
    if (flag_temporizador == 1) {
        
        flag_temporizador = 0;
        
        uint32_t tempo_agora_espera = to_ms_since_boot(get_absolute_time());

        // Verifica se deve executar ação automática (quando não estiver em modo manual)
        if ((flag_manual == 0) && (tempo_agora_espera - tempo_anterior_espera > TEMPO_DE_ESPERA)) {
            tempo_agora_espera = to_ms_since_boot(get_absolute_time());
            if (!leitura_sensores()) {
                if (flag_status_cobertura == 0) {
                    printf("Modo automatico mandou: FECHAR\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "MODO AUTO", 20, 25);
                    ssd1306_draw_string(&ssd, "FECHAR", 20, 40);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                    controle_deslocamento(0);
                } else {
                    printf("Já estava fechada, nada será feito!\n");
                }
            } else {
                if (flag_status_cobertura == 1) {
                    printf("Modo automatico mandou: ABRIR\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "MODO AUTO", 20, 25);
                    ssd1306_draw_string(&ssd, "ABRIR", 20, 40);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                    controle_deslocamento(1);
                } else {
                    printf("Já estava aberta, nada será feito!\n");
                }
            }
        }
    }
}

/**
 * Função de controle manual.
 * Permite alternar entre abrir e fechar a cobertura com base no acionamento do botão do joystick.
 */
void controle_manual(void) {
    printf("Entrou no Modo manual\n");
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
    ssd1306_draw_string(&ssd, "ATIVADO", 20, 40);
    ssd1306_send_data(&ssd);
    sleep_ms(2500);
    bool auxiliar_manual = flag_acionamento_manual;
    while (flag_manual == 1) {
        if (auxiliar_manual != flag_acionamento_manual) {
            auxiliar_manual = flag_acionamento_manual;
            sleep_ms(2500);
            if (gpio_get(BOT_JOY) == 0) {
                sleep_ms(2500);
                printf("Saindo do modo manual\n");
                ssd1306_fill(&ssd, !cor);
                ssd1306_draw_string(&ssd, "SAINDO DO", 20, 25);
                ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 40);
                ssd1306_send_data(&ssd);
                sleep_ms(2500);
                flag_manual = 0;
            } else {
                if (flag_status_cobertura == 0) {
                    printf("Modo manual mandou: FECHAR\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
                    ssd1306_draw_string(&ssd, "FECHAR", 20, 40);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                    controle_deslocamento(0);
                } else {
                    printf("Modo manual mandou: ABRIR\n");
                    ssd1306_fill(&ssd, !cor);
                    ssd1306_draw_string(&ssd, "MODO MANUAL", 20, 25);
                    ssd1306_draw_string(&ssd, "ABRIR", 20, 40);
                    ssd1306_send_data(&ssd);
                    sleep_ms(2500);
                    controle_deslocamento(1);
                }
            }
        }
        if (flag_temporizador == 1){
            flag_temporizador = 0;
            leitura_sensores();
        }
    }
}

/**
 * Callback do timer repetitivo que seta a flag para leitura dos sensores.
 */
bool repeating_timer_callback(struct repeating_timer *t) {
    flag_temporizador = 1;
    return true;
}

/**
 * Configura e inicializa o display SSD1306 via I2C.
 */
void config_display(void) {
    // Inicializa a comunicação I2C em 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    // Limpa o display (inicia com todos os pixels apagados)
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

/**
 * Função principal.
 */
int main() {
    stdio_init_all();
    inicializa_GPIOs();
    inicializa_PIO();
    adc_init();
    config_display();
    init_buzzer();

    // Apaga a matriz de LEDs
    desenho_pio(0, pio, sm);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(BOT_A, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);
    gpio_set_irq_enabled_with_callback(BOT_B, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);
    gpio_set_irq_enabled_with_callback(BOT_JOY, GPIO_IRQ_EDGE_FALL, true, &call_back_dos_botoes);

    // Configura um timer repetitivo com intervalo de 1000 ms
    struct repeating_timer temporizador;
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &temporizador);

    // Delay para estabilização do sistema
    sleep_ms(1000);

    // Inicializa o sistema com a cobertura fechada (auto-home)
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "FAZENDO", 20, 25);
    ssd1306_draw_string(&ssd, "AUTO-HOME", 20, 40);
    ssd1306_send_data(&ssd);
    sleep_ms(2500);
    printf("Fazendo auto-home\n");
    controle_deslocamento(0);
    printf("Auto-home feito\n");
    ssd1306_fill(&ssd, !cor);
    ssd1306_draw_string(&ssd, "AUTO-HOME", 20, 25);
    ssd1306_draw_string(&ssd, "FEITO", 20, 40);
    ssd1306_send_data(&ssd);
    sleep_ms(2500);

    // Loop principal do sistema
    while (1) {
        if (flag_bot_joy == 1) {
            flag_bot_joy = 0;
            if (gpio_get(BOT_JOY) == 0) {
                sleep_ms(3000); //Se o botão foi pressionado por 3 segundos, entra no modo manual
                if ((gpio_get(BOT_JOY) == 0) && flag_deslocamento == 0) {
                    flag_manual = 1;
                    controle_manual();
                }
            }
        }
        controle_automatico();
        sleep_ms(10);
    }
    return 0;
}
