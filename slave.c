/* Ben Haase
 * CS 4760
 * Assignment 2
 *$Author: o-haase $
 *$Date: 2016/02/24 22:56:43 $
 *$Log: slave.c,v $
 *Revision 1.6  2016/02/24 22:56:43  o-haase
 *Cleanup for submission
 *
 *Revision 1.5  2016/02/24 21:58:47  o-haase
 *Minor changes and polishing
 *
 *Revision 1.4  2016/02/24 00:02:09  o-haase
 *Added use of signals and handlers for ^C and timeout events
 *
 *Revision 1.3  2016/02/20 00:38:18  o-haase
 *Added use of shared memory to communicate between slave processes
 *Implemented multiple process solution
 *
 *Revision 1.2  2016/02/19 00:48:13  o-haase
 *Implemented code to test process spawning
 *
 *Revision 1.1  2016/02/18 22:04:36  o-haase
 *Initial revision
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "memstrct.h"

void critical_section();
void pmsg(char*, char*);
void sigtimeout_handler(int);
void sigctlc_handler(int);

//globals for use in signal handlers
char * paddr; //memory address returned from shmat
int pn; //integer representation of process number (from master)
FILE * f;
char * pnc; //character representation of process number (from master)

int main(int argc, char* argv[]){
	int j;
	int ncrits = 0; //number of times CS has been entered
	pn = atoi(argv[1]);
	int n = 20; //number of processes + 1
	pnc = argv[1];
	srand((unsigned)(time(NULL) + pn)); //initialize srand

	//mount shared memory
	int shmid = shmget(SHMKEY, BUFF_SZ, 0777);
	if(shmid == -1){
		perror("Slave: Error in shmget.");
		exit(1);
	}
	paddr = (char*)(shmat(shmid, 0, 0));
	pt * m = (pt*)(paddr);

	//setup signal handlers (ignore SIGINT that will be intercepted by master)
	signal(SIGQUIT, sigtimeout_handler);
	signal(SIGTERM, sigctlc_handler);
	signal(SIGINT, SIG_IGN);

	do{
		do{
			//declare intent to enter CS
			m->flag[pn] = want_in;
			j = m->turn;

			while(j != pn){
				j = (m->flag[j] != idle) ? (m->turn) : ((j + 1) % n);
			}

			//pmsg(pnc, "attempting to enter CS");
			m->flag[pn] = in_cs;

			//try to gain entry to CS
			for(j = 0; j < n; j++){
				if((j != pn) && (m->flag[j] == in_cs)){
					pmsg(pnc, "Failed to enter CS");
					break;
				}
			}

		}while((j < n) || ((m->turn != pn) && (m->flag[(m->turn)] != idle)));

		//critical section entry gained
		m->turn = pn;
		ncrits++;
		critical_section();
		pmsg(pnc, "out of CS");

		//search for next in line non-idle process
		j = (m->turn + 1) % n;
		while(m->flag[j] == idle){
			j = (j + 1) % n;
		}

		//set turn to next process, change flag to idle
		m->turn = j;
		m->flag[pn] = idle;
		sleep((rand() % 3));
	}while(ncrits < 3);

	//dismount memory and print finished message
	shmdt(paddr);
	pmsg(pnc, "done and detached");
	return 0;
}

//function for critical section, open file, sleep, write to file, close file
void critical_section(){
	pmsg(pnc, "is in CS");
	f = fopen("cstest", "a");
	time_t x;
	sleep((rand() % 3));
	time(&x);
	char * t = ctime(&x);

	if(f != NULL){
		fprintf(f, "%s %02s at time %.8s.\n", "File modified by process number", pnc, (t + 11));
	}

	fclose(f);
	f = NULL;
}

//function to print process messages with time
void pmsg(char *p, char *msg){
	time_t tm;
	char * tms;
	time(&tm);
	tms = ctime(&tm);
	fprintf(stderr, "Process: %02s %s at %.8s.\n", p, msg, (tms + 11));
}

//signal handler for SIGQUIT (sent from master on timeout)
void sigtimeout_handler(int s){
	if(f != NULL){
		fclose(f);
		f = NULL;
	}
	shmdt(paddr);
	fprintf(stderr, "Slave process: %02s dying due to timeout \n", pnc);
	exit(1);
}

//signal handle for SIGTERM (sent from master on ^C)
void sigctlc_handler(int s){
	if(f != NULL){
		fclose(f);
		f = NULL;
	}
	shmdt(paddr);
	fprintf(stderr, "Slave process: %02s dying due to interrupt \n", pnc);
	exit(1);
}
