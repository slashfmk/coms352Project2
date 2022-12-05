#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "journal.h"

int requestCount = 0;
pthread_t testingThread;

void* checkOnce(void* args) {
	sleep(1);
}


void issue_journal_txb() {
	printf("issue journal txb\n");
	journal_txb_complete();
}

void issue_journal_bitmap(char* bitmap, int idx) {
	printf("issue journal bitmap %s at %d\n", bitmap, idx);
	journal_bitmap_complete();
}

void issue_journal_inode(char* inode, int idx) {
	printf("issue journal inode %s at %d\n", inode, idx);
	journal_inode_complete();
}

void issue_write_data(char* data, int idx) {
	printf("issue write entries %s at %d\n", data, idx);
	write_data_complete();
}

void issue_journal_txe() {

printf("issue journal txe\n");

	if(requestCount == 0) {
		requestCount++;
		if(pthread_create(&testingThread, NULL, &checkOnce, NULL) != 0) {
			perror("Thread couldn't be created!!");
		}
	}

	journal_txe_complete();
}

void issue_write_bitmap(char* bitmap, int idx) {
	printf("issue write bitmap %s at %d\n", bitmap, idx);
	write_bitmap_complete();
}

void issue_write_inode(char* inode, int idx) {
	printf("issue write inode %s at %d\n", inode, idx);
	write_inode_complete();
}
