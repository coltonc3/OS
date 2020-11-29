#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "queue/queue.h"
#include <pthread.h>
#include <unistd.h>
#include <ucontext.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

/* NOTE: something throwing seg faults at the end of my sut_open() call in i-exec. I've tried it for hours
and have found absolutely no luck. print statements work at the end of open(), but not at the beginning of write() */

typedef void (*sut_task_f)();

int num_threads, max_threads;
struct queue tr_queue; /* task ready queue */
// struct queue wait_queue; /* wait queue */
pthread_t cexec; /* computing task executor */
pthread_t iexec; /* I/O task executor */
ucontext_t c, i;
struct queue_entry *cur_node;
int sockfd;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* mutex lock for c-exec critical sections */
pthread_mutex_t io_lock = PTHREAD_MUTEX_INITIALIZER; /* mutex lock for I/O */
struct queue io_queue;
char *b;
/* data to be stored in I/O queue node */
struct io_data {
  char *host;
  int port;
  int flag;
  char *buffy;
};

/* runner function for C-EXEC */
void *c_executor() {
  while (1){
    /* lock mutex while doing operations on shared queue to avoid race condition */
    pthread_mutex_lock(&lock);

    /* if task-ready queue is empty, unlock mutex and sleep to allow new context to be available */
    if(!queue_peek_front(&tr_queue)) {
      pthread_mutex_unlock(&lock);
      usleep(100);
    }
    else{
      /* pop the head of the task-ready queue to run next */
      cur_node = queue_pop_head(&tr_queue);
      pthread_mutex_unlock(&lock);

      /* switch to context of next task */
      swapcontext(&c, cur_node->data);
      
      /* break out of loop when it's time to shutdown and no more tasks left to run */
      if (!num_threads)
        break;
      }
  }
  return 0;
}

void *io_executor() {
  while(1) {
    pthread_mutex_lock(&io_lock);
    /* check I/O queue */
    if(queue_peek_front(&io_queue)) {
      struct queue_entry *io_node = queue_pop_head(&io_queue);
      pthread_mutex_unlock(&io_lock);
      struct io_data data = *(struct io_data *) io_node->data;
      printf("flag: %d\n", data.flag);
      if (data.flag == 0) { /* open connection */
        struct sockaddr_in server_address = { 0 };
        // create a new socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
          perror("Failed to create a new socket\n");
        }
        // connect to server
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, data.host, &(server_address.sin_addr.s_addr));
        server_address.sin_port = htons(data.port);
        if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
          perror("Failed to connect to server\n");
        }

        pthread_mutex_lock(&lock);
        queue_insert_tail(&tr_queue, cur_node);
        pthread_mutex_unlock(&lock);
      }
      else if(data.flag == 1) { /* read */
        recv(sockfd, b, sizeof(b),0);
        pthread_mutex_lock(&lock);
        queue_insert_tail(&tr_queue, cur_node);
        pthread_mutex_unlock(&lock);
      }
      else if(data.flag == 2) { /* write */
        send(sockfd, data.buffy, strlen(data.buffy),0);
        pthread_mutex_lock(&lock);
        queue_insert_tail(&tr_queue, cur_node);
        pthread_mutex_unlock(&lock);
      }
    }else {
      pthread_mutex_unlock(&io_lock);
      usleep(100);
    }
    if (!num_threads)
    {
      break;
    }
  }
  return 0;
}

/* create two kernel-level threads referred to as "executors"
 * C-EXEC is the executor that handles all compute tasks
 * I-EXEC is the exucutor that handles all I/O tasks
*/
void sut_init() {
  num_threads = 0;
  max_threads = 15;
  tr_queue = queue_create();
  io_queue = queue_create();
  queue_init(&tr_queue);
  queue_init(&io_queue);

  // fire up pthread executors
  pthread_create(&cexec, NULL, c_executor, NULL);
  pthread_create(&iexec, NULL, io_executor, NULL);
}

void sut_shutdown(){
  pthread_join(cexec, NULL);
  pthread_join(iexec, NULL);
}

/* creates a new task to be placed at the back of the task ready queue */
bool sut_create(sut_task_f fn){
  if(num_threads < max_threads) {
    ucontext_t *new_task = (ucontext_t *) malloc(sizeof(ucontext_t));

    char stack[16 * 1024];

    /* copy current context into new task */
    getcontext(new_task);

    /* allocate a new stack for this task's context */
    new_task->uc_stack.ss_sp = stack;
    new_task->uc_stack.ss_size = sizeof(stack);
    new_task->uc_link = &c;

    makecontext(new_task, fn, 0);

    pthread_mutex_lock(&lock); 
    /* toss this user level thread into back of the task-ready queue */
    queue_insert_tail(&tr_queue,  queue_new_node(new_task));
    pthread_mutex_unlock(&lock);

    /* increment thread counter */
    num_threads++;

    // printf("num threads: %d\n", num_threads);

    return true;
  }
  else{
    printf("FATAL: maximum number of threads (%d) reached\n", max_threads);
    return false;
  }
}

/* stops a currently running task's execution, saves context to TCB and places it in the back of the task-ready queue */
void sut_yield() {
  pthread_mutex_lock(&lock);
  
  queue_insert_tail(&tr_queue, cur_node);
  
  pthread_mutex_unlock(&lock);
  
  swapcontext(cur_node->data, &c);
}

/* terminates a task's execution without putting it back in the task-ready queue or updating TCB */
void sut_exit() {
  free(cur_node->data);
  num_threads--;
  // printf("num threads: %d\n", num_threads);
  swapcontext(cur_node->data, &c);
} 

/* sends msg to I-EXEC by enqueing msg in the to-IO queue, said msg will tell I-EXEC to open connection to specified destination at port
 * then we can use sut_read() or sut_write() on this destination
 */
void sut_open(char *dest, int port) {
  struct io_data data;
  data.host = dest;
  data.port = port;
  data.flag = 0;
  pthread_mutex_lock(&io_lock);
  queue_insert_tail(&io_queue, queue_new_node(&data));
  pthread_mutex_unlock(&io_lock);
  swapcontext(cur_node->data, &c);
}

void sut_write(char *buf, int size) {
  struct io_data data;
  data.flag = 2;
  data.buffy = buf;
  data.port = size;
  pthread_mutex_lock(&io_lock);
  queue_insert_tail(&io_queue, queue_new_node(&data));
  pthread_mutex_unlock(&io_lock);
  
}

/* terminates the connection created by sut_open() */
void sut_close() {

}

/* saves context to TCB and puts task at back of wait queue after sending request to the request queue */
char *sut_read() {
  struct io_data data;
  data.flag = 1;
  pthread_mutex_lock(&io_lock);
  queue_insert_tail(&io_queue, queue_new_node(&data));
  pthread_mutex_unlock(&io_lock);
  swapcontext(cur_node->data, &c);
  return b;
}
