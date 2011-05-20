#include <pthread.h>
#include <stdio.h>

#define LOCK
#undef LOCK

int a;
pthread_mutex_t mutex;

void *f1(void *ptr) {
  long i;
#ifdef LOCK
  pthread_mutex_lock(&mutex);
#endif
  for (i = 0; i < 10; i++) {
    a++;
  }
#ifdef LOCK
  pthread_mutex_unlock(&mutex);
#endif
  printf("t1\n");
}

void *f2(void *ptr) {
  long i;
#ifdef LOCK
  pthread_mutex_lock(&mutex);
#endif
  for (i = 0; i < 10; i++) {
    a--;
  }
#ifdef LOCK
  pthread_mutex_unlock(&mutex);
#endif
  printf("t2\n");
}

int main(int argc, char **argv) {
  pthread_t tid1, tid2;
  void *sptr1, *sptr2;
  a = 0;
#ifdef LOCK
  printf("mutex init\n");
  pthread_mutex_init(&mutex, NULL);
#endif
  printf("thread_create\n");
  pthread_create(&tid1, NULL, &f1, NULL);
  pthread_create(&tid2, NULL, &f2, NULL);
  printf("thread_join\n");
  pthread_join(tid1, &sptr1);
  pthread_join(tid2, &sptr2);
#ifdef LOCK
  printf("mutex destroy\n");
  pthread_mutex_destroy(&mutex);
#endif
  printf("result: %i\n", a);
  return 0;
}
