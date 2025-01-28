#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

#define COURIERS 3
#define PRODUCT 3
#define MAGAZINES 3
#define SHM_SIZE 1024

#define MAGAZINE_NUMBER 1

// to jako argumenty
#define KEY 5555

struct magazine {
    int magazine_gold;
    int courier_active[COURIERS];
    int magazine_active;
    int product_number[PRODUCT];
    int product_cost[PRODUCT];
};

struct control_room {
    int control_room_gold;
    int control_room_active;
};

struct shared_data {
    struct magazine magazines[MAGAZINES];
    struct control_room ctrl_room;
};

struct order_info {
    int product[PRODUCT];
    int order_number;
};

struct msgbuf {
    long mtype;
    struct order_info order;
};

int main(int argc, char* argv[]) {

    srand(time(NULL));

    int msgid = msgget(KEY, 0666 | IPC_CREAT);
    struct msgbuf message;

    int shmid = shmget(KEY, SHM_SIZE, 0666 | IPC_CREAT);
    struct shared_data* data = (struct shared_data*)shmat(shmid, NULL, 0);

    int pids[COURIERS];
    int current_courier = -1;

    data->magazines[MAGAZINE_NUMBER].magazine_active = 1;
    data->magazines[MAGAZINE_NUMBER].magazine_gold = 0;
    data->ctrl_room.control_room_gold = 0;
    for (int i = 0; i < COURIERS; i++) {
        data->magazines[MAGAZINE_NUMBER].courier_active[i] = 1;
    }

    int fd = open("mag1.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    char buf[1];
    int bytes_read;
    int tmp[2 * PRODUCT] = {0};
    int i = 0;
    while ((bytes_read = read(fd, &buf, sizeof(buf))) > 0) {
        // printf("Read character: '%c' (ASCII: %d)\n", buf[0], buf[0]);
        if (isdigit(buf[0])) {
            tmp[i] = 10 * tmp[i] + (buf[0] - '0');
        } else if (buf[0] == '\n') {
            i++;
            if (i > 2 * PRODUCT) {
            printf("File contains too many lines\n");
            close(fd);
            exit(1);
            }
        } else if (buf[0] == '\r') {
            continue;
        } else {
            printf("Invalid values in file\n");
            close(fd);
            exit(1);
        }
    }
    close(fd);

    for (int i = 0; i < PRODUCT; i++) {
        data->magazines[MAGAZINE_NUMBER].product_number[i] = tmp[i];
        data->magazines[MAGAZINE_NUMBER].product_cost[i] = tmp[i + PRODUCT];
        // printf("%d\n", data->magazines[MAGAZINE_NUMBER].product_number[i]);
        // printf("%d\n", data->magazines[MAGAZINE_NUMBER].product_cost[i]);
    }

    for (int i = 0; i < COURIERS; i++) {
        pids[i] = fork();
        current_courier++;
        if (pids[i] == 0) {
            srand(getpid());
            int sleeping_time = 0;
            int min_sleep = 100000;
            int max_sleep = 500000;
            int gold = 0;
            while (data->magazines[MAGAZINE_NUMBER].courier_active[i]) {
                sleeping_time = rand() % (max_sleep - min_sleep + 1) + min_sleep;
                usleep(sleeping_time);

                if (msgrcv(msgid, &message, sizeof(struct order_info), 1, IPC_NOWAIT) > 0) {
                    // semafor
                    if (message.order.product[0] > data->magazines[MAGAZINE_NUMBER].product_number[0] ||
                        message.order.product[1] > data->magazines[MAGAZINE_NUMBER].product_number[1] ||
                        message.order.product[2] > data->magazines[MAGAZINE_NUMBER].product_number[2]) {
                            data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
                            printf("Kurier %d, magazynu %d, wylacza sie\n", i, MAGAZINE_NUMBER);
                    } else {
                        printf("Kurier %d, magazynu %d, przyjął zamówienie numer %d, na %dxA, %dxB, %dxC\n", 
                        i, MAGAZINE_NUMBER, message.order.order_number, message.order.product[0],
                        message.order.product[1], message.order.product[2]);

                        for (int j = 0; j < PRODUCT; j++) {
                            data->magazines[MAGAZINE_NUMBER].product_number[j] -= message.order.product[j];
                            gold = message.order.product[j] * data->magazines[MAGAZINE_NUMBER].product_cost[j];
                            data->magazines[MAGAZINE_NUMBER].magazine_gold += gold;
                            data->ctrl_room.control_room_gold += gold;
                        }
                    }
                }
            }
            exit(0);
        }
    }

    for (int i = 0; i < COURIERS; i++) {
        wait(NULL);
    }
    data->magazines[MAGAZINE_NUMBER].magazine_active = 0;

    printf("\n======================================================\n");
    printf("Stan magazynu %d:\n", MAGAZINE_NUMBER);
    for (int i = 0; i < PRODUCT; i++) {
        printf("Produkt %c: %d\n", (char)('A' + i), data->magazines[MAGAZINE_NUMBER].product_number[i]);
    }
    printf("Gold: %d\n", data->magazines[MAGAZINE_NUMBER].magazine_gold);
    printf("======================================================\n");

    shmdt(data);
    printf("Memory detached\n");
    shmctl(shmid, IPC_RMID, NULL);
    printf("Memory marked to delete\n");

    return 0;
}