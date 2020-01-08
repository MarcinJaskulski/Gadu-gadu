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
#define NUMBER_OF_USERS 100
#define NUMBER_OF_SIGN_IN_MESSAGE 150

//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    int id;                                 // id usera
    int nr_deskryptora;                     // deskryptor usera

    int *logInTab;                          // tablica zalogowanych userów - pod logInTab[id] zapisujemy, czy polaczony - 1, czy nie - 0
    int *deskryptorTab;                     // tablica zawierajaca deskryptowy userow - odowłujemy się po id usera
    pthread_mutex_t *mutexForUserTab;       // tablica mutexow dla kazdego usera

    char idFirst[4];                        // skąd wysyłamy wiadomość
    char idSecond[4];                       // dokąd wysyłamy wiadomość
    char message[NUMBER_OF_SIGN_IN_MESSAGE];// treść wiadomości
};

// potegowanie
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

// zamiana char na int
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

// funkcja rozgłasza kto jest obecnie podłaczony do serwera
void whoIs(int id, int deskryptor, int *logInTab, int *deskryptorTab, pthread_mutex_t *mutexForUserTab){
    int n = 0;
    char c[15] = "";
    char tc[15] = "";
    strcat(c, "#yourId{");
    sprintf(tc, "%d", id);
    strcat(c, tc);
    strcat(c, "}\n");

    // jeśli user się rozlączył, to nie może wysłać sobie wiadomości

    if(id != -1){
        pthread_mutex_lock(&mutexForUserTab[id]);
        n = write(deskryptor, c, sizeof(c));
        pthread_mutex_unlock(&mutexForUserTab[id]);
        if(n < 0){
            fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
        }
    }

    char friends[50] = "";
    strcat(friends,"#friends{");

    for(int i=0; i<NUMBER_OF_USERS; i++){
        if(logInTab[i] == 1){
            sprintf(c, "%d", i);
            strcat(friends, c);
            strcat(friends, ";");
        }
    }

    strcat(friends,"}\n");

    for(int i=0; i<NUMBER_OF_USERS; i++){

        if(logInTab[i] == 1){
            pthread_mutex_lock(&mutexForUserTab[i]);
            n = write(deskryptorTab[i], friends, sizeof(friends));
            pthread_mutex_unlock(&mutexForUserTab[i]);
            if(n < 0){
                fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
            }
        }
    }
}

// wysyłanie wiadomości
void handleWrite(struct thread_data_t sendData, int *logInTab, int *deskryptorTab, pthread_mutex_t *mutexForUserTab)
{
    int n = 0;
    char message[NUMBER_OF_SIGN_IN_MESSAGE] = " \n";
    strcat(message, "#fromId{");

    if (logInTab[charToInt(sendData.idSecond)] == 1)
    {
        int idRecipient = charToInt(sendData.idSecond);
        int deskryptor = deskryptorTab[idRecipient];

        strcat(message, sendData.idFirst);
        strcat(message, "}");
        strcat(message, "#message{");
        strcat(message, sendData.message);
        strcat(message, "}\n");

        pthread_mutex_lock(&mutexForUserTab[idRecipient]);
        n = write(deskryptor, message, sizeof(message));
        pthread_mutex_unlock(&mutexForUserTab[idRecipient]);

        if(n < 0){
            fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
        }
    }
}

//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ReadThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());
    struct thread_data_t th_data = *((struct thread_data_t *)t_data);
    free(t_data);

    int *logInTab =th_data.logInTab;
    int *deskryptorTab = th_data.deskryptorTab;
    pthread_mutex_t *mutexForUserTab = th_data.mutexForUserTab;
    int n = 0;
    char bufMes[NUMBER_OF_SIGN_IN_MESSAGE];

    // zanim watek bedzie czekal na info o wiadomosciach, to wysyla odpowedz do zalogowania
    whoIs(th_data.id, th_data.nr_deskryptora, logInTab, deskryptorTab, mutexForUserTab);

    while (1)
    {
        n = read(th_data.nr_deskryptora, &bufMes, sizeof(bufMes));

        if (n > 0)
        {
            // jesli user sie rozlacza
            char end[4] = "";
            strncpy(end, bufMes, 4);
            if(!strcmp("#end", bufMes)){
                break;
            }

            strncpy(th_data.idFirst, bufMes, 3);
            th_data.idFirst[3] = '\0';
            strncpy(th_data.idSecond, bufMes + 3, 3);
            th_data.idSecond[3] = '\0';
            strncpy(th_data.message, bufMes + 6, sizeof(bufMes)-6);
            th_data.message[sizeof(bufMes)-6] = '\0';

            handleWrite(th_data, logInTab, deskryptorTab, mutexForUserTab);
        }
        // nastąpiło rozłaczenie
        if(n == 0){
            break;
        }
        if(n < 0){
            fprintf(stderr, "Niepoprawny odczyt.\n");
        }
        n = 0;
    }

    //jesli przypadkiem zostało rozłączone gdy był zablokowany mutex
    pthread_mutex_unlock(&mutexForUserTab[th_data.id]);
    deskryptorTab[th_data.id] = 0;
    logInTab[th_data.id] = 0;

    whoIs(-1, th_data.nr_deskryptora, logInTab, deskryptorTab, mutexForUserTab);

    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor, int id, int *logInTab, int *deskryptorTab,pthread_mutex_t *mutexForUserTab)
{
    //uchwyt na wątek
    int create_result = 0;
    struct thread_data_t *t_data = malloc(sizeof(struct thread_data_t)); //malloc + zwolnienie na końcu (watek klienta)

    pthread_t readThread;
    t_data->nr_deskryptora = connection_socket_descriptor;
    t_data->id = id;
    t_data->logInTab = logInTab;
    t_data->deskryptorTab = deskryptorTab;
    t_data->mutexForUserTab = mutexForUserTab;


    create_result = pthread_create(&readThread, NULL, ReadThreadBehavior, t_data);
    if (create_result)
    {
        fprintf(stderr, "Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
        exit(-1);
    }
}

void init(int *logInTab, int *deskryptorTab)
{
    for (int i = 0; i < NUMBER_OF_USERS; i++)
    {
        logInTab[i] = 0;
        deskryptorTab[i] = 0;
    }
}

int main(int argc, char *argv[])
{
    int *logInTab = malloc(NUMBER_OF_USERS * sizeof(int));
    int *deskryptorTab = malloc(NUMBER_OF_USERS * sizeof(int));
    init(logInTab, deskryptorTab);

    pthread_mutex_t *mutexForUserTab = malloc(NUMBER_OF_USERS * sizeof(pthread_mutex_t));

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
    int busySpace = 0;
    //accepted client
    while (1)
    {
        busySpace = 0;
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        // 0 - ok; <0 -blad
        if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }

        // pierwszy wolny id dla kolejnego
        for(int i=0; i<=NUMBER_OF_USERS; i++){
            if(i == NUMBER_OF_USERS){
                int n = 0;

                pthread_mutex_lock(&mutexForUserTab[clientId]);
                n = write(connection_socket_descriptor, "#busySpace\n", sizeof("#busySpace\n"));
                pthread_mutex_unlock(&mutexForUserTab[clientId]);

                if(n < 0){
                    fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
                }
                busySpace =1;
            }
            if(logInTab[i] == 0){
                clientId = i;
                break;
            }
        }
        if(busySpace==1){
            busySpace=0;
            continue;
        }

        printf("Client id: %d\n", clientId);
        logInTab[clientId] = 1; // podanie, ze jest zalogowany

        deskryptorTab[clientId] = connection_socket_descriptor; // zapisanie w tablicy deskryptorow

        handleConnection(connection_socket_descriptor, clientId, logInTab, deskryptorTab, mutexForUserTab);
    }

    close(server_socket_descriptor);

    free(logInTab);
    free(deskryptorTab);

    return (0);
}
