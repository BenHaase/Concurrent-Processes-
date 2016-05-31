/* Ben Haase
 * CS 4760
 * Assignment 2
 *$Author: o-haase $
 *$Date: 2016/02/24 21:59:03 $
 *$Log: master.c,v $
 *Revision 1.5  2016/02/24 21:59:03  o-haase
 *Minor changes and polishing
 *
 *Revision 1.4  2016/02/24 00:00:43  o-haase
 *Added use of signals and handlers for ^C and timer function
 *
 *Revision 1.3  2016/02/20 00:37:29  o-haase
 *Added shared memory creation and removal
 *
 *Revision 1.2  2016/02/19 00:47:38  o-haase
 *Implemented process spawn and wait
 *
 *Revision 1.1  2016/02/18 22:03:59  o-haase
 *Initial revision
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "memstrct.h"
	
void test(pt*, int);
void sigint_handler(int);
void timeout_handler(int);

int shmid; //memory var for shmat return val
pid_t slvs[19]; //hold slave process actual pid numbers

int main(int argc, char* argv[]){
	int t;
	int tkill = 60;
	//getopt for time limit if not specied time limit = 60 seconds
	if((t = getopt(argc, argv, "t::")) != -1){
		if(t == 't') tkill = atoi(argv[2]);
		else{
			perror("Error: Options character invalid");
			exit(0);
		}
	}

	srand(time(NULL)); //initialize srand

	//request and format shared memory
	shmid = shmget(SHMKEY, BUFF_SZ, 0777 | IPC_CREAT);
	if(shmid == -1){
		perror("Master: Error in shmget. \n");
		exit(1);
	}
	char * paddr = (char*)(shmat(shmid, 0, 0));
	pt * m = (pt*)(paddr);

	//set flags to idle
	m->flag[0] = idle;
	int i;
	for(i = 1; i < 20; i++) m->flag[i] = idle;

	test(m, tkill); //spawn processes

	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL); //clean up memory

	return 0;
}

//function to spawn the process and run the program
void test(pt * m, int timekill){
	pid_t x;
	int status; //hold status of finished slave process
	int n = 19; //number of processes
	int i;
	int fail = 0; //incremented if failure, flag for termination
	char t[3]; //hold slave process number

	//fork and exec processes 1-19
	for(i = 0; i < 19; i++){
		if((slvs[i] = fork()) < 0){
			perror("Fork Failed \n");
			fail = 1;
			return;
		}

		if(slvs[i] == 0){
			sprintf(t, "%i", (i + 1));
			if((execl((char*)"slave", (char*)"slave", t, (char*)NULL)) == -1){
				perror("Exec Failed");
				fail = 1;
				return;
			}
		}
	}

	//register signal handlers and set time limit
	signal(SIGINT, sigint_handler);
	signal(SIGALRM, timeout_handler);
	alarm(timekill);

	if(fail > 0) alarm(0); //send signal to end processes if error occured

	//wait for the processes to finish
	while(n > 0){
		x = wait(&status);
		//fprintf(stderr, "Child PID: %ld is done, status: 0x%x \n", (long)x, status);
		--n;
	}
}

//signal handler for SIGINT
void sigint_handler(int s){
	int i;
	printf("\n");
	for(i = 0; i < 19; i++) kill(slvs[i], SIGTERM);
	i = 19;
	while(i > 0){
		wait();
		--i;
	}
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL);
	fprintf(stderr, "Master process: 0 dying due to interrupt \n");
	exit(1);
}

//signal handler for SIGALRM (timeout)
void timeout_handler(int s){
	int i;
	printf("\n");
	for(i = 0; i < 19; i++) kill(slvs[i], SIGQUIT);
	i = 19;
	while(i > 0){
		wait();
		--i;
	}
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL);
	fprintf(stderr, "Master process: 0 dying due to timeout \n");
	exit(1);
}
