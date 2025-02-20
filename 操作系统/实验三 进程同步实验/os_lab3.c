#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;

sem_t empty;
sem_t full;
pthread_mutex_t mutex;

void* producer(void* arg) {
    while (1) {
        int item = rand() % 100;  // 生成一个随机数作为产品
        sleep(rand() % 3);

        sem_wait(&empty);
        pthread_mutex_lock(&mutex);

        buffer[in] = item;
        printf("生产者 %ld 生产了 %d 在位置 %d\n", pthread_self(), item, in);
        in = (in + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);
        sem_post(&full); 
    }
    return NULL;
}

void* consumer(void* arg) {
    while (1) {
        sleep(rand() % 3);
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        int item = buffer[out];
        printf("消费者 %ld 消费了 %d 从位置 %d\n", pthread_self(), item, out);
        out = (out + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "用法: %s <睡眠时长> <生产者线程数> <消费者线程数>\n", argv[0]);
        return 1;
    }

    int sleep_time = atoi(argv[1]);
    int producer_count = atoi(argv[2]);
    int consumer_count = atoi(argv[3]);

    pthread_t producers[producer_count], consumers[consumer_count];

    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0); 
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < producer_count; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }

    for (int i = 0; i < consumer_count; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    sleep(sleep_time);

    printf("时间到，结束程序\n");
    exit(0);

    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}
