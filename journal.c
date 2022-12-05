#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

#include "journal.h"


/**
 * ************** My code starts here ************************
 */

// number of needed threads
#define NUMBER_OF_THREAD 3


void writeData(struct write_request *wr);
void checkpoint(struct write_request *wr);

/**
 * ****** FIFO Queue implementation *************
 */
struct queue {
    struct write_request entries[BUFFER_SIZE];
    char *qName;
    int queueTracker;
} queue;

// Init the queue
void initQueue(struct queue *q, char *name) {
    q->queueTracker = 0;
    q->qName = name;
}

// Check weather the queue is full or not
bool isQueueFull(struct queue *q) {
    return q->queueTracker >= BUFFER_SIZE;
}

// Check weather the queue is full or not
bool isQueueEmpty(struct queue *q) {
    return q->queueTracker <= 0;
}

// Adding element to the queue
void enqueue(struct write_request *el, struct queue *q) {
    bool success = false;
    if (!isQueueFull(q)) {
        q->entries[q->queueTracker] = *el;
        q->queueTracker++;
        success = true;
    } else {
        printf("Warning: queue %s is full\n", q->qName);
    }

}

// Removing the first element of the queue and return it
struct write_request dequeue(struct queue *q) {
    struct write_request ex = q->entries[0];
    struct write_request temp;

    for (int i = 0; i < q->queueTracker; i++) {
        temp = q->entries[i + 1];
        q->entries[i] = temp;
    }
    q->queueTracker--;
    return ex;
}

void resetQueue(struct queue *q) {
   for (int i = 0; i < BUFFER_SIZE; i++) {
       q->entries[i].bitmap = NULL;
       q->entries[i].inode = NULL;
       q->entries[i].data = NULL;
       q->entries[i].bitmap_idx = 0;
       q->entries[i].inode_idx = 0;
       q->entries[i].data_idx = 0;
   }
   q->queueTracker = 0;
}

void qDisplay(struct queue *q) {
    for (int i = 0; i < q->queueTracker; i++) {
        printf("%s | %s | %s | %d | %d | %d [%s]\n", q->entries[i].bitmap, q->entries[i].inode, q->entries[i].data,
               q->entries[i].bitmap_idx, q->entries->inode_idx, q->entries->data_idx, q->qName);
    }
}

/**
 * ****** End FIFO Queue implementation *************
 */

// Buffers
struct queue requestBuffer;
struct queue journalMetaBuffer;
struct queue journalCommitBuffer;

// Array of threads
pthread_t threadsArray[NUMBER_OF_THREAD];

// Mutexes
pthread_mutex_t lock1;
pthread_mutex_t lock2;
pthread_mutex_t lock3;

// Semaphores
sem_t bufferEmpty1;
sem_t bufferFull1;

sem_t bufferEmpty2;
sem_t bufferFull2;

sem_t bufferEmpty3;
sem_t bufferFull3;


void *journalRequestWrite(void *args);

void *journalMetaCommit(void *args);

void *checkPointMetaData(void *args);

/*
 * This function can be used to initialize the buffers and threads.
 */
void init_journal() {
    // init buffers
    initQueue(&requestBuffer, "Request Buffer | 1");
    initQueue(&journalMetaBuffer, "Journal meta completed Buffer | 2");
    initQueue(&journalCommitBuffer, "Journal commit completed Buffer | 3");

    // init mutexes
    pthread_mutex_init(&lock1, NULL);
    pthread_mutex_init(&lock2, NULL);
    pthread_mutex_init(&lock3, NULL);

    // Init Semaphores
    sem_init(&bufferEmpty1, 0, BUFFER_SIZE); // initially  BUFFER_SIZE place Empty
    sem_init(&bufferFull1, 0, 0); // initially not full (0 place full)

    sem_init(&bufferEmpty2, 0, BUFFER_SIZE); // initially  BUFFER_SIZE place Empty
    sem_init(&bufferFull2, 0, 0); // initially not full (0 place full)

    sem_init(&bufferEmpty3, 0, BUFFER_SIZE); // initially  BUFFER_SIZE place Empty
    sem_init(&bufferFull3, 0, 0); // initially not full (0 place full)


    // threads init
    if (pthread_create(&threadsArray[0], NULL, &journalRequestWrite, NULL) != 0) {
        perror("Failed to create thread 0!");
    }

    if (pthread_create(&threadsArray[1], NULL, &journalMetaCommit, NULL) != 0) {
        perror("Failed to create thread 1!");
    }

    if (pthread_create(&threadsArray[2], NULL, &checkPointMetaData, NULL) != 0) {
        perror("Failed to create thread 2!");
    }

    pthread_mutex_destroy(&lock1);
    pthread_mutex_destroy(&lock2);
    pthread_mutex_destroy(&lock3);

    sem_destroy(&bufferEmpty1);
    sem_destroy(&bufferFull1);

    sem_destroy(&bufferEmpty2);
    sem_destroy(&bufferFull2);

    sem_destroy(&bufferEmpty3);
    sem_destroy(&bufferFull3);
}

// check if request buffer is not empty
void *journalRequestWrite(void *args) {
    while (1) {

        printf("ThREAD 1 IS WORKING ...\n");
        sem_wait(&bufferFull1);
        pthread_mutex_lock(&lock1);

        struct write_request takenOutItem = dequeue(&requestBuffer);
        sem_post(&bufferEmpty1);

        //TODO work need to be done!
        // call functions to write data and metadata
        // wait for all issued write to complete
        // once done enqueu
        writeData(&takenOutItem);

        if(isQueueFull(&journalMetaBuffer)) {
            printf("THE JOURNAL METADATA BUFFER IS FULL\n");
            printf("******** THREAD STUCK BECAUSE OF FULL BUFFER **********\n");
        }

        sem_wait(&bufferEmpty2);
        enqueue(&takenOutItem, &journalMetaBuffer);
        sem_post(&bufferFull2);

        pthread_mutex_unlock(&lock1);

    }
}

// check if request buffer is not empty
void *journalMetaCommit(void *args) {
    while (1) {
        
        printf("THREAD 2 IS WORKING ...\n");
        sem_wait(&bufferFull2);
        pthread_mutex_lock(&lock2);

       // qDisplay(&journalMetaBuffer);
        struct write_request takenOutItem = dequeue(&journalMetaBuffer);
        sem_post(&bufferEmpty2);
 
        //TODO work need to be done!
        // Issue journal txe
        // wait for completion of writing
        // once done enqueue
        // sleep(3);
        issue_journal_txe();

         if(isQueueFull(&journalCommitBuffer)) {
            printf("THE JOURNAL COMMIT COMPLETED BUFFER IS FULL\n");
        }

        sem_wait(&bufferEmpty3);
        enqueue(&takenOutItem, &journalCommitBuffer);
        sem_post(&bufferFull3);

        pthread_mutex_unlock(&lock2);
    }

}

// check if request buffer is not empty
void *checkPointMetaData(void *args) {
    while (1) {

        printf("THREAD 3 IS WORKING ...\n");
        sem_wait(&bufferFull3);
        pthread_mutex_lock(&lock3);

        struct write_request takenOutItem = dequeue(&journalCommitBuffer);
        sem_post(&bufferEmpty3);

        //TODO work need to be done!
        // Issue writing the metadata
        // wait for completion of writing the metadata
        // call write _complete
        checkpoint(&takenOutItem);
        // printf("****** Write completed ******\n");
        pthread_mutex_unlock(&lock3);

    }
}

/*
 * First function to call - thread 1
 * - Issues writing the metadata
 * - Wait for the completion of writing the metadata
 * - Call write complete()
 */
void writeData(struct write_request *wr) {
    issue_write_data(wr->data, wr->data_idx);
    issue_journal_txb();
    issue_journal_bitmap(wr->bitmap, wr->bitmap_idx);
    issue_journal_inode(wr->inode, wr->inode_idx);
}


/*
 * Last function to call - thread 3
 * - Issues writing the metadata
 * - Wait for the completion of writing the metadata
 * - Call write complete()
 */
void checkpoint(struct write_request *wr) {
    issue_write_bitmap(wr->bitmap, wr->bitmap_idx);
    issue_write_inode(wr->inode, wr->inode_idx);
    write_complete();
}

/**
 * # **************  End My code End here ***********
 */

/* This function is called by the file system to request writing entries to
 * persistent storage.
 *
 * This is the non-thread-safe solution to the problem. It issues all writes in
 * the correct order, but it doesn't wait for each phase to complete before
 * beginning the next. As a result the journal can become inconsistent and
 * unrecoverable.
 */
void request_write(struct write_request *wr) {
    // Enqueue new wr in the request buffer
    if(isQueueFull(&requestBuffer)) {
        printf("REQUEST BUFFER IS FULL\n");
    }
    sem_wait(&bufferEmpty1);
    enqueue(wr, &requestBuffer);
    sem_post(&bufferFull1);

}

/* This function is called by the block service when writing the txb block
 * to persistent storage is complete (e.g., it is physically written to disk).
 */
void journal_txb_complete() {
    printf("journal txb complete\n");
}

void journal_bitmap_complete() {
    printf("journal bitmap complete\n");
}

void journal_inode_complete() {
    printf("journal inode complete\n");
}

void write_data_complete() {
    printf("write entries complete\n");
}

void journal_txe_complete() {
    printf("journal txe complete\n");
}

void write_bitmap_complete() {
    printf("write bitmap complete\n");
}

void write_inode_complete() {
    printf("write inode complete\n");
}

// end

