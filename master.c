/************************************************
 * $Author: o-rooneo $
 *
 * $Date: 2015/03/03 00:51:37 $
 *
 * $Log: master.c,v $
 * Revision 1.1  2015/03/03 00:51:37  o-rooneo
 * Initial revision
 *
 * Revision 1.4  2015/02/26 00:56:37  o-rooneo
 * Fixed memory leak
 *
 * Revision 1.3  2015/02/18 15:28:17  o-rooneo
 * Shared memory and signal handling implemented
 *
 * Revision 1.2  2015/02/16 05:51:16  o-rooneo
 * Multiple consumer creation, shared memory, and signal control implemented
 *
 * Revision 1.1  2015/02/15 06:36:22  o-rooneo
 * Initial revision
 *
 ***********************************************/

/*
 * This program will fork off a single producer
 * and a number of consumers to read from a file 
 * and write to a file using five buffers, 
 * simulating a log file.
*/

#include "definitions.h"

void die(char *);
void killAll(int reason);

int child[19];
int shmid[BUFFERCOUNT+1];
char *shm[BUFFERCOUNT+1];

static void signal_handler(int signum){
	if(signum == 14) fprintf(stderr, "Timeout: ending processing\n");
	killAll(signum);
}

int main(int argc, char **argv)
{

	int consumerCount = 10;
	int i, status, result;
	char countHolder[2];
	for(i=0;i <= 19; i++) child[i] = 0;
	if(argc == 2){
		consumerCount = atoi(argv[1]);
		if(consumerCount > 18){
			printf("This program restricts consumers to 18. Consumer count set to default (10)\n");
			consumerCount = 10;
		}
		if(consumerCount < 1){
			printf("This program must have at least 1 consumer process. Consumer count set to default (10)\n");
			consumerCount = 10;
		}
	}
	if(argc > 2){
		printf("This program is desinged to take a single argument for consumer count.\nCount set to default of 10\n");
	}

	signal(SIGINT, signal_handler); //From here on out there are process and memory that need to be cleaned if SIGINT received
	//Create the correct amount of buffers
	for(i=0; i<=BUFFERCOUNT; i++){
		//Create shared mem segment only accessible to processes created by the same user
		if ((shmid[i] = shmget(BASESHMKEY+i, BUFFERSIZE, IPC_CREAT | 0700)) < 0)
			die("shmget");
		
		//We'll need a pointer to the shared mem to clean up
		if ((shm[i] = shmat(shmid[i], NULL, 0)) == (char *) -1)
			die("shmat");
	}

	alarm(300); //allow five minute run-time
	signal(SIGALRM, signal_handler);


	for(i=0;i <= consumerCount; i++){
		child[i] = fork();
		if(child[i] == -1){								//If we failed to fork then error out
			fprintf(stderr, "%s: Failed to fork", argv[0]);
			return 1;
		}
		if(child[i] == 0){		
			
			if(i == 0){
				result = execl("./producer","producer",(const char *) 0);	//Then replace the process with producer
				if(result == -1) exit(-1);
			}else{
				sprintf(countHolder, "%d", i);
				execl("./consumer","consumer", countHolder,(const char *) 0);	
				if(result == -1) exit(-1);
			}
		}
	}
	
	for(i=0;i <= 19; i++){ //loop through all 19 possible children and make sure they are waited for
		if(child[i] > 0) waitpid(child[i], &status, 0);
	}
	
	for(i=0;i<=BUFFERCOUNT;i++){
		shmdt(shm[i]);
		shmctl(shmid[i], IPC_RMID, NULL); //delete
	}
	printf("\n");
	return 0;
}

void die(char *s)
{
	int i;
    fprintf(stderr, "%s: die", s);
	for(i=0;i<=BUFFERCOUNT;i++){
		shmdt(shm[i]);
	}
	shmctl(shmid[i], IPC_RMID, NULL); //delete
    exit(1);
}

void killAll(int reason)
{
	int i, status;
	printf("\nSending SIGTERM to all children due to a ^C interrupt.\n");
	for(i=0;i <= 19; i++){ //Send SIGTERM to all children
		if(child[i] > 0) kill(child[i], SIGTERM);
	}
	for(i=0;i <= 19; i++) if(child[i] > 0) waitpid(child[i], &status, 0); //prevent printing prompt until all process return
	for(i=0;i<=BUFFERCOUNT;i++){
		shmdt(shm[i]);
	}
	shmctl(shmid[i], IPC_RMID, NULL); //delete
	exit(1);
}
