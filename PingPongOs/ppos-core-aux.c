#include "ppos.h"
#include "ppos-core-globals.h"


// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções
//#define DEBUG 1;
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

void task_set_eet (task_t *task, int et){
    task = chooseTask(task);
    task->executionTime = et;
    task->remainingTime = et;
    task_t *next = task->next;
    int aux;

    do {
        if(next->remainingTime == task->remainingTime ||
          (next->remainingTime < task->remainingTime && task->priority < next->priority) ||
          (next->remainingTime > task->remainingTime && task->priority > next->priority) 
        ){
            next = next->next;
            continue;
        }
        
        aux = task->priority;
        task->priority = next->priority;
        next->priority = aux;
        next = next->next;

    } while (next != task->prev);

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
    printf("\n\tProcessorTime: %d", task->processorTime);
    printf("\n\tRemainingTime: %d\n", task->remainingTime);
}

void print_tcb(task_t* task){
    printf("(%d,%d), ", task->id, task->priority);
}


void handler (int signum) {
  systemTime++;
  if(!running_system_task)
    taskExec->quantum--;
  
  if(taskExec->quantum == 0)
    task_yield();

}

void temporizer () {
  action.sa_handler = handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;
  if (sigaction (SIGALRM, &action, NULL) < 0) {
    perror ("Erro em sigaction: ");
    exit (1) ;
  }

  timer.it_value.tv_usec = 25000;      // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
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
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    running_system_task = true;
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
    printf("cria uma nova tarefa, que recebe como parâmetros uma funcao e um argumento\n");
#endif
}

void after_task_create (task_t *task ) {
    running_system_task = false;
    task_set_eet(task, 99999);
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void before_task_exit () {
    running_system_task = true;
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
    printf("termina a tarefa corrente, indicando um valor de status encerramento\n");
#endif
}

void after_task_exit () {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
    printf("troca de contexto: de %d para %d\n", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);

#endif
}

void before_task_yield () {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
    printf("libera o processador para a próxima tarefa, retornando à fila de tarefas prontas\n");
#endif
}
void after_task_yield () {
    running_system_task = false;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


void before_task_suspend( task_t *task ) {
    running_system_task = true;
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
    printf("suspende uma tarefa, retirando-a de sua fila de prontas\n");
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
    printf("resume a execução de uma tarefa, colocando-a na fila de prontas\n");
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
    printf("suspende a tarefa corrente por um tempo determinado\n");
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
    printf("a tarefa corrente aguarda o encerramento de outra task");
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
    printf("cria um semáforo com valor inicial \"value\"");
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
    printf("requisita o semáforo");
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
    printf("libera o semáforo");
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
    printf("destroi o semáforo");
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
    printf("cria um mutex");
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
    printf("requisita o mutex");
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
    printf("libera o mutex");
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
    printf("destroi o mutex");
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
    printf("cria uma barreira para N tarefas");
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
    printf("espera todas as N tarefas alcançarem a barreira");
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
    printf("destroi uma barreira");
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
    printf("cria uma fila de mensagens com no máximo max mensagens de tamanho size bytes cada");
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
    printf("envia uma mensagem para a fila de mensagens");
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
    printf("recebe uma mensagem da fila de mensagens");
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
    printf("destroi a fila de mensagens");
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
    printf("retorna o número de mensagens atualmente na fila de mensagens");
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
    //printf("\nCHAMANDO O SCHEDULER\n");
    //PRINT_READY_QUEUE
    if(readyQueue == NULL)
        return NULL;

    task_t* next = readyQueue;
    task_t* lowestPriority = readyQueue;

    while (next != readyQueue->prev) {
        next = next->next;
        if (next->priority < lowestPriority->priority) {
            lowestPriority = next;
        }
    }
    running_system_task = false;
    lowestPriority->quantum = 20;
    //printf("\nSCHEDULER RETORNOU [%d]\n", lowestPriority->id);
    return lowestPriority;
}


