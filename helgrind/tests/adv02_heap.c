#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//#define WASTE_TIME(v) { printf("waisting time\n"); v = 0; while ((v) < 10000000) v++; printf("finished waisting time\n"); }
#define WASTE_TIME pthread_yield();

int *q = NULL;
int *p = NULL;
int i, j, k;

void *f1() {
	q = malloc(sizeof(int));
	WASTE_TIME(i);
	*q = 3;
	WASTE_TIME(j);
	p = q;
	return NULL;
}

void *f2() {
	while (p == NULL)
		WASTE_TIME(k);
	assert(*p == 3);
	return NULL;
}

int main(int argc, char **argv) {
	pthread_t tid1, tid2;
	void *sptr1, *sptr2;
	int i = 0;
	for (; i < 100; i++) {
		pthread_create(&tid1, NULL, &f1, NULL);
		pthread_create(&tid2, NULL, &f2, NULL);
		pthread_join(tid1, &sptr1);
		pthread_join(tid2, &sptr2);
	}
}
