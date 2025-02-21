// Converte um código hexadecimal gerado no site: https://www.piskelapp.com/
// Para decimal em no padrão RGB.
// O código gerado será utilizado para exibir imagens na matriz de leds neopixel.
#include <stdio.h>

#define MATRIX_ROWS 5
#define MATRIX_COLS 5
#define MATRIX_DEPTH 3
#define QUNATIDADE_DE_IMAGENS 4

// Função para converter valores ARGB (0xAARRGGBB) para RGB
void convertToRGB(int argb, int rgb[3]) {
    rgb[0] = argb & 0xFF;          // Blue
    rgb[2] = (argb >> 16) & 0xFF;  // Red
    rgb[1] = (argb >> 8) & 0xFF;   // Green
}

int main() {
    // Matriz de entrada com os valores ARGB
    int argb_values[QUNATIDADE_DE_IMAGENS][MATRIX_ROWS * MATRIX_COLS] = {
///////////////////////////////////////////////////////////////////////// SUBSTITUIR TODOS OS SPRITES A PARTIR DAQUI
{
    0x00000000, 0x00000000, 0x00000000, 0xff0000ff, 0x00000000, 
    0x00000000, 0x00000000, 0xff0000ff, 0x00000000, 0x00000000, 
    0x00000000, 0xff0000ff, 0x00000000, 0x00000000, 0x00000000, 
    0x00000000, 0x00000000, 0xff0000ff, 0x00000000, 0x00000000, 
    0x00000000, 0x00000000, 0x00000000, 0xff0000ff, 0x00000000
    },
    {
    0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 
    0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 
    0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 
    0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 
    0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff
    },
    {
    0x00000000, 0xffff0009, 0x00000000, 0x00000000, 0x00000000, 
    0x00000000, 0x00000000, 0xffff0009, 0x00000000, 0x00000000, 
    0x00000000, 0x00000000, 0x00000000, 0xffff0009, 0x00000000, 
    0x00000000, 0x00000000, 0xffff0009, 0x00000000, 0x00000000, 
    0x00000000, 0xffff0009, 0x00000000, 0x00000000, 0x00000000
    },
    {
    0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 
    0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 
    0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 
    0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 
    0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009, 0xffff0009
    }
/////////////////////////////////////////////////////////// ATÉ AQUI
    };

    // Matriz 5x5x3 para armazenar os valores RGB
    int rgb_matrix[MATRIX_ROWS][MATRIX_COLS][MATRIX_DEPTH];

    for(int j = 0; j < QUNATIDADE_DE_IMAGENS; j++){

    // Preencher a matriz RGB com a conversão dos valores ARGB
    for (int i = 0; i < MATRIX_ROWS * MATRIX_COLS; i++) {
        int rgb[3];
        convertToRGB(argb_values[j][i], rgb);
        
        int row = i / MATRIX_COLS;    // Cálculo da linha
        int col = i % MATRIX_COLS;    // Cálculo da coluna
        
        // Armazenar os valores RGB na matriz 5x5x3
        rgb_matrix[row][col][0] = rgb[0];  // Red
        rgb_matrix[row][col][1] = rgb[1];  // Green
        rgb_matrix[row][col][2] = rgb[2];  // Blue
    }

    // Exibir a matriz RGB resultante
    printf("{\n");
    for (int x = 0; x < MATRIX_ROWS; x++) {
        printf("    {");
        for (int y = 0; y < MATRIX_COLS; y++) {
            printf("{%d, %d, %d}", rgb_matrix[x][y][0], rgb_matrix[x][y][1], rgb_matrix[x][y][2]);
            if (y < MATRIX_COLS - 1) {
                printf(", ");
            }
        }
        printf("}");
        if (x < MATRIX_ROWS - 1) {
            printf(",");
        }
        printf("\n");
    }
    if(j==QUNATIDADE_DE_IMAGENS-1){
        printf("}\n\n");        
    }
    else{
    printf("},\n\n");

    }
    }
    return 0;
}