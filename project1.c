#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>

/* 
Max Jensen 
Project 1
10/23/20
This program creates a user specified number of network nodes (between 1 and 9), and delivers a message to a specified node using a token-ring network topology. Upon receiving an interrupt signal, the parent process and all the nodes close gracefully. 
*/

#define MAXMESSAGELENGTH 128
#define WRITEEND 1
#define READEND 0

int numNodes;
int dest;

void sigHandler(int sig, siginfo_t* si, void* context);

//Make a pipe array to handle the file descriptors
typedef int Pipe[2];

int main(/*int argc, char *argv[]*/){	
	//FDs for parent's pipe access
	int parWrite;
	int parRead;
	
	//PID
	pid_t pid;
	
	//Message string
	char message[MAXMESSAGELENGTH];

	//Temp strings for input info
	char temp1[MAXMESSAGELENGTH];
	char temp2[MAXMESSAGELENGTH];
	
	//Temp string to hold the message for the node
	char tempMsg[MAXMESSAGELENGTH];
	
	// Flag that tracks whether the message was successfully delivered or not
	int successfulDelivery = 0;
	
	//Signal handler vars and settings
	struct sigaction sVal;
	sVal.sa_flags = SA_SIGINFO;
	sVal.sa_sigaction = sigHandler;
	
	// Get number of nodes to create
	printf("\nHow many network nodes would you like to create? I can create from 1 to 9!\n");
	fgets(temp1, MAXMESSAGELENGTH, stdin);
	
	numNodes = (char) atoi(temp1);
	if ((numNodes > 0) && (numNodes < 10)){
		printf("\nThank you! I'll create %d nodes.\n", numNodes);
	}
	
	else{
		fprintf(stderr,"You must have passed a non-numerical value or a value not between 1 and 9. Please try again!\n");
		exit(1);
	}
	
	
	// Get destination node
	printf("\nWhat would you like the destination node to be? You can choose any node that has already been created\n");
	fgets(temp2, MAXMESSAGELENGTH, stdin);
	dest = atoi(temp2);
	
	if((dest > 0) && (dest <= numNodes)){
		printf("\nThank you! I'll send the message to node %d.\n", dest);
	}
	
	else{
		fprintf(stderr,"You must have passed a non-numerical value or node that does not exist. Please try again!\n");
		exit(1);
	}
	
	//Set parent PID
	pid = getpid();
	printf("Parent has a PID of %d.\n", pid);
	
	// Creates the pipe array.
	Pipe pipes[numNodes + 1];
	
	// Initialize the parent pipe.
	if (pipe(pipes[0]) < 0) {
		perror("Plumbing problem.\n");
		exit(1);
	}
	
	printf("The parent node has been created!\n");
	printf("Creating %d network nodes.\n", numNodes);
	printf("The message destination is set to node: %d.\n", dest);
	
	
	// Makes the pipes.
	for (int i = 1; i < numNodes + 1; i++) {
		if (pipe(pipes[i]) < 0) {
			perror("Pip creation failed.\n");
			exit(1);
		}
	
		printf("Made Pipe %d\n", i);
	
		//Makes the nodes
		pid_t pid = fork();
		
		if (pid < 0) {
			perror("Fork failed.");
			exit(1);
		}
		
		//Child code below
		else if(pid == 0){
			
			//Closes the ends of pipes the child doesn't need
			for (int x = 0; x < i-1; x++) {
				// Close the write end of the appropriate pipes
				close(pipes[x][WRITEEND]);
				// Close the write end of the appropriate pipes
				close(pipes[x][READEND]);
			}
			
			//Handles the final pipe closing
			close(pipes[i-1][WRITEEND]);
			
			//Handles the current process read close
			close(pipes[i][READEND]);
			
			// Loop for child.
			while (read(pipes[i-1][READEND], &tempMsg, MAXMESSAGELENGTH) > 0) {
				sleep(1);
				printf("Node %d with PID %d received the message: %s", i, getpid(), tempMsg);
				// If dest is the correct destination, clear the message
				if (i == dest && successfulDelivery == 0) {
					printf("The message reached the correct destination! Now removing the message from the token\n");
					strcpy(tempMsg, " \n");
					successfulDelivery = 1;
				}
				// If this isn't the correct destination, write message to the next child's pipe.
				write(pipes[i][WRITEEND], &tempMsg, MAXMESSAGELENGTH);
				printf("Node %d with PID %d is sending the token forward.\n", i, getpid());
			}
			// Clean up all the pipe FDs
			close(pipes[i-1][READEND]);
			close(pipes[i][WRITEEND]);
			exit(0);
		}
	}
	
	//Parent code
	//Creates the signal handler on the parent
	sigaction(SIGINT, &sVal, NULL);
	
	//Close unnecessary parent pipes
	for (int i = 1; i < numNodes; i++) {
		// Close write of pipe.
		close(pipes[i][WRITEEND]);
		// Close read of pipe.
		close(pipes[i][READEND]);
	}
	
	// Close read of the first pipe.
	close(pipes[0][READEND]);
	// Close write of the last pipe.
	close(pipes[numNodes][WRITEEND]);
	
	parWrite = pipes[0][WRITEEND];
	parRead = pipes[numNodes][READEND];

	printf("\nWhat message would you like to send? It must be fewer than 128 characters long.\n");
	fgets(message, MAXMESSAGELENGTH, stdin);
	printf("\nThank you! I'll send the message:\n%s\n", message);
	
	//Parent loop
	while (1) {
		write(parWrite, message, MAXMESSAGELENGTH);
		read(parRead, message, MAXMESSAGELENGTH);
		printf("Parent with PID %d received the message: %s", getpid(), message);
		// Clear message if there is one.
		printf("If there was still a message remaining in the token it has been cleared by the parent.\n");
		strcpy(message, " \n");
	}
	
	return 0;
		
}

/* Signal handler function */
void sigHandler(int sig, siginfo_t* si, void* context) {
	if (SIGINT) {
		sleep(1);
		// Close processes then announce their closing.
		printf("\nI received a closing interrupt.\n");
		printf("Closing the token-ring network\n");
		exit(0);
	}
}