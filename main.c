#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define NTHREADS 10
// pthread_mutex_t lock;

/*Estructura para guardar en qué posición se encontró la secuencia en la referencia*/
typedef struct posiciones {
    int pos_ini;
    int pos_fin;
} posicion;

/*Estructura que le indicará a los threads el rango de secuencias que deberán buscar en la referencia*/
typedef struct rangos {
    int lim_inf;
    int lim_sup;
} rango;

posicion secuencias[1001];                                        // 1001 por que las secuencias se van a guardar del 1 al 1000
int pos_next_secuencia = 1;                                       // Empieza en la posición 1 porque la posición 0 no la vamos a usar
int num_secuencias_mapeadas = 0, num_secuencias_no_mapeadas = 0;  // Contador de secuencias mapeadas y no mapeadas
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

/*Función que hará cada uno de los threads. Va a necesitar la firma del parámetro (void *x) para que
pueda recibir los argumentos*/
void *algoritmo_substring(void *x) {
    // pthread_mutex_lock(&lock);
    /*Thread 1: 1-100*/
    /*Thread 2: 101-200*/
    /*Thread 3: 201-300*/

    FILE *archivo_secuencia;
    char *secuencia = NULL;
    size_t secuencia_line_buf = 0;
    int line_count = 0;  // variable que llevara un registro del número de secuencia en la que va
    ssize_t secuencia_len;
    /*Casteando el argumento*/
    rango limite;
    limite = *((rango *)x);
    archivo_secuencia = fopen("s_cerevisia_2021_03.seq", "r");

    secuencia_len = getline(&secuencia, &secuencia_line_buf, archivo_secuencia);

    /*Variables para el algoritmo de encontrar substring*/
    int i, j, k, encontrado = 0;
    posicion pos;

    /*Buscar si el rango de secuencias que le corresponde al thread están en la referencia*/
    while (secuencia_len >= 0) {
        line_count++;
        if (line_count >= limite.lim_inf && line_count <= limite.lim_sup) {
            /*Le restamos -2 porque el getline me regresaba dos caracteres de más. Por ejemplo, para
        la primera secuencia me tomaba que el tamaño era 12,916 cuando en realidad era 12,914*/
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
                    secuencias[pos_next_secuencia] = pos;
                    encontrado = 1;
                    num_secuencias_mapeadas++;
                }

                if (encontrado == 1)
                    break;
            }

            if (encontrado == 0) {
                pos.pos_ini = -1;
                pos.pos_fin = -1;
                secuencias[pos_next_secuencia] = pos;
                num_secuencias_no_mapeadas++;
            }
            /*Aumentamos contador para guardar la siguiente estructura de tipo "posición"*/
            pos_next_secuencia++;

            /*Reseteamos variable encontrado para la próxima secuencia*/
            encontrado = 0;
        }

        /*Get the next line*/
        secuencia_len = getline(&secuencia, &secuencia_line_buf, archivo_secuencia);
    }
    fclose(archivo_secuencia);
    // pthread_mutex_unlock(&lock);
    return NULL;
}

//Funcion para ordenar los limites de las posiciones iniciales por el metodo burbuja
void ordena_estructura_bsort(){
    int i,j;
    posicion temp;
    int tama=1001;

    for (i=1; i<=tama; i++){
        for (j=i+1; j<=tama; j++){

            if(secuencias[i].pos_ini > secuencias[j].pos_ini)
            {
                temp=secuencias[i];
                secuencias[i]=secuencias[j];
                secuencias[j]=temp;

            }
        }

    }


}

//funcion que calcula el porcentaje de la similitud entre las cadenas y la referencia
float calcular_porcentaje(){

    ordena_estructura_bsort();

    int tama=1001;
    long acumulado_caracteres=0;

    //para que no haya traslapes modificaremos algunos rangos
    for(int i=tama-1; i>=1 && secuencias[i].pos_fin != -1 ;i--){
        if(secuencias[i].pos_ini < secuencias[i-1].pos_fin)
        {
            secuencias[i].pos_ini = secuencias[i-1].pos_fin;
        }
        
    }

    for (int j=tama-1; j>=1 && secuencias[j].pos_fin != -1 ;j--){
        acumulado_caracteres = acumulado_caracteres + (secuencias[j].pos_fin - secuencias[j].pos_ini);
    }
    
    return ( (acumulado_caracteres * 100 ) / lsize_referencia );

}

int main() {
    posicion posicion_dummy = {-1, -1};
    /*posicion_dummy es porque vamos a guardar las secuencias de la posición 1 a la 1000, ya que
    son 1000 secuencias*/
    secuencias[0] = posicion_dummy;

    obtener_string_referencia();

    /* EMPIEZA PARTE DE THREADS */
    // if (pthread_mutex_init(&lock, NULL) != 0) {
    //     printf("\n mutex setup has failed\n");
    //     return 1;
    // }

    /*Estructura que va a contener referencias a cada uno de los threads que estamos generando.
    En este caso, estamos generando 10 threads*/
    pthread_t threads[NTHREADS];

    /*Estructura que va a tener los argumentos de nuestros threads.
    En este caso, el límite inferior (entero que le indicará desde cuál secuencia empieza)
    y el límite superior (entero que le indicará en cuál secuencia termina). Es decir, guardará
    los rangos que le van a corresponder a cada thread */
    rango thread_args[NTHREADS];

    int rc;         // Variable que guardará el resultado de crear el thread
    rango limites;  // Variable que guardará los rangos

    /*Crear los threads*/
    for (int i = 0; i < NTHREADS; i++) {
        /*Asignar los límites a cada thread*/

        if (i == 0) {  // Thread 1
            limites.lim_inf = 1;
            limites.lim_sup = 100;
            thread_args[i] = limites;
        } else if (i == 1) {  // Thread 2
            limites.lim_inf = 101;
            limites.lim_sup = 200;
            thread_args[i] = limites;
        } else if (i == 2) {  // Thread 3
            limites.lim_inf = 201;
            limites.lim_sup = 300;
            thread_args[i] = limites;
        } else if (i == 3) {  //Thread 4
            limites.lim_inf = 301;
            limites.lim_sup = 400;
            thread_args[i] = limites;
        } else if (i == 4) {  // Thread 5
            limites.lim_inf = 401;
            limites.lim_sup = 500;
            thread_args[i] = limites;
        } else if (i == 5) {  // Thread 6
            limites.lim_inf = 501;
            limites.lim_sup = 600;
            thread_args[i] = limites;
        } else if (i == 6) {  // Thread 7
            limites.lim_inf = 601;
            limites.lim_sup = 700;
            thread_args[i] = limites;
        } else if (i == 7) {  // Thread 8
            limites.lim_inf = 701;
            limites.lim_sup = 800;
            thread_args[i] = limites;
        } else if (i == 8) {  // Thread 9
            limites.lim_inf = 801;
            limites.lim_sup = 900;
            thread_args[i] = limites;
        } else {  // Thread 10
            limites.lim_inf = 901;
            limites.lim_sup = 1000;
            thread_args[i] = limites;
        }

        printf("Creando thread %d\n", i + 1);
        rc = pthread_create(&threads[i], NULL, algoritmo_substring, (void *)&thread_args[i]);
    }

    /*Esperar a que nuestros threads terminen*/
    for (int i = 0; i < NTHREADS; i++) {
        rc = pthread_join(threads[i], NULL);
    }
    /* TERMINA PARTE DE THREADS */

    for (int i = 1; i < 1001; i++) {
        printf("Sec %d: Posición inicial: %d, Posición final: %d\n", i, secuencias[i].pos_ini, secuencias[i].pos_fin);
    }
    
    printf("El porcentaje de igualdad ante la cadena de referencia es del: %.5f porciento\n", calcular_porcentaje());
    printf("\n%d secuencias mapeadas\n%d secuencias no mapeadas\n", num_secuencias_mapeadas, num_secuencias_no_mapeadas);

    // Thread 1: 1-100
    // for (int i = 1; i <= 100; i++) {
    //     secuencias[i] =  // lo que regrese el algoritmo
    // }

    // free(referencia);  // Todavía en duda si puede ir aquí
    return 0;
}
