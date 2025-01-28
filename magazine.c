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
#include <time.h>
#include <sys/sem.h>

#define COURIERS 3
#define PRODUCT 3
#define MAGAZINES 3
#define SHM_SIZE 1024
#define WAITING_TIME 15
#define SEM_KEY 6666
#define MEMORY_FREE 0

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
    int sem_ready;
    int mag_count;
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

    if (argc != 3) {
        printf("Niepoprawna liczba argumentow\n");
        exit(1);
    }
    int key = atoi(argv[2]);
    if (key <= 0) {
        printf("Podaj liczbe dodatnią całkowitą\n");
        exit(1);
    }

    srand(time(NULL));

    int msgid = msgget(key, 0666 | IPC_CREAT);
    struct msgbuf message;

    int shmid = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    struct shared_data* data = (struct shared_data*)shmat(shmid, NULL, 0);

    int semid = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    struct sembuf memory_lock = {MEMORY_FREE, -1, 0};
    struct sembuf memory_unlock = {MEMORY_FREE, 1, 0};

    while (!data->sem_ready) {
        usleep(100000);
    }

    semop(semid, &memory_lock, 1);
    const int MAGAZINE_NUMBER = data->mag_count;
    data->mag_count++;
    semop(semid, &memory_unlock, 1);

    int pids[COURIERS];
    int current_courier = -1;

    semop(semid, &memory_lock, 1);

    data->magazines[MAGAZINE_NUMBER].magazine_active = 1;
    data->magazines[MAGAZINE_NUMBER].magazine_gold = 0;
    data->ctrl_room.control_room_gold = 0;
    for (int i = 0; i < COURIERS; i++) {
        data->magazines[MAGAZINE_NUMBER].courier_active[i] = 1;
    }

    semop(semid, &memory_unlock, 1);

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        for (int i = 0; i < COURIERS; i++) {
            semop(semid, &memory_lock, 1);
            data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
            semop(semid, &memory_unlock, 1);
        }
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
            for (int i = 0; i < COURIERS; i++) {
                semop(semid, &memory_lock, 1);
                data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
                semop(semid, &memory_unlock, 1);
            }
        }
    }
    close(fd);

    for (int i = 0; i < (2 * PRODUCT); i++) {
        if (tmp[i] == 0) {
            for (int i = 0; i < COURIERS; i++) {
                semop(semid, &memory_lock, 1);
                data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
                semop(semid, &memory_unlock, 1);
            }
            break;
        }
    }

    semop(semid, &memory_lock, 1);

    for (int i = 0; i < PRODUCT; i++) {
        data->magazines[MAGAZINE_NUMBER].product_number[i] = tmp[i];
        data->magazines[MAGAZINE_NUMBER].product_cost[i] = tmp[i + PRODUCT];
        // printf("%d\n", data->magazines[MAGAZINE_NUMBER].product_number[i]);
        // printf("%d\n", data->magazines[MAGAZINE_NUMBER].product_cost[i]);
    }

    semop(semid, &memory_unlock, 1);

    for (int i = 0; i < COURIERS; i++) {
        pids[i] = fork();
        current_courier++;
        if (pids[i] == 0) {
            srand(getpid());
            int sleeping_time = 0;
            int min_sleep = 100000;
            int max_sleep = 500000;
            int gold = 0;
            int last_order_time = time(NULL);
            int current_time;
            while (data->magazines[MAGAZINE_NUMBER].courier_active[i]) {
                sleeping_time = rand() % (max_sleep - min_sleep + 1) + min_sleep;
                usleep(sleeping_time);

                current_time = time(NULL);
                if (difftime(current_time, last_order_time) > WAITING_TIME) {
                    data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
                    printf("Kurier %d magazynu %d wyłącza się po %d sekundach bez zlecenia\n", i, MAGAZINE_NUMBER, WAITING_TIME);
                    exit(0);
                }


                if (msgrcv(msgid, &message, sizeof(struct order_info), 1, IPC_NOWAIT) > 0) {
                    last_order_time = time(NULL);
                    if (message.order.product[0] > data->magazines[MAGAZINE_NUMBER].product_number[0] ||
                        message.order.product[1] > data->magazines[MAGAZINE_NUMBER].product_number[1] ||
                        message.order.product[2] > data->magazines[MAGAZINE_NUMBER].product_number[2]) {
                            data->magazines[MAGAZINE_NUMBER].courier_active[i] = 0;
                            printf("Kurier %d, magazynu %d, wylacza sie z powodu braku zasobow(zamowienie %d)\n", i, MAGAZINE_NUMBER, message.order.order_number);
                    } else {
                        printf("Kurier %d, magazynu %d, przyjął zamówienie numer %d, na %dxA, %dxB, %dxC\n", 
                        i, MAGAZINE_NUMBER, message.order.order_number, message.order.product[0],
                        message.order.product[1], message.order.product[2]);

                        semop(semid, &memory_lock, 1);

                        for (int j = 0; j < PRODUCT; j++) {
                            data->magazines[MAGAZINE_NUMBER].product_number[j] -= message.order.product[j];
                            gold = message.order.product[j] * data->magazines[MAGAZINE_NUMBER].product_cost[j];
                            data->magazines[MAGAZINE_NUMBER].magazine_gold += gold;
                            data->ctrl_room.control_room_gold += gold;
                        }
                        semop(semid, &memory_unlock, 1);
                    }
                }
            }
            exit(0);
        }
    }

    for (int i = 0; i < COURIERS; i++) {
        wait(NULL);
    }

    semop(semid, &memory_lock, 1);

    data->magazines[MAGAZINE_NUMBER].magazine_active = 0;

    printf("\n======================================================\n");
    printf("Stan magazynu %d:\n", MAGAZINE_NUMBER);
    for (int i = 0; i < PRODUCT; i++) {
        printf("Produkt %c: %d\n", (char)('A' + i), data->magazines[MAGAZINE_NUMBER].product_number[i]);
    }
    printf("Gold: %d\n", data->magazines[MAGAZINE_NUMBER].magazine_gold);
    printf("======================================================\n\n");

    semop(semid, &memory_unlock, 1);

    shmdt(data);
    printf("Memory detached\n");

    return 0;
}