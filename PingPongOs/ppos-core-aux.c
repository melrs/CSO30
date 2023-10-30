#include "ppos.h"
#include "ppos-core-globals.h"


// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções
//#define DEBUG 1;
#define QUANTUM 20;
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>

// estrutura de inicialização do timer
struct itimerval timer;
struct sigaction action;
bool running_system_task = false;

task_t * chooseTask(task_t *task) {
    if (task == NULL)
        return taskExec;

    return task;
}

int task_get_eet(task_t *task){
    return chooseTask(task)->executionTime;
}

int task_get_ret(task_t *task){
    return chooseTask(task)->remainingTime;
}

int task_getprio (task_t *task){
    return chooseTask(task)->priority;
}

void task_setprio (task_t *task, int prio){
    task = chooseTask(task);
    task->priority = prio;
}

void task_set_dinamic_prio(task_t *task){
    if (running_system_task)
        return;
    
    task->priority = task->remainingTime;
}

void task_set_eet (task_t *task, int et){
    task = chooseTask(task);
    task->executionTime = et;
    task->remainingTime = et;

    task_set_dinamic_prio(task);
}

void print_tcb_details(task_t* task){
    task = chooseTask(task);
    printf("TID: %d, ", task->id);
    printf("\n\tState: %c", task->state);
    if (task->next != NULL) {
        printf("\n\tNext: %d", (task->next)->id);
    } else {
        printf("\n\tNext: (null)");
    }

    if (task->prev != NULL) {
        printf("\n\tPrev: %d", (task->prev)->id);
    } else {
        printf("\n\tPrev: (null)");
    }
    printf("\n\tExitCode: %d", task->exitCode);
    printf("\n\tAwakeTime: %d", task->awakeTime);
    printf("\n\tPriority: %d", task->priority);
    printf("\n\tIsSystemTask: %d", task->isSystemTask);
    printf("\n\tExecutionTime: %d", task->executionTime);
    printf("\n\tRemainingTime: %d", task->remainingTime);
    printf("\n\tRunningTime: %d", task->runningTime);
    printf("\n\tActivations: %d", task->activations);
    printf("\n\tQuantum: %d\n", task->quantum);
}

void print_tcb(task_t* task){
    printf("(%d,%d, Rem time: %d), ", task->id, task->priority, task->remainingTime);
}


void handler (int signum) {
  systemTime++;
  if(taskExec->isSystemTask || running_system_task)
    return;

  if(taskExec->quantum <= 0){
    task_yield();
  }

  taskExec->quantum--;
  taskExec->runningTime++;
}

void temporizer () {
  action.sa_handler = handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  if (sigaction (SIGALRM, &action, NULL) < 0) {
    perror ("Erro em sigaction: ");
    exit (1) ;
  }

  timer.it_value.tv_usec = 0;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 1;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

  if (setitimer (ITIMER_REAL, &timer, NULL) < 0) {
    perror ("Erro em setitimer: ");
    exit (1);
  }

}

// ****************************************************************************



void before_ppos_init () {
    running_system_task = true;
    systemTime = 0;
    temporizer();
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_ppos_init () {
    running_system_task = false;
    taskExec->start = systime();
    taskExec->isSystemTask = true;
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    running_system_task = true;
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create (task_t *task ) {
    task->start = systime();
    task->activations = 0;
    task_set_eet(task, 99999);
    running_system_task = false;
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void before_task_exit () {
    running_system_task = true;
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    running_system_task = false;
    taskExec->totalExecutionTime = systime() - taskExec->start;
    printf("\nTask %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, taskExec->totalExecutionTime, taskExec->runningTime, taskExec->activations);
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    running_system_task = true;
    taskExec->remainingTime = taskExec->executionTime - taskExec->runningTime;
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    running_system_task = false;
    //taskExec->quantum = QUANTUM;
    taskExec->awakeTime = systime();
    taskExec->activations++;
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]\n", taskExec->id, task->id);

#endif
}

void before_task_yield () {
    running_system_task = true;
    task_set_dinamic_prio(taskExec);
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}
void after_task_yield () {
    running_system_task = false;
    task_set_dinamic_prio(taskExec);
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


void before_task_suspend( task_t *task ) {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    running_system_task = true;   
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
    printf("\ntask_join - BEFORE EXECUTANDO STATE:- [%c]", taskExec->state);
    printf("\ntask_join - BEFORE PARAM:- [%d]", task->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER EXECUTANDO:- [%d]", taskExec->id);
    printf("\ntask_join - AFTER PARAM:- [%d]", task->id);
    printf("\ntask_join - AFTER EXECUTANDO STATE:- [%c]", taskExec->state);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

// Uma função scheduler que analisa a fila de tarefas prontas, devolvendo um ponteiro para a próxima tarefa a receber o processador, que será a com menor prioridade.
task_t * scheduler() {
    running_system_task = true;
    #ifdef DEBUG
        printf(msg,"\nCHAMANDO O SCHEDULER fila: %d exc: %d\n", readyQueue->id, taskExec->id);
        RINT_READY_QUEUE
    #endif
    if(readyQueue == NULL || readyQueue->next == readyQueue)
        return readyQueue;
    
    task_t* next = readyQueue;
    task_t* lowestPriority = readyQueue;
    #ifdef DEBUG
        printf("next [%d,%d]\n",next->id, next->executionTime);
        printf("LOWEST 1 [%d, %d]\n", lowestPriority->id, lowestPriority->executionTime);
    #endif

    while (next != readyQueue->prev) {
        next = next->next;
        if (!next->isSystemTask && (next->priority < lowestPriority->priority || lowestPriority->isSystemTask)) {
            lowestPriority = next;
        }
        #ifdef DEBUG
            printf("next [%d,%d]\n",next->id, next->executionTime);
            printf("LOWEST 1 [%d, %d]\n", lowestPriority->id, lowestPriority->executionTime);
        #endif
    }
    running_system_task = false;
    lowestPriority->quantum = QUANTUM;
    #ifdef DEBUG
        printf("SCHEDULER RETORNOU [%d]\n", lowestPriority->id);
    #endif
    
    return lowestPriority;
}


