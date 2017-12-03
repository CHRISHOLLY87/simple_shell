#include <stdio.h>
#include <stdlib.h>

int commandNumber = 0;
struct node *head_job = NULL;
struct node *current_job = NULL;

void addToJobList();
void display(struct node *tmp);


struct node {
    int number;
	struct node *next;
	struct node *prev;
}

void addToJobList() {
	struct node *job = malloc(sizeof(struct node));

	//If the job list is empty, create a new head
	if (head_job == NULL) {
		job->number = 1;

		//the new head is also the current node
		job->next = NULL;
		job->prev = NULL;
		head_job = job;
		current_job = head_job;
	}

	//Otherwise create a new job node and link the current node to it
	else {

		job->number = current_job->number + 1;

		job->prev = current_job;
		current_job->next = job;
		current_job = job;
		job->next = NULL;
	}
	commandNumber++;
	printf("\njob: [%d]\n", job->number);
}

void display(struct node *tmp) {	//treverse jobs linked list
	if(tmp == NULL) {
		printf("EMPTY LIST!\n");
		return;
	}
	printf("[%d]\t\n", tmp->number);
    if (tmp->next == NULL) return;
    display(tmp->next);
}


int main() {
    struct node *test = head_job;
    addToJobList();
    display(test);
    addToJobList();
    display(test);
    return 0;
}
