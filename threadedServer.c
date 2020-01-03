#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <math.h>

#define SERVER_PORT 1234
#define QUEUE_SIZE 5

int logIn[100]; //main i wskaźnik

int idDeskryptor[100];

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    int id;
    int nr_deskryptora;
    char wiadomosc1[1024];
    char wiadomosc2[1024];
    int *tab;

    char idFirst[4];
    char idSecond[4];
    char message[100];
};

int int_pow(int base, int exp)
{
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp /= 2;
        base *= base;
    }
    return result;
}

int charToInt(char tab[4])
{
    int res = 0;
    for (int i = 0; i < 3; i++)
    {
        if (tab[i] != '0')
        {
            res = res + int_pow(10, 2 - i) * atoi(&tab[i]);
        }
    }

    return res;
}




// void SendWhoIs(void *t_data)
// {
//     struct thread_data_t *th_data = (struct thread_data_t *)t_data;
//     printf("Send: %d \n", charToInt((*th_data).idSecond));
//     while (logIn[charToInt((*th_data).idSecond)] == 0)
//     {
//         // printf("uzytkownik nie jest zalogowany");
//     }

//     if (logIn[charToInt((*th_data).idSecond)] == 1)
//     {

//         int deskryptor = idDeskryptor[charToInt((*th_data).idSecond)];

//         write(deskryptor, (*th_data).message, sizeof((*th_data).message));
//         printf("Mes: %s\n", (*th_data).message);
//     }
// }

void userOnServer(){
    //int id = 
}




// void SendThreadBehavior(void *t_data)
// {
//     struct thread_data_t *th_data = (struct thread_data_t *)t_data;
//     printf("Send: %d \n", charToInt((*th_data).idSecond));
//     while (logIn[charToInt((*th_data).idSecond)] == 0)
//     {
//         // printf("uzytkownik nie jest zalogowany");
//     }

//     if (logIn[charToInt((*th_data).idSecond)] == 1)
//     {

//         int deskryptor = idDeskryptor[charToInt((*th_data).idSecond)];

//         write(deskryptor, (*th_data).message, sizeof((*th_data).message));
//         printf("Mes: %s\n", (*th_data).message);
//     }
//     pthread_exit(NULL);
// }

void handleWrite(struct thread_data_t *sendData)
{
    printf("%s\n",sendData->idFirst);

    if (logIn[charToInt((*sendData).idSecond)] == 1)
    {
        int deskryptor = idDeskryptor[charToInt((*sendData).idSecond)];

        write(deskryptor, (*sendData).message, sizeof((*sendData).message));
        printf("Mes: %s\n", (*sendData).message);
    }
}

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ReadThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t *th_data = (struct thread_data_t *)t_data;
    struct thread_data_t *sendData = malloc(sizeof(struct thread_data_t));
    int n = 0;
    char bufMes[1000];
    printf("%d\n",th_data->nr_deskryptora);

    // zanim watek bedzie czekal na info o wiadomosciach, to wysyla odpowedz do zalogowania
    char c[15];
    char tc[15];
    strcat(c, "#yourID{");
    sprintf(tc, "%d", th_data->id);
    strcat(c, tc);
    strcat(c, "}\n");
    printf("%s\n", c);
    write((*th_data).nr_deskryptora, c, sizeof(c));

    char friends[50];
    strcat(friends,"#friends{");
    for(int i=0; i<sizeof(logIn)/sizeof(logIn[0]); i++){
        if(logIn[i] == 1 && i != th_data->id){
            sprintf(c, "%d", i);
            strcat(friends, c);
            strcat(friends, ";");
        }
    }
    strcat(friends,"}\n");
    printf("%s\n", friends);

    write((*th_data).nr_deskryptora, friends, sizeof(friends));


    while (1)
    {
        n = read((*th_data).nr_deskryptora, &bufMes, sizeof(bufMes));
        if (n > 0)
        {
            strncpy((*th_data).idFirst, bufMes, 3);
            (*th_data).idFirst[3] = '\0';
            strncpy((*th_data).idSecond, bufMes + 3, 3);
            (*th_data).idSecond[3] = '\0';
            strncpy((*th_data).message, bufMes + 6, 10);
            (*th_data).message[3] = '\0';

            // printf("Id First: %s\n", (*th_data).idFirst);
            // printf("Id Sec: %s\n", (*th_data).idSecond);
            // printf("Id Mes: %s\n", (*th_data).message);

            memcpy(sendData->idFirst, th_data->idFirst, sizeof(sendData->idFirst));
            memcpy(sendData->idSecond, th_data->idSecond, sizeof(sendData->idSecond));
            memcpy(sendData->message, th_data->message, sizeof(sendData->message));
            printf("SendData: %s\n", sendData->message);
            handleWrite(sendData);
        }
        n = 0;
    }

    printf("Desk: %s\n", (*th_data).idFirst);
    logIn[(*th_data).nr_deskryptora] = 0;
    free(t_data);
    free(sendData);
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor, int id)
{
    //uchwyt na wątek
    int create_result;
    
    struct thread_data_t *t_data=malloc(sizeof(struct thread_data_t)); //malloc + zwolnienie na końcu (watek klienta)
    pthread_t readThread;
    t_data->nr_deskryptora = connection_socket_descriptor;
    t_data->id = id;


    create_result = pthread_create(&readThread, NULL, ReadThreadBehavior, (void *)t_data);
    if (create_result)
    {
        printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
        exit(-1);
    }
}

void init()
{
    for (int i = 0; i < sizeof(logIn) / sizeof(logIn[0]); i++)
    {
        logIn[i] = 0;
        idDeskryptor[i] = 0;
    }
}

int main(int argc, char *argv[])
{
    init();

    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;

    //inicjalizacja gniazda serwera

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
        exit(1);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr_val, sizeof(reuse_addr_val));

    bind_result = bind(server_socket_descriptor, (struct sockaddr *)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
        exit(1);
    }

    listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
        exit(1);
    }

    int clientId = 0;
    //accepted client
    while (1)
    {
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }

        //nasłuchiwanie na nowego clienta. Jesli jest nowy to stworzy mu watek
        read(connection_socket_descriptor, NULL, 0);
        printf("Client id: %d\n", clientId);
        logIn[clientId] = 1; //podanie, ze jest zalogowany

        idDeskryptor[clientId] = connection_socket_descriptor; //zapisanie w tablicy deskryptorow


        handleConnection(connection_socket_descriptor, clientId);
        clientId++;
    }

    close(server_socket_descriptor);

    return (0);
}
