
#include <stdio.h>
#include <pthread.h>
#include <deque>
#include <iostream>
#include <vector>
#include <errno.h>
#include <string.h>
#include <thread>
#include <stdlib.h>
#include <unistd.h>
using namespace  std;

/******************* tpool header *************/
#define MAX_ALLOWED_THREAD 200

typedef void *threadpool;

typedef void (*dispatch_fn)(void *);

threadpool create_threadpool(int num_threads_in_pool);

void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,void *arg);

void destroymytpool(threadpool destroyme);

/************  pool functions ****************/

typedef struct struct_mytpool_work {
    void (*routine) (void *);
    void *arg;
    struct struct_mytpool_work* next;
} mywork;

typedef struct struct_tpool {
   // you should fill in this structure with whatever you need
   int available_threads; // number of threads available
   pthread_mutex_t mutex; //mutex lock
   pthread_cond_t q_empty;
   pthread_cond_t q_not_empty;
   pthread_t *threads;
   int tsize;//number of task 
   mywork *work_head;//work q head - to get a work
   mywork *work_tail;//work q end - to insert new work
   bool stop;
} mytpool;

void *thread_assign_to_function(threadpool thread_pool)
{
  mytpool *my_thread_pool = (mytpool *)thread_pool;
  mywork *temp;   

  pthread_mutex_lock(&(my_thread_pool->mutex));

  while (1) {
     while(my_thread_pool->tsize == 0)
     {
         pthread_cond_wait(&(my_thread_pool->q_not_empty), &(my_thread_pool->mutex));
     }
  
     temp = my_thread_pool->work_head;
     my_thread_pool->tsize--;
     if (my_thread_pool->tsize == 0) {
       my_thread_pool->work_head = NULL;
       my_thread_pool->work_tail = NULL;
     } 
     else
     {
        my_thread_pool->work_head = temp->next;
     }

    pthread_mutex_unlock(&(my_thread_pool->mutex)); 
    (temp->routine) (temp->arg);

    free(temp);
  }
}

threadpool create_threadpool(int num_threads_in_pool) {

  mytpool *thread_pool;

// Requested pool size checking
  if(num_threads_in_pool <= 0){
    cout<<"ERROR:Number of threads in the thread pool must be a positive number"<<endl;
    return NULL;
  }

  if(num_threads_in_pool > MAX_ALLOWED_THREAD){
    cout<<"ERROR:Sorry, we are unable to create the thread pool of size:"<<num_threads_in_pool<<endl;
    cout<<"Maximum allowed threads is:"<<MAX_ALLOWED_THREAD<<endl;
    return NULL;
  }

  thread_pool = (mytpool *) malloc(sizeof(mytpool));
  if (thread_pool == NULL) {
    fprintf(stderr, "Out of memory creating a new threadpool!\n");
    return NULL;
  }
  thread_pool->available_threads = num_threads_in_pool;
  thread_pool->tsize = 0;
  thread_pool->work_head = NULL;
  thread_pool->work_tail = NULL;
  thread_pool->threads = (pthread_t *)malloc(num_threads_in_pool * sizeof(pthread_t));
  pthread_mutex_init(&(thread_pool->mutex), NULL);
  pthread_cond_init(&(thread_pool->q_empty), NULL);
  pthread_cond_init(&(thread_pool->q_not_empty), NULL);

  for (int i=0; i< thread_pool->available_threads ; i++)
  {
     if(pthread_create(&(thread_pool->threads[i]), NULL, thread_assign_to_function, thread_pool) != 0)
     {
        fprintf(stderr, "Error during threadpool creation!\n");  
        return NULL;
     }
  }

  return (threadpool) thread_pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,void *arg) {
  mytpool *thread_pool = (mytpool *) from_me;
  //printf("Inside dispatch\n");
  mywork *fn;
  
  fn = (mywork *)malloc(sizeof(mywork));
  if (fn == NULL)
  {
      fprintf(stderr, "Error allocating space for task\n");
      return;
  }

  fn->routine = dispatch_to_here;
  fn->arg = arg;
  fn->next = NULL;

  pthread_mutex_lock(&(thread_pool->mutex));
   
  if (thread_pool->tsize == 0)
  {
     thread_pool->tsize++;
     thread_pool->work_head = fn;
     thread_pool->work_tail = fn;
     pthread_cond_signal(&(thread_pool->q_not_empty));
  }
  else
  {
     thread_pool->tsize++;
     thread_pool->work_tail->next = fn;
     thread_pool->work_tail = fn;
     pthread_cond_signal(&(thread_pool->q_not_empty));
  }
  pthread_mutex_unlock(&(thread_pool->mutex));
}
void destroymytpool(threadpool destroyme) {
  mytpool *thread_pool = (mytpool *) destroyme;
  pthread_mutex_destroy(&(thread_pool->mutex));
  pthread_cond_destroy(&(thread_pool->q_empty));
  pthread_cond_destroy(&(thread_pool->q_not_empty));
  thread_pool->available_threads = 0;
  free(thread_pool->threads);
  free(thread_pool);
  return; 
}

/***************driver code**********************/

void fn_message_print(void *arg) {
  static int c=0;
  c++;
  int seconds = 2;
  //int *msg=(int*)arg;
  //int val= (int) arg;
  cout<<"\nThread called with message:  "<<(char*)arg<<endl;
  sleep(seconds);
  cout<<"\nThread end for with message: "<<(char *)arg<<endl;
}

int main(int argc, char **argv) {
  threadpool tp;

  tp = create_threadpool(3);

   //int *t=(int*)malloc(sizeof(int));
  char t[100]="1st_thread_msg_thread_function";
  int tnum=0;
  fprintf(stdout, "\n------------call from main---------- work 1\n");
  dispatch(tp, fn_message_print, (void *)t);
  //(*t)++;
  fprintf(stdout, "\n------------call from main---------- work 2\n");
  char t2[100]="2nd_thread_msg_thread_function";
  dispatch(tp, fn_message_print, (void *)t2);
  //(*t)++;
  fprintf(stdout, "\n------------call from main---------- work 3\n");
  char t3[100]="3rd_thread_msg_thread_function";
  dispatch(tp, fn_message_print, (void *)t3);
  //(*t)++;
  fprintf(stdout, "\n. . . .  3 threads called . . .. \n");
  sleep(5);
  fprintf(stdout, "\n\n");

  fprintf(stdout, "\n------------call from main---------- work 4\n");
  char t4[100]="4th_thread_msg_thread_function";
  dispatch(tp, fn_message_print, (void *)t4);
 // (*t)++;
  fprintf(stdout, "\n------------call from main---------- work 5\n");
  char t5[100]="5th_thread_msg_thread_function";
  dispatch(tp, fn_message_print, (void *)t5);
 // (*t)++;
  fprintf(stdout, "\n------------call from main---------- work 6\n");
  char t6[100]="6th_thread_msg_thread_function";
  dispatch(tp, fn_message_print, (void *)t6);
 // (*t)++;
  fprintf(stdout, "\n. . . .  3 threads called . . .. \n");
  sleep(2);
  exit(-1);
}