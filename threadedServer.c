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

//    // --- PAMIEC WSPOLDZIELONA ---
//    // -- Dla logIn
//    shmid = shmget(IPC_PRIVATE, 100*sizeof(int), IPC_CREAT|0600);
//    if (shmid == -1){
//       perror("Blad: Utworzenie segmentu pamieci wspoldzielonej");
//       exit(1);
//    }
   
//    buf = (int*)shmat(shmid, NULL, 0);
//    if (buf == NULL){
//       perror("Blad: Przylaczenie segmentu pamieci wspoldzielonej");
//       exit(1);
//    }
   
//    buf[0] = 0; // ilosc wyprodukowanych dziel
//    buf[1] = 0; // startowa liczba osob w czytelnii
//    buf[2] = 0; // liczba dziel na polce




//struktura zawierająca dane, które zostaną przekazane do wątku
struct thread_data_t
{
    int id;
    int nr_deskryptora;

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

// funkcja rozgłasza kto jest obecnie podłaczony do serwera
void whoIs(int id, int deskryptor){
    int n = 0;
    char c[15] = "";
    char tc[15] = "";
    strcat(c, "#yourId{");
    sprintf(tc, "%d", id);
    strcat(c, tc);
    strcat(c, "}\n");
    printf("%s\n", c);
    n = write(deskryptor, c, sizeof(c));

    if(n < 0){
        fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
    }

    char friends[50];
    strcat(friends,"#friends{");
    for(int i=0; i<sizeof(logIn)/sizeof(logIn[0]); i++){
        if(logIn[i] == 1){
            sprintf(c, "%d", i);
            strcat(friends, c);
            strcat(friends, ";");
        }
    }
    strcat(friends,"}\n");
    printf("%s\n", friends);

    for(int i=0; i<sizeof(logIn)/sizeof(logIn[0]); i++){
        if(logIn[i] == 1){
            n = write(idDeskryptor[i], friends, sizeof(friends));
            if(n < 0){
                fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
            }
        }
    }
}

// wysyłanie wiadomości
void handleWrite(struct thread_data_t *sendData)
{
    //printf("%s\n",sendData->idFirst);
    int n = 0;
    char message[100] = "";
    strcat(message, "#fromId{");

    if (logIn[charToInt((*sendData).idSecond)] == 1)
    {

        int deskryptor = idDeskryptor[charToInt((*sendData).idSecond)];

        strcat(message, sendData->idFirst);
        strcat(message, "}");

        strcat(message, "#message{");
        strcat(message, sendData->message);
        strcat(message, "}\n");
        n = write(deskryptor, message, sizeof(message));
        if(n < 0){
            fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
        }
        printf("Mes: %s\n", message);
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
    whoIs(th_data->id, (*th_data).nr_deskryptora);



    while (1)
    {
        n = read((*th_data).nr_deskryptora, &bufMes, sizeof(bufMes));
        printf("Mes: %s\n", bufMes);
        if (n > 0)
        {
            // jesli user sie rozlacza
            char end[4] = "";
            strncpy(end, bufMes, 4);
            if(!strcmp("#end", bufMes)){
                break;
            }

            //printf("SendData0: %s\n", bufMes);
            strncpy((*th_data).idFirst, bufMes, 3);
            (*th_data).idFirst[3] = '\0';
            strncpy((*th_data).idSecond, bufMes + 3, 3);
            (*th_data).idSecond[3] = '\0';
            strncpy((*th_data).message, bufMes + 6, sizeof(bufMes)-6);
            (*th_data).message[sizeof(bufMes)-6] = '\0';

            //printf("SendData1: %s\n", (*th_data).message);

            memcpy(sendData->idFirst, th_data->idFirst, sizeof(sendData->idFirst));
            memcpy(sendData->idSecond, th_data->idSecond, sizeof(sendData->idSecond));            
            memcpy(sendData->message, th_data->message, sizeof(sendData->message));
            //printf("SendData2: %s\n", sendData->message);
            handleWrite(sendData);
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

    printf("Desk: %s\n", (*th_data).idFirst);
    logIn[(*th_data).id] = 0;
    whoIs(-1, (*th_data).nr_deskryptora);

    free(t_data);
    free(sendData); 
    //free(th_data); // wystarczy jedno, bo ta sama komorka pamieci
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor, int id)
{
    //uchwyt na wątek
    int create_result = 0;
    
    struct thread_data_t *t_data = malloc(sizeof(struct thread_data_t)); //malloc + zwolnienie na końcu (watek klienta)
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

        //nasłuchiwanie na nowego clienta. Jesli jest nowy to stworzy mu watek
        //goodRead = read(connection_socket_descriptor, NULL, 0);

        // pierwszy wolny id dla kolejnego
        for(int i=0; i<=100; i++){
            if(i == 100){
                int n = 0;
                n = write(connection_socket_descriptor, "#busySpace", sizeof("#busySpace"));
                if(n < 0){
                    fprintf(stderr, "Niepoprawne wyslanie wiadomosci.\n");
                }
                busySpace =1;
            }
            if(logIn[i] == 0){
                clientId = i;
                break;
            }
        }
        if(busySpace==1){
            busySpace=0;
            continue;
        }

        printf("Client id: %d\n", clientId);
        logIn[clientId] = 1; //podanie, ze jest zalogowany

        idDeskryptor[clientId] = connection_socket_descriptor; //zapisanie w tablicy deskryptorow

        handleConnection(connection_socket_descriptor, clientId);

    }

    close(server_socket_descriptor);

    return (0);
}
