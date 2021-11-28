#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>

int main(int argc , char *argv[])
{
    int s;
    struct sockaddr_in server;
    char message[2000] , server_reply[220000];
    int recv_size;

    //Create a socket
    if((s = socket(AF_INET , SOCK_STREAM , 0 )) == -1)
    {
        printf("Could not create socket \n");
    }
    printf("Socket created.\n");


    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    //Connect to remote server
    if (connect(s , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("connect error");
        return 1;
    }
    puts("Connected");

    //Receive a reply from the server
     while (1) {
         if ((recv_size = recv(s, server_reply, 220000, 0)) == -1) {
             puts("recv failed");
             return 1;
         } else {
             server_reply[recv_size] = '\0';
             puts(server_reply);

             if (recv_size >= 50000)
             {
                 return 1;
             }
             
         }

         scanf("%[^\n]%*c", message);

         if (send(s, message, strlen(message), 0) < 0) {
             puts("Send failed");
             return 1;
         } 
     }


    return 0;
}
