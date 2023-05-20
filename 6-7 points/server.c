#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

// ip 10.0.2.15
// port 8080

int server_fd;
pthread_mutex_t mutex;
FILE *infile, *outfile;
double total_area = 0.0;
double subrange_start = 0.0;
double subrange_size;
int all_clients;
int num_clients;
int observer_socket;

void handler(int signum)
{
    printf("Получен сигнал, завершаем работу!\n");
    pthread_exit(NULL);
    pthread_mutex_destroy(&mutex);
    close(server_fd);
    exit(0);
}

void *handleClient(void *args)
{
    int new_socket = *(int *)args;
    if (num_clients <= 0)
    {
        close(new_socket);
        free(args);
        return NULL;
    }

    num_clients--;
    pthread_mutex_lock(&mutex);

    double client_subrange_start = subrange_start;
    double client_subrange_end = subrange_start + subrange_size;
    subrange_start += subrange_size;
    double client_result;
    send(new_socket, &client_subrange_start, sizeof(double), 0);
    send(new_socket, &client_subrange_end, sizeof(double), 0);

    pthread_mutex_unlock(&mutex);

    send(new_socket, &all_clients, sizeof(int), 0);
    read(new_socket, &client_result, sizeof(double));
    fprintf(outfile, "Счетовод [%d] подсчитал площадь своей территории: %.6f кв.м\n", all_clients - num_clients, client_result);
    printf("Счетовод [%d] подсчитал площадь своей территории: %.6f кв.м\n", all_clients - num_clients, client_result);

    pthread_mutex_lock(&mutex);
    total_area += client_result;
    send(observer_socket, &client_subrange_start, sizeof(double), 0);
    send(observer_socket, &client_subrange_end, sizeof(double), 0);
    send(observer_socket, &client_result, sizeof(double), 0);
    send(observer_socket, &total_area, sizeof(double), 0);
    send(observer_socket, &num_clients, sizeof(int), 0);
    pthread_mutex_unlock(&mutex);

    close(new_socket);
    free(args);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Использованиие: %s <ip адрес> <порт> <кол-во счетоводов> <входной файл> <выходной файл>\n", argv[0]);
        return 1;
    }
    signal(SIGINT, handler);
    char *ip = argv[1];
    int port = atoi(argv[2]);
    num_clients = atoi(argv[3]);
    double a = 0.0;
    double b = 0.0;

    int new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((infile = fopen(argv[4], "r")) == NULL)
    {
        perror("Ошибка при открытии входного файла!\n");
        exit(1);
    }
    if ((outfile = fopen(argv[5], "w")) == NULL)
    {
        perror("Ошибка при открытии выходного файла!\n");
        exit(1);
    }
    printf("Cчитываем входные данные...\n");
    if (fscanf(infile, "%lf %lf", &a, &b) != 2)
    {
        printf("Ошибка при чтении входных данных, убедитесь, что в файле ввода только 2 double числа.\n");
        exit(1);
    }
    if (a < 0 || b < 0)
    {
        printf("Ошибка при чтении входных данных, убедитесь, что числа неотрицательные!\n");
        exit(1);
    }

    subrange_size = (b - a) / num_clients;

    printf("Пытаемся создать сокет...\n");
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Ошибка при создания сокета.\n");
        exit(EXIT_FAILURE);
    }

    printf("Пытаемся настроить сокет...\n");
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("Ошибка при настройке сокета.\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    printf("Пытаемся подключить сокет к ip и порту...\n");
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Ошибка при настройке сокета.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, num_clients + 1) < 0)
    {
        perror("Ошибка слушанья.\n");
        exit(EXIT_FAILURE);
    }

    printf("Агроном ждет прихода наблюдателя...\n");

    if ((observer_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("Ошибка при подключении наблюдателя!");
        exit(EXIT_FAILURE);
    }

    printf("Наблюдатель подключился!\n");

    send(observer_socket, &a, sizeof(double), 0);
    send(observer_socket, &b, sizeof(double), 0);
    send(observer_socket, &num_clients, sizeof(int), 0);

    printf("Агроном ждет прихода счетоводов...\n");
    pthread_mutex_init(&mutex, NULL);
    subrange_start = a;
    all_clients = num_clients;
    while (num_clients > 0)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Подключение провалено.\n");
            exit(EXIT_FAILURE);
        }
        printf("Счетовод пришел за заданием!\n");
        pthread_t thread;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        if (pthread_create(&thread, NULL, handleClient, client_socket) != 0)
        {
            perror("Ошибка при создании потока клиента.\n");
            exit(EXIT_FAILURE);
        }
    }
    fprintf(outfile, "Агроном и счетоводы получили общую площадь: %.6f кв.м\n", total_area);
    printf("Агроном и счетоводы получили общую площадь: %.6f кв.м\nПодробнее в файле вывода %s\n", total_area, argv[5]);
    pthread_exit(NULL);
    pthread_mutex_destroy(&mutex);
    close(server_fd);
    return 0;
}