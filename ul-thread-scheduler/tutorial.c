#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

void *thread_1(void *arg) {
  while(1) {
    printf("hello from thread 1\n");
    usleep(1000*1000);
  }
}

void *thread_2(void *arg) {
  while(1) {
    printf("hello from thread 2\n");
    usleep(1000*1000);
  }
}

int main() {
  pthread_t thread_handle;
  pthread_t thread2_handle;

  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

  // takes the handle, attr, function we want to supply, and args to supply to function
  pthread_create(&thread_handle, NULL, thread_1, NULL); 
  pthread_create(&thread2_handle, NULL, thread_2, NULL); 

  // takes thread handle and place to put the return from the thread
  pthread_join(thread_handle, NULL); //waits for thread_1 to complete
  pthread_join(thread2_handle, NULL);
}
