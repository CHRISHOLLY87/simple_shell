/*
 * ECSE 427 - Operating Systems	FALL 2017
 * A1 - Shell program
 * Saleh Bakhit - 260632353 
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include<time.h>

int commandNumber = 0;	//keeps track of the number of jobs + command number
struct node *head_job = NULL;
struct node *current_job = NULL;
pid_t process_pid;	//global so parent can access pid of child

//---TA/PROF PROVIDED FUNCTIONS---
//clears args to accept next command
void initialize(char *args[]) {
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}
	return;
}

struct node {	//node struc for jobs linked list
	int number;	//job number
	int pid; //pid of bg process
	struct node *next;
	struct node *prev;
};

//adds bg job to list
void addToJobList(char *args[]) {
	struct node *job = malloc(sizeof(struct node));

	//If the job list is empty, create a new head
	if (head_job == NULL) {
		job->number = 1;
		job->pid = process_pid;

		//the new head is also the current node
		job->next = NULL;
		job->prev = NULL;
		head_job = job;
		current_job = head_job;
	}

	//Otherwise create a new job node and link the current node to it
	else {

		job->number = current_job->number + 1;
		job->pid = process_pid;

		job->prev = current_job;
		current_job->next = job;
		current_job = job;
		job->next = NULL;
	}
	commandNumber++;
}

//delay for bg processes
void rDelay() {
	int w,rem;
	w = rand() % 10;
	rem = sleep(w);
	while(rem != 0) {
		rem = sleep(rem);
	}
}

int getcmd(char *line, char *args[], int *background) {
	int i = 0;
	char *token, *loc;

	//Copy the line to a new char array because after the tokenization a big
	//part of the line gets deleted since the null pointer is moved
	char *copyCmd = malloc(sizeof(char) * sizeof(line) * strlen(line));
	sprintf(copyCmd, "%s", line);

	// Check if background is specified..
	if ((loc = index(line, '&')) !=NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;

	//Create a new line pointer to solve the problem of memory leaking created by strsep() and getline() when making line = NULL
	char *lineCopy = line;
	while ((token = strsep(&lineCopy, " \t\n")) != NULL) {
		for (int j = 0; j < strlen(token); j++)
			if (token[j] <= 32)
				token[j] = '\0';
		if (strlen(token) > 0)
			args[i++] = token;
	}
	
	return i;
}
//---ENG OF TA/PROF PROVIDED FUNCTIONS---

//---HELPER FUNCTIONS---
//returns index of ">" (determines if redirection is specified)
int redirectIndex(char *args[], int cnt) {	//determines if output redirection is specified + position of ">"
	for(int i = 0; i < cnt; i++) {
		if(!strcmp(">", args[i])) return i;
	}
	return 0;
}

//kill background jobs(if any)
//jobs are not foregrounded before killing since its no longer a requirement
static void killBg(int sig) {
	if(head_job == NULL) return;
	else {	//look for active jobs, foregroumnd and kill
		struct node *tmp = head_job;
		int status;
		pid_t pid;
		while (tmp != NULL) {	//traverse entire list
			pid = waitpid(tmp->pid, &status, WNOHANG);	//0 if bg job is "EXECUTING"
			if(pid == 0) {	//"EXECUTING", kill process
				kill(tmp->pid, SIGKILL);
			}
			tmp = tmp->next;
		}
		head_job = NULL;
		printf("\nrunning background jobs have been terminated\n");
	}
}
//---END OF HELPER FUNCTIONS---

int main(void) {
	char *args[20];
	int bg;	//1 if bg specified, 0 if not
	
	time_t now;	//time variable to introduce a delay to background jobs
	srand((unsigned int) (time(&now)));

	//get user, if not name it "User"
	char *user = getenv("USER");
	if (user == NULL) user = "User";


	char str[sizeof(char)*strlen(user) + 4];
	sprintf(str, "\n%s>> ", user);

	if(signal(SIGTSTP, SIG_IGN) == SIG_ERR) {	//ignore <CTRL><Z> signal
		printf("ERROR! Could't ignore signal <CTRL><Z>");
	}
	if(signal(SIGINT, killBg) == SIG_ERR) {
		printf("ERROR! Could't implement signal <CTRL><C>");
	}

	while (1) {	//keep accepting commands until exit signal is triggered
		initialize(args);	//initialize to null for next command
		bg = 0;	//reset bg indicator

		int length = 0;
		char *line = NULL;
		char *buf;
		size_t linecap = 0; // 16 bit unsigned integer
		sprintf(str, "\n%s$ ", user);
		printf("%s", str);

		length = getline(&line, &linecap, stdin);
		
		if (length <= 0) { //exit when <ctrl><D> --> length = -1
			printf("<ctrl><D>\n");
			exit(-1);
		}
		
		int cnt  = getcmd(line, args, &bg); //cnt = numnber of args
		
		if(args[0] == NULL) continue;	//no command entered don't do anything
		
		//command is "pwd" OR "cp"
		else if (!strcmp("pwd", args[0]) || !strcmp("cp", args[0])) {
			process_pid = fork();
			if (process_pid == 0) {	//child runs
				if(bg == 1) rDelay();	//add delay if in bg
				execvp(args[0], args);
			}
			else {	//parent
				if(bg == 1) addToJobList(args);	//add to job list and accept more commands
				else {	//wait
					int status;
					waitpid(process_pid, &status, WUNTRACED);
				}
			}
        }
		
		//command is "cd"
		else if (!strcmp("cd", args[0])) {
			int result = 0;
			//HOME directory
			if (args[1] == NULL) {
				char *home = getenv("HOME");
				if (home != NULL) {
					result = chdir(home);
				}
				else {
					printf("cd: No $HOME variable declared in the environment");
				}
			}
			//Otherwise go to specified directory
			else {
				result = chdir(args[1]);
			}
			if (result == -1) fprintf(stderr, "cd: %s: No such file or directory", args[1]);

		}
		
		//command is "cat" OR "ls"
		else if(!strcmp("cat", args[0]) || !strcmp("ls", args[0])) {
			int i = redirectIndex(args, cnt);	//i is where ">" is, if not i=0

			if(i != 0) {	//redirection is specified (assuming one at a time)
				process_pid = fork();
				if(process_pid == 0) {	//child runs
					/*
					file1 > file2
					close stdout, open(truncate file) OR create a file2 = argument after ">".
					after dup(), when we execvp using file1, output is redirected to file2
					*/
					close(1);
					int fd = open(args[i+1], O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
					dup(fd);
					for(int j = i; j < cnt; j++) {	//all arguments ">" and after should be removed
						args[j] = NULL;
					}
					if(bg == 1) rDelay();	//if bg is specified, add delay
					execvp(args[0], args);
				}
				else {	//parent
					if(bg == 1) addToJobList(args);	//bg specified, add to job list, accept more commands
					else {	//no bg, wait
						int status;
						waitpid(process_pid, &status, WUNTRACED);
					}
				}
			}
			else {	//no output redirection
				process_pid = fork();
				if (process_pid == 0) {	//child runs
					if(bg == 1) rDelay();	//if bg is specified, add delay
					execvp(args[0], args);
				}
				else {	//parent
					if(bg == 1) addToJobList(args);	//bg specified, add to job list, accept more commands
					else {	//no bg, wait
						int status;
						waitpid(process_pid, &status, WUNTRACED);
					}
				}
			}
        }
		
		//command is "fg"
        else if(!strcmp("fg", args[0])) {
			if(head_job == NULL) printf("No background jobs found");
			pid_t pid;
			int status;
			struct node *tmp = head_job;
			while(tmp != NULL) {	//to make sure shell doesn't crash if fg requested is > cnt
				if(tmp->number == (int)(*args[1] - '0')) {	//find index requested
					pid = waitpid(tmp->pid, &status, WNOHANG);
					printf("REQUEST: [%d]\t%d\t\n", tmp->number, tmp->pid);
					if(pid == 0) {	//wait if "EXECUTING", then remove
						printf("STATUS: EXECUTING\n");
						waitpid(tmp->pid, &status, WUNTRACED);
					}
					else {	//already finished executing
						printf("STATUS: DONE\n");
					}
					//removing node
					if(tmp == current_job) {	//tail node
						current_job = tmp->prev;
						current_job->next == NULL;
						free(tmp);
					}
					else if(tmp == head_job) {	//head node
						head_job = tmp->next;
						head_job->prev = NULL;
						free(tmp);
					}
					else {	//mid node
						tmp->prev->next = tmp->next;
						tmp->next->prev = tmp->prev;
						free(tmp);
					}
					commandNumber--;	//decrement #JOBS
					break;	//once found, exit loop.
				}
				tmp = tmp->next;
			}
			if(commandNumber == 0) head_job = NULL;	//reset head
		}
		
		//command is "jobs"
        else if(!strcmp("jobs", args[0])) {
			if(head_job == NULL) printf("No background jobs found");
			else {	//check if a job has finished, if so remove it
				struct node *tmp = head_job;
				int status;
				pid_t pid;
				printf("#\tpid\tstatus\n");
				while (tmp != NULL) {	//traverse entire list
					pid = waitpid(tmp->pid, &status, WNOHANG);	//0 if "EXECUTING", child pid if "DONE"
					if(pid == tmp->pid) {	//"DONE", display status and remove node
						printf("[%d]\t%d\tDONE\n", tmp->number, tmp->pid);
						//removing node
						if(tmp == current_job) {	//head node
							current_job = tmp->prev;
							current_job->next == NULL;
							free(tmp);
						}
						else if(tmp == head_job) {	//tail node
							head_job = tmp->next;
							head_job->prev = NULL;
							free(tmp);
						}
						else {	//mid node
							tmp->prev->next = tmp->next;
							tmp->next->prev = tmp->prev;
							free(tmp);
						}
						commandNumber--;	//decrement #JOBS
					}
					else if(pid == 0) {	//"EXECUTING", display status
						printf("[%d]\t%d\texecuting\n", tmp->number, tmp->pid);
					}
					tmp = tmp->next;
				}
				if(commandNumber == 0) head_job = NULL;	//if all list "DONE", list is empty
			}
		}

		//command is exit
        else if(!strcmp("exit", args[0])) {
            exit(EXIT_SUCCESS);
        }
		
		//invalid command
        else {
            printf("%s: command not found\n", args[0]);
		}
		
		free(line);
	}
}
