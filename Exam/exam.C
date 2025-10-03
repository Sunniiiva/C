#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define NUM_THREADS 2
#define BYTE_RANGE 256

int count[BYTE_RANGE];
unsigned char buffer[BUFFER_SIZE];
pthread_mutex_t mutex;
pthread_cond_t cond_full, cond_empty;
int bytes_in_buffer = 0;

void* thread_A(void* arg) {
    FILE* fp = fopen("task4_pg2265.txt", "rb");
    if (!fp) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&mutex);
        while (bytes_in_buffer == BUFFER_SIZE)
            pthread_cond_wait(&cond_empty, &mutex);

        int read_bytes = fread(buffer + bytes_in_buffer, 1, BUFFER_SIZE - bytes_in_buffer, fp);
        bytes_in_buffer += read_bytes;

        if (read_bytes < BUFFER_SIZE - bytes_in_buffer) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_cond_signal(&cond_full);
        pthread_mutex_unlock(&mutex);
    }

    fclose(fp);
    pthread_exit(NULL);
}

void* thread_B(void* arg) {
    memset(count, 0, sizeof(count));

    while (1) {
        pthread_mutex_lock(&mutex);
        while (bytes_in_buffer == 0)
            pthread_cond_wait(&cond_full, &mutex);

        for (int i = 0; i < bytes_in_buffer; i++)
            count[buffer[i]]++;

        bytes_in_buffer = 0;
        pthread_cond_signal(&cond_empty);
        pthread_mutex_unlock(&mutex);

       if (bytes_in_buffer == 0) 
            break;
    }
    for (int i = 0; i < BYTE_RANGE; i++)
        printf("%d: %d\n", i, count[i]);

    pthread_exit(NULL);
}

int main(void) {
    pthread_t threadA, threadB;
    void* memory_buffer = malloc(BUFFER_SIZE);

    if (pthread_create(&threadA, NULL, thread_A, (void*)memory_buffer) != 0) {
        perror("Could not create thread A");
        exit(1);
    }

    if (pthread_create(&threadB, NULL, thread_B, (void*)memory_buffer) != 0) {
        perror("Could not create thread B");
        exit(1);
    }

    if (pthread_join(threadA, NULL) != 0) {
        perror("Could not join thread A");
        exit(1);
    }

    if (pthread_join(threadB, NULL) != 0) {
        perror("Could not join thread B");
        exit(1);
    }

    free(memory_buffer);
    return 0;
}



//FIXED CODE:

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define BUFFER_SIZE 4096
#define BYTE_RANGE 256

// Structure to hold shared data between threads
typedef struct {
    unsigned char *buffer;
    int *count;
    int *bytes_in_buffer;
    pthread_mutex_t *mutex;
    sem_t *sem_empty;
    sem_t *sem_full;
    char *filename;
} ThreadData;

// Worker thread A: Reads data from file into buffer
void* thread_A(void* arg) {
    ThreadData *data = (ThreadData *)arg;
    FILE* fp = fopen(data->filename, "rb");
    if (!fp) {
        perror("Failed to open file");
        pthread_exit(NULL);
    }

    while (1) {
        sem_wait(data->sem_empty);
        pthread_mutex_lock(data->mutex);

        int read_bytes = fread(data->buffer, 1, BUFFER_SIZE, fp);
        *(data->bytes_in_buffer) = read_bytes;

        pthread_mutex_unlock(data->mutex);
        sem_post(data->sem_full);

        if (read_bytes == 0) { // End of file
            break;
        }
    }

    fclose(fp);
    pthread_exit(NULL);
}

// Worker thread B: Counts byte values from buffer
void* thread_B(void* arg) {
    ThreadData *data = (ThreadData *)arg;

    while (1) {
        sem_wait(data->sem_full);
        pthread_mutex_lock(data->mutex);

        if (*(data->bytes_in_buffer) == 0) {
            pthread_mutex_unlock(data->mutex);
            sem_post(data->sem_empty);
            break;
        }

        for (int i = 0; i < *(data->bytes_in_buffer); i++) {
            data->count[data->buffer[i]]++;
        }

        pthread_mutex_unlock(data->mutex);
        sem_post(data->sem_empty);
    }

    for (int i = 0; i < BYTE_RANGE; i++) {
        printf("%d: %d\n", i, data->count[i]);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pthread_t threadA, threadB;
    pthread_mutex_t mutex;
    sem_t sem_empty, sem_full;
    unsigned char *buffer = malloc(BUFFER_SIZE);
    int *count = calloc(BYTE_RANGE, sizeof(int));
    int bytes_in_buffer = 0;

    if (!buffer || !count) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem_empty, 0, 1);
    sem_init(&sem_full, 0, 0);

    ThreadData data = {
        .buffer = buffer,
        .count = count,
        .bytes_in_buffer = &bytes_in_buffer,
        .mutex = &mutex,
        .sem_empty = &sem_empty,
        .sem_full = &sem_full,
        .filename = argv[1]
    };

    if (pthread_create(&threadA, NULL, thread_A, &data) != 0) {
        perror("Could not create thread A");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&threadB, NULL, thread_B, &data) != 0) {
        perror("Could not create thread B");
        exit(EXIT_FAILURE);
    }

    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);

    pthread_mutex_destroy(&mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);

    free(buffer);
    free(count);

    return 0;
}
