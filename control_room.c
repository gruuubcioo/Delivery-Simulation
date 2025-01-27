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

// to jako argumenty
#define KEY 5555
#define LICZBA_ZAMOWIEN 20
#define MAX_A 5
#define MAX_B 10
#define MAX_C 15

struct order_info {
    int a_product;
    int b_product;
    int c_product;
    int order_nummber;
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

    int delay;
    int min = 25000; // 0.025s
    int max = 500000; // 0.5 s

    for (int i = 0; i < LICZBA_ZAMOWIEN; i++) {

        int delay = rand() % (max - min + 1) + min;
        usleep(delay);

        a = (rand() % (MAX_A - 1)) + 1;
        b = (rand() % (MAX_B - 1)) + 1;
        c = (rand() % (MAX_C - 1)) + 1;

        message.order.a_product = a;
        message.order.b_product = b;
        message.order.c_product = c;
        message.order.order_number = i;

        msgsnd(msgid, &message, sizeof(struct order_info), 0);
        printf("Zlecono zlecenie %d na %dxA, %dxB, %dxC", i, a, b, c);
    }


    return 0;
}