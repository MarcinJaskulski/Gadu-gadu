#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define BUF_SIZE 1024
#define NUM_THREADS 5

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
   //TODO
   int nr_deskryptora;
   char message[100];
};

//wskaźnik na funkcję opisującą zachowanie wątku
void *ThreadBehavior(void *t_data)
{
   struct thread_data_t *th_data = (struct thread_data_t *)t_data;
   //dostęp do pól struktury: (*th_data).pole
   //TODO (przy zadaniu 1) klawiatura -> wysyłanie albo odbieranie -> wyświetlanie
   printf("THREAD\n");

   while (1)
   {
      printf("Wpisz wiadomosc: \n");
      // scanf("%s", (*th_data).idFirst);
      // scanf("%s", (*th_data).idSecond);
      scanf("%s", (*th_data).message);
      // write((*th_data).nr_deskryptora, (*th_data).idFirst, sizeof((*th_data).idFirst));
      // write((*th_data).nr_deskryptora, (*th_data).idSecond, sizeof((*th_data).idSecond));
      write((*th_data).nr_deskryptora, (*th_data).message, sizeof((*th_data).message));
   }
   pthread_exit(NULL);
}

//funkcja obsługująca połączenie z serwerem
void handleConnection(int connection_socket_descriptor)
{
   //wynik funkcji tworzącej wątek
   int create_result = 0;
   int n = 0;
   char buf[100];
   printf("HC\n");
   //uchwyt na wątek
   pthread_t thread1;
   int id;
   printf("Podaj swoje ID:\n");
   scanf("%d", &id);
   //dane, które zostaną przekazane do wątku
   struct thread_data_t t_data;
   //TODO
   t_data.nr_deskryptora = connection_socket_descriptor;
   printf("ID: %d\n", id);
   write(connection_socket_descriptor, &id, sizeof(id));
   printf("wyslano id\n");
   create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)&t_data);
   if (create_result)
   {
      printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
      exit(-1);
   }
   while (1)
   {
      n = read(connection_socket_descriptor, buf, sizeof(buf));
      if (n > 0)
      {
         printf("Wiadomosc od ziomka: %s\n", buf);
      }
   }

   // TODO (przy zadaniu 1) odbieranie -> wyświetlanie albo klawiatura -> wysyłanie
   //  while(1){
   //     int n =read(t_data.nr_deskryptora, t_data.wiadomosc2, BUF_SIZE);
   //     if(n>0){
   // 	printf("%s\n", t_data.wiadomosc2);
   //     }
   //  }
}

int main(int argc, char *argv[])
{
   int connection_socket_descriptor;
   int connect_result;
   struct sockaddr_in server_address;
   struct hostent *server_host_entity;

   if (argc != 3)
   {
      fprintf(stderr, "Sposób użycia: %s server_name port_number\n", argv[0]);
      exit(1);
   }

   server_host_entity = gethostbyname(argv[1]);
   if (!server_host_entity)
   {
      fprintf(stderr, "%s: Nie można uzyskać adresu IP serwera.\n", argv[0]);
      exit(1);
   }

   connection_socket_descriptor = socket(PF_INET, SOCK_STREAM, 0);
   if (connection_socket_descriptor < 0)
   {
      fprintf(stderr, "%s: Błąd przy probie utworzenia gniazda.\n", argv[0]);
      exit(1);
   }

   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   memcpy(&server_address.sin_addr.s_addr, server_host_entity->h_addr, server_host_entity->h_length);
   server_address.sin_port = htons(atoi(argv[2]));

   connect_result = connect(connection_socket_descriptor, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
   if (connect_result < 0)
   {
      fprintf(stderr, "%s: Błąd przy próbie połączenia z serwerem (%s:%i).\n", argv[0], argv[1], atoi(argv[2]));
      exit(1);
   }

   handleConnection(connection_socket_descriptor);

   close(connection_socket_descriptor);
   return 0;
}
