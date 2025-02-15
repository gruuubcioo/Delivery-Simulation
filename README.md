# **Dispatch System with Warehouses and Couriers**

### This project simulates a system for managing warehouses, couriers, and orders. The main components of the system are:

- **Interprocess Communication** – Uses shared memory, message queues, and semaphores.
- **Control room** – Creates and initializes shared memory (its part), a message queue, and a semaphore. Sends orders through the message queue. When warehouses have finished their work, it displays the amount of gold spent.
- **Warehouses** – Wait for the control room. Initialize shared memory (their part) with default values and data from configuration files. Create their own couriers. When all couriers shut down, the warehouse shuts down and displays its statistics.
- **Couriers** – Pick up orders from the message queue sent by the control room. If a courier can complete the order, it updates the shared memory. If a courier cannot pick up the order or has not received any for a specified time (15 seconds by default), it shuts down.

## **Compilation and Execution**

To compile all modules:

```sh
    gcc control_room.c -o control_room
    gcc warehouse.c -o warehouse
```

To execute (3 warehouses on 3 terminals by deafult):

```sh
    ./control_room <ipc_key> <order_count> <max_A> <max_B> <max_C>
    ./warehouse <conf_file> <ipc_key>
```

Where:

-  `<ipc_key>` – IPC key for communication
-  `<order_count>` – Number of generated orders
-  `<max_A>` – Maximum quantity of product A in a single order
-  `<max_B>` – Maximum quantity of product B in a single order
-  `<max_C>` – Maximum quantity of product C in a single order
-  `<conf_file>` – Configuration file storing information about the initial warehouse status