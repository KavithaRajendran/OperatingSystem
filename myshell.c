/********************************************************************************************************************************************************
 *
 * File Name: myshell.c
 *
 * How to execute this file:
 * 	gcc -o output_file_name myshell.c
 * 	./output_file_name
 * 
 * Description:
 * 	This programs creates a shell like environment.
 * 	Receives input from user and every user given command is run by a new child process
 * 	Parent process waits for child to terminate and then waits for next input from user in continuous loop until user types exit.
 * 
 * Authors: 
 * 	Name	    	        
 *	Kavitha Rajendran       
 *	Rajkumar PannerSelvam   
 *
*********************************************************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#define MAX 1024
int fork(void);
int getpid(void);
int getppid(void);
int sleep(int);
int wait(int*);
void exit(int);

char input[MAX];
char* commandsArgv[256];

/* This function get input from user and parse it for execvp */
void input_parser()
{
    
}

void main()
{
    
    char input[MAX];
    char* commandsArgv[256];
	int stat,pid,res;
    int num_of_cmds=0;
	while(1)
	{
	    // Shell starts here --> printing prompt
	    printf(">>"); 
	    // Gets user input
	    if(fgets(input,sizeof(input),stdin))
	    {
  	        char* s;
	        int spaceCharIndex = strlen(input)-1;
            int j=0;
    	    input[spaceCharIndex]='\0';
            commandsArgv[j] = strtok(input, " " );

            while( commandsArgv[j]!=NULL)
            {
                commandsArgv[++j]=strtok(NULL, " " );

            }
            //int k;
	    	//for (k=0; k < j ; k++)
	    	//{
	        //   printf ( "%s input split \n" , commandsArgv[k]);
	        //}
            if(commandsArgv[0]!=NULL)
            {
	        // if user enters 'exit', comeout of loop
    	    res = strcmp(input,"exit");
	        if (res == 0)
    	    {	
	    	    printf("Total Number of commands Given: %d \n",num_of_cmds);
		        printf("Terminating... \n");
    		    break;
	        }
	        else
	        {
	            num_of_cmds++;
                if(!strcmp(commandsArgv[0],"cd"))
                {
                    printf("Change directory command is given \n");
                    if(commandsArgv[1]!=NULL)
                    {
                        chdir(commandsArgv[1]);
                    }
                    else
                    {
                        chdir();
                    }
                }
                else 
                {
		        //Creating a child
	    	    pid = fork();
		        if(pid == 0)
		        {
		    	    execvp(commandsArgv[0],commandsArgv);
                    printf("%s: command not found...\n",commandsArgv[0]);
		    	    exit(0);
		        }
		        else
		        {
			        //wait for child to terminate
			        wait(&stat);
			        //printf("Parent:Value of stat is %d\n",stat);
		        }
                }
	        }
            } 
	    }
	}
}
