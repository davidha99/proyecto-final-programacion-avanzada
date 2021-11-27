#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct posiciones {
    int pos_ini;
    int pos_fin;
} posicion;

posicion secuencias[1001];  // 1001 por que las secuencias se van a guardar del 1 al 1000
int cont_secuencias = 1;    // Empieza en la posición 1 porque la posición 0 no la vamos a usar
char *referencia;
long lsize_referencia;

void obtener_string_referencia() {
    FILE *archivo_referencia;
    archivo_referencia = fopen("S._cerevisiae_processed.txt", "rb");

    fseek(archivo_referencia, 0L, SEEK_END);
    lsize_referencia = ftell(archivo_referencia);
    rewind(archivo_referencia);

    /* allocate memory for entire content */
    referencia = calloc(1, lsize_referencia + 1);

    /* copy the file into the buffer */
    fread(referencia, lsize_referencia, 1, archivo_referencia);

    /*referencia ya es un string*/
    fclose(archivo_referencia);
}

void algoritmo_substring() {
    /*Thread 1: 1-100*/
    /*Thread 2: 101-200*/
    /*Thread 3: 201-300*/
    FILE *archivo_secuencia;
    char *secuencia = NULL;
    size_t secuencia_line_buf = 0;
    ssize_t secuencia_len;
    archivo_secuencia = fopen("s_cerevisia_2021_03.seq", "r");

    secuencia_len = getline(&secuencia, &secuencia_line_buf, archivo_secuencia);

    /*Variables para el algoritmo de encontrar substring*/
    int i, j, k, encontrado = 0;
    posicion pos;

    while (secuencia_len >= 0) {
        /*Le restamos -2 porque por el getline me regresaba dos caracteres de más. Por ejemplo,
        para la primera secuencia me tomaba que el tamaño era 12,916 cuando en realidad era 12,914*/
        secuencia_len -= 2;

        /*Buscar si la secuencia está en la referencia*/
        for (i = 0; referencia[i]; i++) {
            k = i;
            for (j = 0; j <= secuencia_len - 1; j++) {
                if (referencia[k] != secuencia[j])
                    break;
                k++;
            }
            if (j == secuencia_len) {
                pos.pos_ini = i;
                pos.pos_fin = k - 1;
                secuencias[cont_secuencias] = pos;
                encontrado = 1;
            }

            if (encontrado == 1)
                break;
        }

        if (encontrado == 0) {
            pos.pos_ini = -1;
            pos.pos_fin = -1;
            secuencias[cont_secuencias] = pos;
        }
        /*Aumentamos contador para guardar la siguiente estructura de tipo "posición"*/
        cont_secuencias++;

        /*Reseteamos variable encontrado para la próxima secuencia*/
        encontrado = 0;

        /*Get the next line*/
        secuencia_len = getline(&secuencia, &secuencia_line_buf, archivo_secuencia);
    }
    fclose(archivo_secuencia);
}

int main() {
    posicion posicion_dummy = {-1, -1};
    /*posicion_dummy es porque vamos a guardar las secuencias de la posición 1 a la 1000, ya que
    son 1000 secuencias*/
    secuencias[0] = posicion_dummy;

    obtener_string_referencia();

    algoritmo_substring();

    for (int i = 1; i < 1001; i++) {
        printf("Sec %d: Posición inicial: %d, Posición final: %d\n", i, secuencias[i].pos_ini, secuencias[i].pos_fin);
    }

    // Thread 1: 1-100
    // for (int i = 1; i <= 100; i++) {
    //     secuencias[i] =  // lo que regrese el algoritmo
    // }

    // free(referencia);  // Todavía en duda si puede ir aquí
    return 0;
}