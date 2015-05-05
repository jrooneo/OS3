/************************************************
 * $Author: o-rooneo $
 *
 * $Date: 2015/02/26 00:57:56 $
 *
 * $Log: consumer.c,v $
 * Revision 1.2  2015/02/26 00:57:56  o-rooneo
 * outputting without doubling up on lines
 *
 * Revision 1.1  2015/02/25 20:24:30  o-rooneo
 * Initial revision
 *
 * Revision 1.1  2015/02/16 05:53:08  o-rooneo
 * Initial revision
 *
 *
 ***********************************************/
 
#include "definitions.h"
#include "fileHandler.h"

char exe[20];
char err[50];
int shmid[BUFFERCOUNT+1];
char *shm[BUFFERCOUNT+1];

enum state { idle, want_in, in_cs };
int *turn;
int flag[18]; // Flag corresponding to each process in shared memory

static void signal_handler(int signum)
{
	int i;
	for(i=0; i<=BUFFERCOUNT; i++){
		shmdt(shm[i]);
	}
	perror("consumer killed from interrupt");
	exit(1);
}

int main(int argc, char **argv)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, signal_handler);
	FILE *fp, *log, *unshared;
	int status, process=atoi(argv[1]);
	int i, n = 18; //max consumer count
	bool rerun = false, allEmpty= false;
	char *shared[BUFFERCOUNT+1], temp[BUFFERSIZE];
	int bufferFlags[5];
	int semid1;
	unsigned short semval1;
	time_t t;
	srand((unsigned) time(&t));

	char personalLog[14];
	sprintf(personalLog, "unshared%i.log", process);
	unshared = fileHandlerWrite(personalLog);

	//Semaphore definitions
	struct sembuf wait1,signal1;

	wait1.sem_num = 0;
	wait1.sem_op = -1;
	wait1.sem_flg = SEM_UNDO;

	signal1.sem_num = 0;
	signal1.sem_op = 1;
	signal1.sem_flg = SEM_UNDO;

	semid1 = semget(key1,1,IPC_CREAT | 0666);
	printf("Consumer %i allocating the semaphore: %s\n",process, strerror(errno));

	semval1 = 1;
	semctl(semid1,0,SETVAL,semval1);
	printf("Consumer %i setting semaphore value to %d: %s\n",process,semval1,strerror(errno));

	semval1 = semctl(semid1,0,GETVAL);
	printf("Consumer %i initialized semaphore value to %d: %s\n",process,semval1,strerror(errno));

	for(i=0;i<=BUFFERCOUNT;i++){	 //Buffers start at index 1 and go to BUFFERCOUNT
		shmid[i] = shmget(BASESHMKEY+i, BUFFERSIZE, 0700);
	}

	for(i=0; i<=BUFFERCOUNT; i++){
		if (shmid[i] == -1){ 
			snprintf(err, 50, "%s: Producer shm error", exe);
			perror(err);
			exit(1);
		}
	}

	for(i=0;i<=BUFFERCOUNT;i++){
		shm[i] = (char *)(shmat(shmid[i],0,0));
		shared[i] = (char *)shm[i];
	}
	
	do{	
		/***************** Critical Section Begin *********************/
		semop(semid1,&wait1,1);
		for (i = 1; i <= BUFFERCOUNT; i++ ){
			if(shared[0][i]==1){
				fp = fileHandlerWrite("consumerFile.log");						
				fprintf(fp, "Consumer %i %s",process, shared[i]);
				fclose(fp);
				log = fileHandlerWrite("shared.log");	
				fprintf(log, "Consumer %i %s",process, shared[i]);
				fclose(log);
				fprintf(unshared, "Consumer %i %s",process, shared[i]);
				rerun = false;
				allEmpty = false;
				shared[0][i]=0;
				semop(semid1,&signal1,1);
				break;
			}
		}
		if(allEmpty) semop(semid1,&signal1,1);
		allEmpty = true;
		/***************** Critical Section End ***********************/
	}while(shared[0][0]!=1);
	for(i=0; i<=BUFFERCOUNT; i++){
		shmdt(shm[i]);
	}
	fclose(unshared);
	exit(0);
}

