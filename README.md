# OperatingSystem

Project 1 - My own shell which can execute all Unix Commands:
-------------------------------------------------------------
A C program called myshell.c, which, when compiled and run, will do what the shell does, namely:

It executes in a loop (until user types exit on the keyboard); 
Prints a prompt on the screen;
Reads the command typed on the keyboard (terminated by \n); 
Creates a new process and lets the child execute the user’s command;
Waits for the child to terminate and then goes back to beginning of the loop.
If the command typed is exit, then it terminates.
Also it prints the total number of commands executed just before terminating your program.

Assumption: Each line represents one command only, no command will end with & (all commands will be attached commands, no background execution), user will not type ^c or ^z, all commands are simple commands, etc.

Project 2 - Design Unix File System:
------------------------------------

