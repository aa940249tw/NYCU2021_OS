#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>

typedef long l;

typedef struct {
	int num;
	int *arr;
	int len;
} Pass;

double time_diff(struct timeval *start, struct timeval *end) {
    return 1e6*(end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec);
}

void normal(int *arr, int size, int num) {
	//printf("Normal Ver:\n");
	double avg_time = 0;
	for(int i = 0; i < 10; i++) {
		int count = 0;
		struct timeval start;
		struct timeval end;
		gettimeofday(&start, NULL);
		for(int j = 0; j < size; j++) count += (arr[j] == num ? 1 : 0);
		gettimeofday(&end, NULL);
		printf("Test %d: time spent: %4.lf usec | Found \"%d\" %d times.\n", i, time_diff(&start, &end), num, count);
		avg_time += time_diff(&start, &end)*0.1;
	}
	printf("Average time of Normal:             %-8.2lf\n", avg_time);
}

void multi_process(int *arr, int size, int num, int process_num) {
	//printf("Multi-process Ver:\n");
	double avg_time = 0;
	for(int i = 0; i < 10; i++) {
		int count = 0;
		pid_t p;
		struct timeval start;
		struct timeval end;
		gettimeofday(&start, NULL);
		for(int j = 0; j < process_num; j++) {
			p = fork();
			if(p < 0) printf("Fork failed\n");
			else if(p == 0) {
				int p_count = 0;
				for(int k = j*(size/process_num); k < (j+1)*(size/process_num); k++) p_count += (arr[k] == num ? 1 : 0);
				exit(p_count);
			}
		}
		int status, ret;
		for(int j = 0; j < process_num; j++) {
			ret = wait(&status);
			count += WEXITSTATUS(status);
		}
		gettimeofday(&end, NULL);
		//printf("Test %d: time spent: %4.lf usec | Found \"%d\" %d times.\n", i, time_diff(&start, &end), num, count);
		avg_time += time_diff(&start, &end)*0.1;
	}
	printf("Average time of Multi-process:      %-8.2lf\n", avg_time);
}

int thread_counting = 0;
pthread_mutex_t mutex;

void *function_mutex(void *arg) {
	Pass *t_args = (Pass *)arg;
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < t_args->len; i++) {
		thread_counting += (t_args->arr[i] == t_args->num ? 1 : 0);
	}
	pthread_mutex_unlock(&mutex);
	return NULL;
}

void multi_thread_mutex(int *arr, int size, int num, int thread_num) {
	//printf("Multi-thread-Mutex Ver:\n");
	double avg_time = 0;
	int **tmp_arr = malloc(size * sizeof(int));
	Pass *args = malloc(sizeof(Pass) * thread_num);
	for(size_t i = 0; i < thread_num; i++) {
		int *t_arr = malloc((size/thread_num) * sizeof(int));
		for(size_t j = 0; j < (size/thread_num); j++) *(t_arr+j) = arr[i*(size/thread_num)+j];
		tmp_arr[i] = t_arr;
		(args+i)->num = num;
		(args+i)->arr = tmp_arr[i];
		(args+i)->len = size/thread_num;
	}
	for(int i = 0; i < 10; i++) {
		thread_counting = 0;
		pthread_t p[thread_num];
		struct timeval start;
		struct timeval end;
		gettimeofday(&start, NULL);
		for(size_t j = 0; j < thread_num; j++) pthread_create(p+j, NULL, function_mutex, args+j);
		for(size_t j = 0; j < thread_num; j++) pthread_join(p[j], NULL);
		gettimeofday(&end, NULL);
		printf("Test %d: time spent: %4.lf usec | Found \"%d\" %d times.\n", i, time_diff(&start, &end), num, thread_counting);
		avg_time += time_diff(&start, &end)*0.1;
	}
	printf("Average time of Multi-Thread-Mutex: %-8.2lf\n", avg_time);
	free(tmp_arr);
}

void *function(void *arg) {
	l count = 0;
	Pass *t_args = (Pass *)arg;
	for(int i = 0; i < t_args->len; i++) count += (t_args->arr[i] == t_args->num ? 1 : 0);
	return (void *) count;
}

void multi_thread(int *arr, int size, int num, int thread_num) {
	//printf("Multi-thread Ver:\n");
	double avg_time = 0;
	int **tmp_arr = malloc(size * sizeof(int));
	Pass *args = malloc(sizeof(Pass) * thread_num);
	for(size_t i = 0; i < thread_num; i++) {
		int *t_arr = malloc((size/thread_num) * sizeof(int));
		for(size_t j = 0; j < (size/thread_num); j++) *(t_arr+j) = arr[i*(size/thread_num)+j];
		tmp_arr[i] = t_arr;
		(args+i)->num = num;
		(args+i)->arr = tmp_arr[i];
		(args+i)->len = size/thread_num;
	}
	for(int i = 0; i < 10; i++) {
		l count = 0;
		pthread_t p[thread_num];
		struct timeval start;
		struct timeval end;
		gettimeofday(&start, NULL);
		for(size_t j = 0; j < thread_num; j++) pthread_create(p+j, NULL, function, args+j);
		void *tmp;
		for(size_t j = 0; j < thread_num; j++) {
			pthread_join(p[j], &tmp);
			count += (l) tmp;
		}
		gettimeofday(&end, NULL);
		//printf("Test %d: time spent: %4.lf usec | Found \"%d\" %ld times.\n", i, time_diff(&start, &end), num, count);
		avg_time += time_diff(&start, &end)*0.1;
	}
	printf("Average time of Multi-Thread:       %-8.2lf\n", avg_time);
	free(tmp_arr);
}


void generate_and_count(int size, int num) {
	int *arr = malloc(size * sizeof(int));
	for(int i = 0; i < size; i++) arr[i] = rand() % 1000000;
	normal(arr, size, num);
	multi_process(arr, size, num, 10);
	multi_thread(arr, size, num, 10);
	multi_thread_mutex(arr, size, num, 10);
	free(arr);
}

int main(int argc, char *argv[]) {
	char *p;
	int num = (int)strtol(argv[1], &p, 10);
	int n_size[7] = {100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
	srand(0);
	pthread_mutex_init(&mutex, 0);
	for(int i = 0; i < 7; i++) {
		printf("Array size: %d\n", n_size[i]);
		generate_and_count(n_size[i], num);
	}
	pthread_mutex_destroy(&mutex);
}


