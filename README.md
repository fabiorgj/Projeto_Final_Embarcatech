Embarcatech

Projeto final

Aluno: Fábio Rocha Gomes Jardim

E-mail: fabiogomesj@gmail.com

Matrícula: TIC370100723

# Sistema de Automação para Barcaças de Secagem de Grãos - Enfase na Cultura do Cacau no sul da Bahia

Este repositório contém o firmware desenvolvido para o protótipo mínimo do sistema de automação de barcaças de secagem de grãos. O código simula funcionalidades como controle automático e manual da cobertura, leitura de sensores ambientais, e interface com o usuário via display e LEDs, adaptado para execução na placa BitdogLab.

## Vídeo

[Link para o drive](https://drive.google.com/file/d/18e4z5cQMUwJ62UOw6baL6OcMlThxKqVY/view?usp=sharing)

## Visão Geral
O sistema proposto visa automatizar a abertura e fechamento da cobertura de barcaças de secagem com base em dados de temperatura, chuva e umidade. O protótipo utiliza componentes da BitdogLab para simular sensores, atuadores e interfaces, validando a lógica de controle antes da implementação prática.

O sistema tem como objetivo:
- Monitorar condições ambientais (temperatura e "chuva", simuladas via leituras analógicas).
- Decidir automaticamente quando fechar ou abrir a cobertura da barcaça.
- Permitir controle manual por meio de um botão (joystick) e tratamento de emergência.
- Exibir informações de operação em um display SSD1306.
- Utilizar uma matriz de LEDs 5×5 para sinalizações visuais e um buzzer para alertas sonoros.  

### Funcionalidades Principais
- **Controle Automático**: Decide abrir/fechar a cobertura com base em leituras simuladas de sensores.
- **Controle Manual**: Permite intervenção do usuário via botão do joystick.
- **Sinalização Visual e Sonora**: Usa matriz de LEDs e buzzer para indicar estados do sistema.
- **Interface Gráfica**: Display exibe temperatura, chuva e status da cobertura.
- **Botão de Emergência**: Interrompe operações instantaneamente.
- **Auto-Home**: Inicializa a cobertura na posição fechada.

## Hardware Utilizado (BitdogLab)
- **Matriz de LEDs 5x5**: Indica movimentos e alertas.
- **Display SSD1306**: Mostra dados dos sensores e status.
- **Joystick**: Simula leituras de temperatura (eixo X) e chuva (eixo Y).
- **Botões A/B**: Simulam sensores de fim de curso.
- **LEDs RGB**: Representam o acionamento do motor (verde: fechar, vermelho: abrir).
- **Buzzer**: Alerta sonoro durante movimentos e emergências.

## Principais Funções
### `controle_deslocamento(int chamada)`
Controla o movimento da cobertura (abrir/fechar). Atualiza LEDs, display e buzzer, e trata interrupções de emergência.
- `chamada = 0`: Fecha a cobertura.
- `chamada = 1`: Abre a cobertura.

### `sinalizacao(int sinal)`
Gera sinais visuais e sonoros conforme o estado do sistema:
- `sinal = 0`: Desliga buzzer e LEDs.
- `sinal = 1`: Alerta de emergência (X vermelho e buzzer).
- `sinal = 2/5`: Seta indicando direção (fechar/abrir).
- `sinal = 3/6`: LEDs vermelho/verde sólidos.
- `sinal = 4/7`: Cobertura aberta/fechada (retângulo).

### `leitura_sensores()`
Lê valores simulados de temperatura e chuva via ADC (joystick) e atualiza o display. Retorna `1` se condições estão dentro dos limites, `0` caso contrário.

### `controle_automatico()`
Executa decisões autônomas periodicamente com base nos sensores. Aciona `controle_deslocamento()` se necessário.

### `controle_manual()`
Permite controle manual da cobertura via botão do joystick. Ativa/desativa o modo manual após 3 segundos de pressionamento.

## Variáveis Globais
- **Flags de Estado**:
  - `flag_status_cobertura`: `0` (aberta), `1` (fechada).
  - `flag_manual`: `1` quando em modo manual.
  - `flag_bot_emergencia`: `1` durante emergência.
  - `flag_deslocamento`: `1` durante movimento.
- **Sensores Simulados**:
  - `temperatura`: Valor convertido do eixo X do joystick.
  - `chuva`: Valor convertido do eixo Y do joystick.    

# Estrutura e Funcionalidades do Código

O código é composto por diversas funções, divididas em módulos de inicialização, controle, leitura de sensores e tratamento de interrupções:

## 1. Funções Auxiliares

### matrix_rgb
- **Descrição:** Converte valores RGB para o formato GRB (necessário para os LEDs ws2812).
- **Detalhes:** Atenua o brilho (multiplicação por 0.025) e retorna um valor de 32 bits.

### desenho_pio
- **Descrição:** Desenha um sprite na matriz de LEDs via PIO.
- **Detalhes:** Percorre uma matriz multidimensional que contém os sprites (representando estados como “tudo apagado”, “seta”, “full vermelho”, etc.) e inverte a ordem de pixels em linhas pares para ajuste na visualização.

## 2. Inicialização e Configuração

### inicializa_GPIOs
- **Descrição:** Configura os pinos dos LEDs, botões (com pull-up) e sensores (inicialização dos pinos ADC).
- **Detalhes:** Garante que os LEDs iniciem desligados.

### inicializa_PIO
- **Descrição:** Seleciona o PIO (neste caso, pio0), carrega o programa para controle da matriz de LEDs e configura a state machine utilizada.

### config_display
- **Descrição:** Inicializa a comunicação I2C e configura o display SSD1306.
- **Detalhes:** Limpa o display ao início.

### init_buzzer
- **Descrição:** Configura o pino do buzzer para função PWM.
- **Detalhes:** Ajusta a frequência e inicia o buzzer desligado.

## 3. Funções de Controle e Sinalização

### sinalizacao
- **Descrição:** Aciona o buzzer e atualiza o display da matriz de LEDs com o sprite correspondente.
- **Detalhes:** Diferentes valores de parâmetro `sinal` indicam abertura, fechamento ou alerta de emergência.

### controle_deslocamento
- **Descrição:** Responsável pelo movimento da cobertura (abrir ou fechar).
- **Detalhes:** Durante a operação, atualiza o display, aciona os LEDs e o buzzer, além de monitorar os botões de fim de curso. Também trata situações de emergência.

### controle_automatico
- **Descrição:** Realiza a leitura dos sensores periodicamente (utilizando um timer).
- **Detalhes:** Se os valores estiverem fora dos limites predefinidos, aciona automaticamente o movimento da cobertura (fechamento ou abertura), desde que o sistema não esteja em modo manual.

### controle_manual
- **Descrição:** Permite que o usuário assuma o controle da cobertura via botão do joystick.
- **Detalhes:** Desabilita temporariamente o modo automático, permitindo a abertura ou fechamento conforme o comando manual.

## 4. Leitura de Sensores e Interrupções

### leitura_sensores
- **Descrição:** Faz a leitura dos sensores de temperatura e chuva via ADC.
- **Detalhes:** Atualiza o display com os valores medidos e retorna um valor booleano indicando se os sensores estão dentro dos limites aceitáveis.

### call_back_dos_botoes
- **Descrição:** Função de callback para interrupções dos botões.
- **Detalhes:** Implementa debounce e, conforme o botão pressionado (BOT_A, BOT_B ou BOT_JOY), atualiza flags de controle (por exemplo, fim de curso, emergência ou acionamento manual).

### repeating_timer_callback
- **Descrição:** Callback de um timer repetitivo.
- **Detalhes:** Seta uma flag para acionar a leitura dos sensores em intervalos regulares (1000 ms).

## 5. Função Principal

### main
- **Descrição:** Inicializa todos os módulos (GPIOs, PIO, ADC, display e buzzer), configura as interrupções e o timer.
- **Detalhes:** Executa o auto-home (fechamento inicial da cobertura) e, em seguida, entra em um loop infinito que verifica:
  - Se o botão do joystick foi pressionado para acionar o modo manual.
  - Caso contrário, executa o controle automático baseado na leitura dos sensores e no tempo decorrido.

## Fluxo de Execução

### Inicializações
- Configuração dos GPIOs, PIO, ADC, display e buzzer.
- Apagamento inicial da matriz de LEDs.
- Configuração de interrupções e timer repetitivo.

### Auto-home
- Realiza o fechamento da cobertura (definindo a posição inicial) com feedback via display e LEDs.

### Loop Principal
- Monitora a entrada do botão do joystick para alternar entre os modos manual e automático.
- No modo automático, lê os sensores e decide se a cobertura deve ser aberta ou fechada.
- No modo manual, permite a intervenção do usuário para controle direto da cobertura.

## Observações

### Simulação na BitdogLab
- Foram realizadas adaptações, como a utilização da matriz de LEDs 5×5 e a simulação dos sensores por meio dos eixos do joystick.
- A comunicação Wi-Fi foi removida, focando nos conceitos de controle e sinalização.

### Aplicação Prática
- Em uma implementação real, os sensores e atuadores poderão ser substituídos por dispositivos de maior precisão e comunicação remota integrada, conforme discutido no relatório do projeto.

## Conclusão

Este firmware demonstra uma implementação básica e funcional de um sistema de automação para barcaças, integrando controle automático e manual, monitoramento de sensores e feedback visual/sonoro. A estrutura modular do código facilita futuras expansões e adaptações para hardware real, conforme as necessidades do projeto.
