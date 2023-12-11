#include "ppos_disk.h"
#include "ppos.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>

//#define  DEBUGPD 1
#define DISK_ERROR -1
#define DISK_SUCCESS 0
#define DISK_FCFS 0
#define DISK_SSTF 1
#define DISK_CSCAN 2

static struct sigaction disk_act;
bool disk_called = 0;
disk_t* disk_v;
struct task_t taskDiskManager;
disk_request* diskRequestQueue;
struct task_t* diskManagerQueue;


void disk_handler (int signum) {
#ifdef DEBUGPD
    printf ("Recebi o sinal %d\n", signum) ;
#endif
    disk_called = 1;
    task_resume(&taskDiskManager);
}

void signal_config () {
#ifdef DEBUGPD
    printf ("signalConfig: inicio\n");
#endif
    disk_act.sa_handler = disk_handler;
    sigemptyset (&disk_act.sa_mask);
    disk_act.sa_flags = 0;
    if (sigaction (SIGUSR1, &disk_act, NULL) < 0) {
        perror ("Erro em sigaction: ");
        exit (1) ;
    }
}

bool status(int expected_state) {
    return disk_cmd (DISK_CMD_STATUS, 0, 0) == expected_state;
}

void disk_enqueue (disk_request* request) {
#ifdef DEBUGPD
    printf ("disk_enqueue: inicio\n");
#endif
    if (diskRequestQueue == NULL) {
        request->next = request;
        request->prev = request;
        diskRequestQueue = request;
    } else {
        request->next = diskRequestQueue;
        request->prev = diskRequestQueue->prev;
        diskRequestQueue->prev->next = request;
        diskRequestQueue->prev = request;
    }
}

disk_request* disk_dequeue () {
#ifdef DEBUGPD
    printf ("disk_dequeue: inicio\n");
#endif
    if (diskRequestQueue == NULL) {
        return NULL;
    }
    disk_request* request = diskRequestQueue;
    if (diskRequestQueue->next == diskRequestQueue) {
        diskRequestQueue = NULL;
    } else {
        diskRequestQueue->next->prev = diskRequestQueue->prev;
        diskRequestQueue->prev->next = diskRequestQueue->next;
        diskRequestQueue = diskRequestQueue->next;
    }
    return request;
}

disk_request* disk_fcfs () {
#ifdef DEBUGPD
    printf ("disk_fcfs: inicio\n");
#endif

    if (diskRequestQueue == NULL) {
        return NULL;
    }
    return disk_dequeue();
}

disk_request* disk_sstf () {
#ifdef DEBUGPD
    printf ("disk_sstf: inicio\n");
#endif
    if (diskRequestQueue == NULL || diskRequestQueue->next == diskRequestQueue) {
        printf("disk_sstf: diskRequestQueue == NULL\n");
        return disk_dequeue();
    }
    disk_request* request = diskRequestQueue;
    disk_request* aux = diskRequestQueue->next;
    int distance = abs(diskRequestQueue->block - disk_v->lastBlock);
    while (aux != diskRequestQueue) {
        int aux_distance = abs(aux->block - disk_v->lastBlock);
        if (aux_distance < distance) {
            distance = aux_distance;
            request = aux;
        }
        aux = aux->next;
    }
    if (request->next == request) {
        diskRequestQueue = NULL;
    } else {
        request->next->prev = request->prev;
        request->prev->next = request->next;
        diskRequestQueue = request->next;
    }

    return request;
}

disk_request* disk_cscan () {
#ifdef DEBUGPD
    printf ("disk_cscan: inicio\n");
#endif

    if (diskRequestQueue == NULL) {
        return NULL;
    }
    disk_request* request = diskRequestQueue;
    disk_request* aux = diskRequestQueue;
    int distance = abs(diskRequestQueue->block - disk_v->lastBlock);
    while (aux->next != diskRequestQueue) {
        aux = aux->next;
        int aux_distance = abs(aux->block - disk_v->lastBlock);
        if (aux_distance > distance) {
            distance = aux_distance;
            request = aux;
        }
    }
    if (request->next == request) {
        diskRequestQueue = NULL;
    } else {
        request->next->prev = request->prev;
        request->prev->next = request->next;
        diskRequestQueue = request->next;
    }

    return request;
}

disk_request* disk_choose_scheduler (int scheduler) {
#ifdef DEBUGPD
    printf ("disk_choose_scheduler: inicio\n");
#endif
    switch (scheduler) {
        case DISK_FCFS: //execution time 29997 ms, 7681 blocks covered
            return disk_fcfs();
        case DISK_SSTF:
            return disk_sstf(); //execution time 26888 ms, 4671 blocks covered
        case DISK_CSCAN:
            return disk_cscan(); //execution time 90698 ms, 65265 blocks covered

        default:
            return NULL;
    }
}

void create_disk_request(int block, void *buffer, int cmd) {
#ifdef DEBUGPD
    printf ("create_disk_request: inicio\n");
#endif
    disk_request* request = malloc(sizeof(disk_request));
    request->block = block;
    request->buffer = buffer;
    request->cmd = cmd;
    request->task = taskExec;
    disk_enqueue(request);
}

void disk_manager_body (void * args) {
#ifdef DEBUGPD
    printf ("disk_manager_body: inicio\n");
#endif
    while (true) {
        sem_up(&(disk_v->semaphore));
        if (disk_called > 0) {
            task_resume(diskManagerQueue);
            disk_called = 0;
        }     

        if (status(DISK_STATUS_IDLE) && (diskRequestQueue != NULL)) {
            disk_request* request = disk_choose_scheduler(1);
            if(disk_cmd (request->cmd, request->block, request->buffer) < DISK_SUCCESS) {
                printf("Erro ao executar o comando %d\n", request->cmd);
                exit(1);
            }
            int distance = abs(request->block - disk_v->lastBlock);
            disk_v->blockCount += distance;
            disk_v->lastBlock = request->block;
        }
        
        task_suspend(taskExec, 0);
        task_yield();
        sem_down(&(disk_v->semaphore));
    }
}

int disk_mgr_init1 (int *numBlocks, int *blockSize) {
#ifdef DEBUGPD
    printf ("disk_mgr_init: inicio\n");
#endif
    signal_config();
    if( disk_cmd (DISK_CMD_INIT, 0, 0) < DISK_SUCCESS || status(DISK_STATUS_UNKNOWN))
        return DISK_ERROR;
    disk_v = malloc(sizeof(disk_t));
    disk_v->blockSize = disk_cmd (DISK_CMD_BLOCKSIZE, 0, 0);
    disk_v->numBlocks = disk_cmd (DISK_CMD_DISKSIZE, 0, 0);
    disk_v->lastBlock = 0;
    *blockSize = disk_v->blockSize;
    *numBlocks = disk_v->numBlocks;
    sem_create(&(disk_v->semaphore), 0);
    task_create(&taskDiskManager, disk_manager_body, NULL);
    taskDiskManager.isSystemTask = 1;
    taskDiskManager.state = PPOS_TASK_STATE_SUSPENDED;

    return DISK_SUCCESS;
}

int disk_block_read1 (int block, void *buffer) {
#ifdef DEBUGPD
    printf ("disk_block_read: inicio\n");
#endif
    if(status(DISK_STATUS_UNKNOWN))
        return DISK_ERROR;
    sem_up(&(disk_v->semaphore));
    create_disk_request(block, buffer, DISK_CMD_READ);
 
    if (toupper(taskDiskManager.state) == PPOS_TASK_STATE_SUSPENDED) {
        task_resume(&taskDiskManager);
    }
    sem_down(&(disk_v->semaphore));
    task_suspend(taskExec, &diskManagerQueue);
    task_yield();
    return DISK_SUCCESS;
}

int disk_block_write1 (int block, void *buffer) {
#ifdef DEBUGPD
    printf ("disk_block_write: inicio\n");
#endif
    if(status(DISK_STATUS_UNKNOWN))
        return DISK_ERROR;
    sem_up(&(disk_v->semaphore));
    
    create_disk_request(block, buffer, DISK_CMD_WRITE);
 
    if (toupper(taskDiskManager.state) == PPOS_TASK_STATE_SUSPENDED) {
        task_resume(&taskDiskManager);
        task_suspend(taskExec, &diskManagerQueue);
        task_yield();
    }
    sem_down(&(disk_v->semaphore));
    return DISK_SUCCESS;
}

int disk_blocks() {
    return disk_v->blockCount;
}