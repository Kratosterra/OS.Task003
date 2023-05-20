#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

// ip 10.0.2.15
// port 8080

int server_fd;

void handler(int signum)
{
    printf("Получен сигнал, завершаем работу!\n");
    close(server_fd);
    exit(0);
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
    int num_clients = atoi(argv[3]);
    FILE *infile, *outfile;
    double a = 0.0;
    double b = 0.0;

    int new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    double total_area = 0.0;
    double subrange_start = 0.0;

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

    double subrange_size = (b - a) / num_clients;

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

    if (listen(server_fd, num_clients) < 0)
    {
        perror("Ошибка слушанья.\n");
        exit(EXIT_FAILURE);
    }

    printf("Агроном ждет прихода счетоводов...\n");
    subrange_start = a;
    int all_clients = num_clients;
    while (num_clients > 0)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("Подключение провалено.\n");
            exit(EXIT_FAILURE);
        }

        printf("Счетовод пришел за заданием!\n");

        double client_subrange_start = subrange_start;
        double client_subrange_end = subrange_start + subrange_size;

        send(new_socket, &client_subrange_start, sizeof(double), 0);
        send(new_socket, &client_subrange_end, sizeof(double), 0);
        send(new_socket, &all_clients, sizeof(int), 0);

        double client_result;
        valread = read(new_socket, &client_result, sizeof(double));
        total_area += client_result;

        fprintf(outfile, "Счетовод [%d] подсчитал площадь своей территории: %.6f кв.м\n", all_clients - num_clients + 1, client_result);
        printf("Счетовод [%d] подсчитал площадь своей территории: %.6f кв.м\n", all_clients - num_clients + 1, client_result);

        close(new_socket);
        num_clients--;
        subrange_start += subrange_size;
    }

    fprintf(outfile, "Агроном и счетоводы получили общую площадь: %.6f кв.м\n", total_area);
    printf("Агроном и счетоводы получили общую площадь: %.6f кв.м\nПодробнее в файле вывода %s\n", total_area, argv[5]);
    close(server_fd);
    return 0;
}