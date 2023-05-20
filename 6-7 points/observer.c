#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Использование: %s <ip адрес> <порт>\n", argv[0]);
        return 1;
    }
    char *ip = argv[1];
    int port = atoi(argv[2]);
    int observer_fd;
    struct sockaddr_in serv_addr;
    int valread;
    if ((observer_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Ошибка при создании сокета.");
        exit(EXIT_FAILURE);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(serv_addr.sin_addr)) <= 0)
    {
        perror("Ошибка при конвертации");
        exit(EXIT_FAILURE);
    }
    if (connect(observer_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Не удалось подключится к серверу");
        exit(EXIT_FAILURE);
    }
    double a = 0;
    double b = 0;
    int all_clients = 0;
    int num_clients = 0;
    printf("Наблюдатель подключился. Ожидаем обновлений...\n");
    recv(observer_fd, &a, sizeof(double), 0);
    recv(observer_fd, &b, sizeof(double), 0);
    recv(observer_fd, &all_clients, sizeof(int), 0);
    printf("Общая информация.\na = %f\nb = %f\nКол-во клиентов: %d\n\n", a, b, all_clients);
    while (1)
    {
        double subrange_start, subrange_end, client_result, total_area;
        valread = recv(observer_fd, &subrange_start, sizeof(double), 0);
        if (valread == 0)
        {
            printf("Сервер отключён.\n");
            break;
        }
        recv(observer_fd, &subrange_end, sizeof(double), 0);
        recv(observer_fd, &client_result, sizeof(double), 0);
        recv(observer_fd, &total_area, sizeof(double), 0);
        recv(observer_fd, &num_clients, sizeof(int), 0);
        printf("Клиент №%d\n", all_clients - num_clients);
        printf("Область [%f, %f]\nКлиент выдал результат: %f\nОбщая площадь: %f\n\n",
               subrange_start, subrange_end, client_result, total_area);
    }
    close(observer_fd);
    return 0;
}