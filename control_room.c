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
#include <sys/sem.h>

#define MAGAZINES 3
#define COURIERS 3
#define PRODUCT 3
#define SHM_SIZE 1024
#define SEM_KEY 6666
#define MEMORY_FREE 0

// to jako argumenty
#define KEY 5555
#define LICZBA_ZAMOWIEN 20
#define MAX_A 5
#define MAX_B 10
#define MAX_C 15

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
    int a = 0;
    int b = 0;
    int c = 0;
    struct msgbuf message;
    message.mtype = 1;

    int shmid = shmget(KEY, SHM_SIZE, 0666 | IPC_CREAT);
    struct shared_data* data = (struct shared_data*)shmat(shmid, NULL, 0);
    int semid = semget(SEM_KEY, 1, 0666 | IPC_CREAT);
    semctl(semid, MEMORY_FREE, SETVAL, 1);
    data->sem_ready = 1;
    
    struct sembuf memory_lock = {MEMORY_FREE, -1, 0};
    struct sembuf memory_unlock = {MEMORY_FREE, 1, 0};

    semop(semid, &memory_lock, 1);

    data->ctrl_room.control_room_active = 1;

    semop(semid, &memory_unlock, 1);

    int sleeping_time;
    int min_sleep = 25000; // 0.025s
    int max_sleep = 500000; // 0.5 s

    for (int i = 0; i < LICZBA_ZAMOWIEN; i++) {

        int sleeping_time = rand() % (max_sleep - min_sleep + 1) + min_sleep;
        usleep(sleeping_time);

        a = (rand() % (MAX_A + 1));
        b = (rand() % (MAX_B + 1));
        c = (rand() % (MAX_C + 1));

        message.order.product[0] = a;
        message.order.product[1] = b;
        message.order.product[2] = c;
        message.order.order_number = i;

        msgsnd(msgid, &message, sizeof(struct order_info), 0);
        printf("Zlecono zlecenie numer %d, na %dxA, %dxB, %dxC\n", i, a, b, c);
    }

    int mag_active = 0;
    while (data->ctrl_room.control_room_active) {
        mag_active = 0;
        sleep(2);
        for (int i = 0; i < MAGAZINES; i ++) {
            mag_active += data->magazines[i].magazine_active;
        }
        if (mag_active == 0) {
            semop(semid, &memory_lock, 1);
            data->ctrl_room.control_room_active = 0;
            semop(semid, &memory_unlock, 1);
        }
    }

    printf("\n======================================================\n");
    printf("Stan dyspozytorni:\n");
    printf("Gold: %d\n", data->ctrl_room.control_room_gold);
    printf("======================================================\n");

    msgctl(msgid, IPC_RMID, NULL);
    printf("Kolejka usunieta\n");
    shmdt(data);
    printf("Memory detached\n");
    shmctl(shmid, IPC_RMID, NULL);
    printf("Memory marked to delete\n");
    semctl(semid, 0, IPC_RMID);
    printf("Semafor usunięty\n");

    return 0;
}