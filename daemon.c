#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

static void daemonize() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    /*TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(027);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    // chdir("/Users/jorgemiguelsotorodriguez/progrAva");
    chdir("/Users/davidhdz/Documents/repos/proyecto-final-programacion-avanzada");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    /* Open the log file */
    openlog("firstdaemon", LOG_PID, LOG_DAEMON);
}

#define NTHREADS 10
pthread_mutex_t lock;
char *refFileName, *seqFileName;

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

posicion posicion_secuencias[1001];  // 1001 por que las secuencias se van a guardar del 1 al 1000
// int pos_next_secuencia = 1;                                       // Empieza en la posición 1 porque la posición 0 no la vamos a usar
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
    int pos_next_secuencia = limite.lim_inf;

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
                    pos.pos_ini = i + 1;
                    pos.pos_fin = k + 1;
                    posicion_secuencias[pos_next_secuencia] = pos;
                    encontrado = 1;
                    num_secuencias_mapeadas++;
                }

                if (encontrado == 1)
                    break;
            }

            if (encontrado == 0) {
                pos.pos_ini = -1;
                pos.pos_fin = -1;
                posicion_secuencias[pos_next_secuencia] = pos;
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
    free(secuencia);
    return NULL;
}

//Funcion para ordenar los limites de las posiciones iniciales por el metodo burbuja
void ordena_estructura_bsort() {
    int i, j;
    posicion temp;
    int tama = 1001;

    for (i = 1; i <= tama; i++) {
        for (j = i + 1; j <= tama; j++) {
            if (posicion_secuencias[i].pos_ini > posicion_secuencias[j].pos_ini) {
                temp = posicion_secuencias[i];
                posicion_secuencias[i] = posicion_secuencias[j];
                posicion_secuencias[j] = temp;
            }
        }
    }
}

//funcion que calcula el porcentaje de la similitud entre las cadenas y la referencia
float calcular_porcentaje() {
    ordena_estructura_bsort();

    int tama = 1001;
    long acumulado_caracteres = 0;

    //para que no haya traslapes modificaremos algunos rangos
    for (int i = tama - 1; i >= 1 && posicion_secuencias[i].pos_fin != -1; i--) {
        if (posicion_secuencias[i].pos_ini < posicion_secuencias[i - 1].pos_fin) {
            posicion_secuencias[i].pos_ini = posicion_secuencias[i - 1].pos_fin;
        }
    }

    for (int j = tama - 1; j >= 1 && posicion_secuencias[j].pos_fin != -1; j--) {
        acumulado_caracteres = acumulado_caracteres + (posicion_secuencias[j].pos_fin - posicion_secuencias[j].pos_ini);
    }

    return ((acumulado_caracteres * 100) / lsize_referencia);
}

int main() {
    printf("Starting daemonize\n");
    daemonize();
    FILE *fp = NULL;
    fp = fopen("Log.txt", "w+");
    if (fp != NULL) {
        while (1) {
            int s, new_socket;
            struct sockaddr_in server, client;
            int c;
            char *message;

            //Create a socket
            // Type stream tcp
            if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                printf("Error creating socket");
            }

            printf("Socket created.\n");
            //Prepare the sockaddr_in structure
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = INADDR_ANY;
            server.sin_port = htons(8888);
            //Bind
            if (bind(s, (struct sockaddr *)&server, sizeof(server)) == -1) {
                printf("Bind error\n");
                exit(EXIT_FAILURE);
            }
            puts("Bound");
            listen(s, 3);
            puts("Waiting for connections...");
            c = sizeof(struct sockaddr_in);
            //---------------
            int iOpc = 999, recv_size;
            int flagRef = 0, flagSeq = 0;
            char client_reply[2000];
            FILE *fileRef;
            FILE *fileSeq;
            //---------------

            while ((new_socket = accept(s, (struct sockaddr *)&client, &c)) !=
                   -1) {
                puts("Connection accepted \n");
                do {
                    message = "Opciones \n (1) Upload_reference\n (2) Upload_sequence \n (3) Ejecutar";
                    send(new_socket, message, strlen(message), 0);

                    // Opciones
                    // (1) Upload_reference
                    // (2) Upload_sequence
                    // (3) Ejecutar

                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                        puts("recv failed");
                    } else {
                        client_reply[recv_size] = '\0';
                        iOpc = atoi(client_reply);
                    }

                    switch (iOpc) {
                            // (1) Upload_reference
                        case 1:
                            //Si aun no hay un archivo guardado.
                            if (flagRef == 0) {
                                message = "Cual es el nombre del archivo Reference ?";
                                send(new_socket, message, strlen(message), 0);

                                if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                    puts("recv failed");
                                } else {
                                    client_reply[recv_size] = '\0';
                                    refFileName = client_reply;
                                    printf("%s \n", refFileName);
                                }

                                fileRef = fopen(refFileName, "rb");
                                if (fileRef != NULL) {
                                    message = "Archivo Guardado. (1)\nPresione 1 para continuar ";
                                    fclose(fileRef);
                                    flagRef = 1;
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    } else {
                                        memset(client_reply, 0, sizeof(client_reply));
                                    }
                                } else {
                                    message = "El archivo no existe. (1)\nPresione 1 para continuar ";
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    }
                                }
                            } else {
                                //Si ya hay un archivo guardado
                                message = "Ya tienes un archivo de referencia guardado. (1)\nPresione 1 para continuar ";
                                send(new_socket, message, strlen(message), 0);
                                if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                    puts("recv failed");
                                }
                            }

                            break;

                            // (2) Upload_sequence
                        case 2:
                            if (flagSeq == 0) {
                                message = "Cual es el nombre del archivo Sequence ?";
                                send(new_socket, message, strlen(message), 0);

                                if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                    puts("recv failed");
                                } else {
                                    seqFileName = client_reply;
                                    printf("%s \n", seqFileName);
                                }

                                fileSeq = fopen(seqFileName, "rb");
                                if (fileSeq != NULL) {
                                    message = "Archivo Guardado. (1)\nPresione 1 para continuar ";
                                    fclose(fileSeq);
                                    flagSeq = 1;
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    } else {
                                        memset(client_reply, 0, sizeof(client_reply));
                                    }
                                } else {
                                    message = "El archivo no existe. (1)\nPresione 1 para continuar ";
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    }
                                }
                            } else {
                                //Si ya hay un archivo guardado
                                message = "Ya tienes un archivo de sequencia guardado. (1)\nPresione 1 para continuar ";
                                send(new_socket, message, strlen(message), 0);
                                if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                    puts("recv failed");
                                }
                            }

                            break;

                            // (3) Ejecutar
                        case 3:
                            if (flagSeq == 1 && flagRef == 1) {
                                memset(client_reply, 0, sizeof(client_reply));
                                message = "Seguro que quieres ejecutar el programa? Si: 1, No: 0";
                                send(new_socket, message, strlen(message), 0);
                                if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                    puts("recv failed");
                                } else {
                                    if (atoi(client_reply) == 1) {
                                        posicion posicion_dummy = {-1, -1};
                                        /*posicion_dummy es porque vamos a guardar las secuencias de la posición 1 a la 1000, ya que
                                         son 1000 secuencias*/
                                        posicion_secuencias[0] = posicion_dummy;

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
                                        free(referencia);
                                        /* TERMINA PARTE DE THREADS */
                                        char response[220000] = "";
                                        char temp[50] = "";

                                        for (int i = 1; i < 1001; i++) {
                                            strcat(response, "Secuencia #");
                                            sprintf(temp, "%d", i);
                                            strcat(response, temp);
                                            memset(temp, 0, sizeof(temp));
                                            if (posicion_secuencias[i].pos_ini == -1) {
                                                strcat(response, " No se encontro en la referencia\n");
                                            } else {
                                                strcat(response, ": Posicion inicial: ");
                                                sprintf(temp, "%d", posicion_secuencias[i].pos_ini);
                                                strcat(response, temp);
                                                memset(temp, 0, sizeof(temp));
                                                strcat(response, ", ");
                                                strcat(response, "Posición final: ");
                                                sprintf(temp, "%d", posicion_secuencias[i].pos_fin);
                                                strcat(response, temp);
                                                memset(temp, 0, sizeof(temp));
                                                strcat(response, "\n");
                                                // printf("Sec %d: Posición inicial: %d, Posición final: %d\n", i, posicion_secuencias[i].pos_ini, posicion_secuencias[i].pos_fin);
                                            }
                                        }
                                        float porcentaje = calcular_porcentaje();
                                        strcat(response, "\n");
                                        strcat(response, "El porcentaje de igualdad ante la cadena de referencia es del: ");
                                        sprintf(temp, "%.5f", porcentaje);
                                        strcat(response, temp);
                                        memset(temp, 0, sizeof(temp));
                                        strcat(response, "\n");
                                        sprintf(temp, "%d", num_secuencias_mapeadas);
                                        strcat(response, temp);
                                        memset(temp, 0, sizeof(temp));
                                        strcat(response, " secuencias mapeadas \n");
                                        sprintf(temp, "%d", num_secuencias_no_mapeadas);
                                        strcat(response, temp);
                                        memset(temp, 0, sizeof(temp));
                                        strcat(response, " secuencias no mapeadas \n");
                                        send(new_socket, response, strlen(response), 0);
                                        return EXIT_SUCCESS;
                                        //printf("\n\nEl porcentaje de igualdad ante la cadena de referencia es del: %.5f%%", calcular_porcentaje());
                                        //printf("\n%d secuencias mapeadas\n%d secuencias no mapeadas\n", num_secuencias_mapeadas, num_secuencias_no_mapeadas);
                                    }
                                }
                            } else {
                                if (flagRef == 0 && flagSeq == 0) {
                                    message = "No ha ingresado ningun archivo. (1)\nPresione 1 para continuar ";
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    }
                                } else if (flagRef == 0 && flagSeq == 1) {
                                    message = "No ha ingresado el archivo de referencia. (1)\nPresione 1 para continuar ";
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    }

                                } else if (flagRef == 1 && flagSeq == 0) {
                                    message = "No ha ingresado el archivo de sequencia. (1)\nPresione 1 para continuar ";
                                    send(new_socket, message, strlen(message), 0);
                                    if ((recv_size = recv(new_socket, client_reply, 2000, 0)) == -1) {
                                        puts("recv failed");
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                    }

                } while (iOpc != 0);
                if (iOpc == 0) {
                    message = "Bye";
                    send(new_socket, message, strlen(message), 0);
                    return 1;
                }
            }
            if (new_socket == -1) {
                printf("accept failed \n");
                return 1;
            }

            syslog(LOG_NOTICE, "First daemon running.");
            fflush(fp);
        }
    }
    fclose(fp);
    syslog(LOG_NOTICE, "First daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
