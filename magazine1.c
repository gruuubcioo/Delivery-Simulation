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

#define COURIERS 3
#define PRODUCT 3
#define MAGAZINES 3
#define SHM_SIZE 1024

#define MAGAZINE_NUMBER 0

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

int main(int argc, char* argv[]) {

    srand(time(NULL));

    int msgid = msgget(KEY, 0666 | IPC_CREAT);

    int shmid = shmget(KEY, SHM_SIZE, 0666 | IPC_CREAT);
    struct shared_data* data = (struct shared_data*)shmat(shmid, NULL, 0);


    int pids[COURIERS];
    int current_courier = -1;

    data->magazines[MAGAZINE_NUMBER].magazine_active = 1;
    data->magazines[MAGAZINE_NUMBER].magazine_gold = 0;
    for (int i = 0; i < COURIERS; i++) {
        data->magazines[MAGAZINE_NUMBER].courier_active[i] = 1;
    }

    int fd = open("plik.txt", O_RDONLY);


    char buf[1];
    int bytes_read;
    int pos;
    int i = 0;
    while ((bytes_read = read(fd, &buf, sizeof(buf))) > 0) {
        i++
        if (buf[0] == '\n') {

        }
    }


    for (int i = 0; i < COURIERS; i++) {
        pids[i] = fork();
        current_courier++;
        if (pid[i] == 0) {
            srand(getpid());
            while (1) {
                
            }
        }
    }



    return 0;
}