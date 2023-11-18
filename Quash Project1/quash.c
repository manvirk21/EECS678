#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE 1000
#define FILE_PERMISSIONS 0644

// Declaring several global variables and arrays.
char *inputBuffer;
char *arguments[SIZE];
char *commandList[SIZE];
char *exportrguments[SIZE];
char currentDir[SIZE];
char directory[SIZE];

/**
 * The below type defines a struct called "job" with fields for name, index, status, and process ID.
 * @property {char} Name - A pointer to a character array that represents the name of the job.
 * @property {int} Index - The Index property is an integer that represents the unique identifier of a
 * job. It is used to differentiate between different jobs in a system.
 * @property {int} Status - The "Status" property in the job struct represents the current status of
 * the job. It is an integer value that can be used to indicate different states/conditions of the job.
 * @property {int} pid - The "pid" property in the "job" struct represents the process ID of the job.
 */
typedef struct job {
    char *Name;
    int Index;
    int Status;
    int pid;
} job;


/* Declaring a variable called JobsNum of type int. */
int JobsNum;

/* Declaring an array of job structures called Jobs with a size of SIZE. */
job Jobs[SIZE];

// Store the foreground job (needed for Ctrl-C and Ctrl-Z)
job foregroundJob;

// For keeping track of foreground jobs
int FPID[SIZE];
int numForegroundProcesses;


// The function sets the text color to red.
void setTextColorRed() {
    printf("\033[1;31m");
}


// The function "resetTextColor" resets the text color to the default color.
void resetTextColor() {
    printf("\033[0m");
}

/**
 * The function `tokenizeInput` takes a string `s` and a delimiter `delimParameter`, and splits `s`
 * into tokens using the delimiter, storing the tokens in the `token` array and updating the
 * `totalTokens` variable with the total number of tokens.
 * 
 * @param token An array of strings where the tokens will be stored.
 * @param s The string to be tokenized.
 * @param delimParameter The delimParameter is a string that specifies the delimiter characters used to
 * separate the tokens in the input string 's'.
 * @param totalTokens A pointer to an integer variable that will store the total number of tokens found
 * in the input string.
 */
void tokenizeInput(char *token[], char *s, char *delimParameter,  int *totalTokens){
    int index = 0;  // declaring a variable `index` and initializing it to 0
    token[0] = strtok(s, delimParameter);
    while(token[index]!=NULL){
        token[++index] = strtok(NULL, delimParameter);
    }
    // Returns the total number of tokens
    *totalTokens = index;
}

/*
 * The function getCurrentDir retrieves the current working directory and stores it in the variable
 * currentDir.
 */
void getCurrentDir() {
    if(getcwd(currentDir, SIZE) == NULL) {
        perror("");
        exit(0);
    }
    return;
}

/**
 * The function "checkRedirection" checks if any of the arguments contain redirection symbols ("<",
 * ">", or ">>").
 * 
 * @param numArgs The parameter `numArgs` represents the number of arguments passed to the function.
 * @param arguments An array of strings representing the command line arguments passed to the program.
 * 
 * @return 1 if any of the arguments in the array "arguments" is "<", ">", or ">>". Otherwise, it is
 * returning 0.
 */
int checkRedirection(int numArgs, char *arguments[]) {
   for (int i = 0; i < numArgs; i++) {
       if (strcmp(arguments[i], "<") == 0)
           return 1;
       else if(strcmp(arguments[i], ">") == 0)
           return 1;
       else if(strcmp(arguments[i], ">>") == 0 )
           return 1;
   }
   return 0;
}

/**
 * The function `redirectionHandler` handles input and output redirection.
 * 
 * @param totalArgs The total number of arguments in the argList array.
 * @param argList An array of strings representing the command and its arguments. The first element
 * (argList[0]) is the command itself, and the rest of the elements are the arguments.
 * 
 * @return an integer value. The return value is 0 if the function executes successfully, and -1 if
 * there is an error.
 */
int redirectionHandler(int totalArgs, char *argList[]) {    
    // declaring two character arrays, `inputFile` and `outputFile`, each with a size of 10000 characters.
    char inputFile[10000], outputFile[10000];
    int inputPos = 0, outputPos = 0, appendPos = 0;
    int inputDup, outputDup;
    int pos = 100;
    int inputpipeFileDescriptors, outputpipeFileDescriptors;

    /* Iterating through an array of strings called `argList` and checking each string for specific.
    If a string is equal to "<", it sets the variable `inputPos` to the current index. If a string is
    equal to ">", it sets the variable `outputPos` to the current index. If a string is equal to ">>",
    it sets the variable `appendPos` to the current index. */
    for(int i = 0; i < totalArgs; i++) {
        if(strcmp(argList[i], "<") == 0) 
            inputPos = i;
        if(strcmp(argList[i], ">") == 0) 
            outputPos = i;
        if(strcmp(argList[i], ">>") == 0) 
            appendPos = i;
    }


    /* Checking if the input position is not 0. If it is not 0, it copies the input file name from
    the argument list into the inputFile variable. It then sets the pos variable to the input
    position and sets the argument at that position to NULL. */
    if(inputPos != 0) {
        strcpy(inputFile, argList[inputPos + 1]);
        pos = inputPos;
        argList[pos] = NULL;
        struct stat fileStatus;
        if(stat(inputFile, &fileStatus) < 0) {
            perror("Stat ");
            return -1;
        }

        inputDup = dup(STDIN_FILENO);
        if (inputDup < 0) {
            perror("Dup ");
            return -1;
        }

        inputpipeFileDescriptors = open(inputFile, O_RDONLY, FILE_PERMISSIONS);
        if (inputpipeFileDescriptors < 0) {
            printf("File doesn't exist!\n");
            return -1;
        }   
        if (dup2(inputpipeFileDescriptors, STDIN_FILENO) < 0) {
            perror("Dup2 ");
            return -1;
        }
    }
    
    if(outputPos != 0 || appendPos != 0) {
        if(outputPos + appendPos <= pos)
            pos = outputPos + appendPos;
        strcpy(outputFile, argList[outputPos + appendPos + 1]);
        argList[pos] = NULL;
        outputDup = dup(STDOUT_FILENO);
        if (outputDup < 0) {
            perror("Dup ");
            return -1;
        }

        if(outputPos != 0)
            outputpipeFileDescriptors = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
        else
            outputpipeFileDescriptors = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, FILE_PERMISSIONS);

        if (outputpipeFileDescriptors < 0) {
            printf("File doesn't exist!\n");
            return -1;
        }  

        if(dup2(outputpipeFileDescriptors, STDOUT_FILENO) < 0)
        {
            perror("Dup2 ");
            return -1;
        }
    }

    /* Creating a child process using the fork() system call. If the fork() call is successful
    the child process executes the command specified in the argList using the execvp() system
    call. If the execvp() call fails, an error message is printed and -1 is returned. */
    pid_t pidValue;
    pidValue = fork();
    if(pidValue < 0) {
        close(outputpipeFileDescriptors);
        perror("Fork ");
        return -1;
    }
    if(pidValue == 0) {
        if(execvp(argList[0], argList) < 0) {
            perror("Execvp ");
            return -1;
        }
    }
    else {
        wait(NULL);
        dup2(inputDup, STDIN_FILENO);
        dup2(outputDup, STDOUT_FILENO);
    }   
    return 0;
}

// Check for the presence of pipes in the arguments
/**
 * The function checks if there are any pipe characters ("|") in an array of arguments.
 * 
 * @param arguments An array of strings representing command line arguments. Each element in the array
 * is a separate argument.
 * @param argumentCount The parameter "argumentCount" is the number of elements in the "arguments"
 * array.
 * 
 * @return an integer value. If the function finds a pipe symbol "|" in the array of arguments, it
 * returns 1. Otherwise, it returns 0.
 */
int checkForPipes(char* arguments[], int argumentCount) {
	for (int i = 0; i < argumentCount; i++) {
       if (strcmp(arguments[i], "|") == 0)
          return 1;
  }
  return 0;
}

/**
 * The `piping` function takes a command and the number of arguments, tokenizes the command by pipes,
 * sets up file descriptors for input and output, forks child processes to execute each command, and
 * handles redirection if needed.
 * 
 * @param command The `command` parameter in the `piping` function is a string that represents the
 * entire command to be executed, including any arguments and pipes. It is used to split the command
 * into individual pipe commands.
 * @param argumentCount The parameter `argumentCount` represents the number of arguments passed to the
 * `piping` function. It is used to determine the size of the `commandArgument` array in the
 * `handleForegroundChild` function.
 */

void piping(char *command, int argumentCount) {  
    char* pipedCommands[100];
    int numPipes = 0;
    tokenizeInput(pipedCommands, command, "|", &numPipes);

    int pipeFileDescriptors[2];
    int originalInput = dup(STDIN_FILENO);
    int originalOutput = dup(STDOUT_FILENO);

    /* Implementing a shell command that handles piped commands. It takes a string of
    piped commands as input and executes them using the `execvp` function. */
    for (int i = 0; i  < numPipes; i++) {
        int lenOfEachPipeCommand = 0;
        char *singlePipeCommand[10000];
        tokenizeInput(singlePipeCommand, pipedCommands[i], " \t", &lenOfEachPipeCommand);
        /* Setting up a pipeline for inter-process communication. It is creating multiple pipes
        and redirecting the standard input and output of each process to the appropriate pipe file
        descriptors. This allows the processes to pass data between each other through the pipes. */
        if (i == 0) {
            pipe(pipeFileDescriptors);
            dup2(pipeFileDescriptors[1], STDOUT_FILENO); 
            close(pipeFileDescriptors[1]);
        }
        else if(i == numPipes - 1) {
            dup2(pipeFileDescriptors[0], STDIN_FILENO);
            dup2(originalOutput,1);
        }
        else {
            dup2(pipeFileDescriptors[0], STDIN_FILENO);
            pipe(pipeFileDescriptors);
            dup2(pipeFileDescriptors[1], STDOUT_FILENO); 
            close(pipeFileDescriptors[1]);
        }
        int pid_fork = fork();
        int status;
        if(pid_fork == 0) {
            if(checkRedirection(lenOfEachPipeCommand, singlePipeCommand) == 1) {
                // Handle redirection if needed
                redirectionHandler(lenOfEachPipeCommand, singlePipeCommand);
            }
            else {
                execvp(singlePipeCommand[0], singlePipeCommand);
            }
            exit(0);
        }
        else {
            waitpid(pid_fork, &status, WUNTRACED);
            dup2(originalInput, STDIN_FILENO);
            dup2(originalOutput, STDOUT_FILENO);
        }
    }
    return;
}

/**
 * The function `customComparator` compares the names of two job objects.
 * 
 * @param p A pointer to the first element to be compared.
 * @param q The parameter `q` is a pointer to a constant void type.
 * 
 * @return the result of the `strcmp` function, which is an integer representing the comparison result
 * of the two strings.
 */
int customComparator(const void *p, const void *q){
    return strcmp(((job*)p)->Name, ((job*)q)->Name);
}

/**
 * The function handles the execution of a foreground child process.
 * 
 * @param argumentCount The parameter `argumentCount` represents the number of arguments passed to the
 * function, including the name of the command itself.
 * @param commandArgument An array of strings representing the command and its arguments. The last
 * element of the array should be NULL to indicate the end of the arguments.
 */
int handleForegroundChild(int argumentCount, char *commandArgument[]) {
    setpgid(0, 0);
    commandArgument[argumentCount] = NULL;

    tcsetpgrp(STDIN_FILENO, getpgid(0));
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    int execvpResult = execvp(commandArgument[0], commandArgument);

    if(execvpResult < 0){
        printf("Invalid command!\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

/**
 * Handle the parent process in the foreground
 * 
 * @param pid The parameter `pid` is the process ID of the foreground parent process.
 * @param commandArgument commandArgument is an array of strings that represents the command and its
 * arguments. The first element (commandArgument[0]) is the command itself, and the following elements
 * (commandArgument[1], commandArgument[2], etc.) are the arguments for the command.
 * @param argumentCount The parameter `argumentCount` represents the number of arguments passed to the
 * `handleForegroundParent` function.
 */
void handleForegroundParent(int pid, char *commandArgument[], int argumentCount) {
    char foregroundCommand[SIZE];
    strcpy(foregroundCommand, "");
    strcat(foregroundCommand, commandArgument[0]);

    foregroundJob.pid = pid;
    foregroundJob.Name = malloc(strlen(commandArgument[0]) * sizeof(char) + 2);
    strcpy(foregroundJob.Name, commandArgument[0]);
    foregroundJob.Index = 0;

    int status;
    if (waitpid(pid, &status, WUNTRACED) < 0)
        printf("Invalid command");

    foregroundJob.pid = -1;

    tcsetpgrp(STDIN_FILENO, getpgid(0));

    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);

    /* Checking if a process has been stopped using the WIFSTOPPED macro. If the process
    has been stopped. */
    if(WIFSTOPPED(status)){   
        int len = strlen(foregroundCommand);
        for (int i = 1; i < argumentCount - 1; i++) {
            len += strlen(commandArgument[i]);
        }

        Jobs[JobsNum].pid = pid;
        Jobs[JobsNum].Name = malloc(len * sizeof(char));

        Jobs[JobsNum].Status = 1;
        Jobs[JobsNum].Index = JobsNum;

        strcpy(Jobs[JobsNum].Name, commandArgument[0]);
        for (int i = 1; i < argumentCount - 1; i++) {
            strcat(Jobs[JobsNum].Name, " ");
            strcat(Jobs[JobsNum].Name, arguments[i]);
        }

        JobsNum++;
        FPID[numForegroundProcesses] = pid;
        numForegroundProcesses++;

        printf("Process %s with process ID [%d] suspended\n", foregroundCommand, (int)pid );
        return;
    } else{
        /* Checking if the variable "status" is equal to 1. If it is, then it prints a message
        indicating that a process with the name "foregroundCommand" and process ID "pid" has
        exited. */
        if (status == 1)
            printf("Process %s with process ID [%d] exited \n", foregroundCommand, (int)pid );
    }
}

/**
 * The function executes a foreground process by forking a child process and handling the child and
 * parent processes accordingly.
 * 
 * @param argumentCount The parameter `argumentCount` represents the number of arguments passed to the
 * function. It is of type `int` and is used to determine the number of command line arguments passed
 * to the program.
 * @param commandArgument The `commandArgument` parameter is an array of strings that represents the
 * command and its arguments. Each element in the array is a separate argument passed to the command.
 */
void executeForegroundProcess(int argumentCount, char *commandArgument[]) {
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    int pid = fork();

    /* Checking if the process ID (pid) is equal to 0. If it is, it calls the function
    handleForegroundChild with the arguments argumentCount and commandArgument. If the pid is not
    equal to 0, it calls the function handleForegroundParent with the arguments pid, commandArgument,
    and argumentCount. After that, it frees the memory allocated for the foregroundJob.Name. */
    if (pid == 0) {
        handleForegroundChild(argumentCount, commandArgument);
    } else {
        handleForegroundParent(pid, commandArgument, argumentCount);
        free(foregroundJob.Name);
    }
}

/*
 * The function handleSIGCHLD is used to handle the SIGCHLD signal, which is sent when a child process
 * terminates, and updates the status of the terminated process in a Jobs array.
 */
void handleSIGCHLD() {
    pid_t pid;
    pid = waitpid(-1, NULL, WNOHANG);
    /* Checking if the value of `pid` is less than 0. If it is, the code returns. If not, it iterates
    through an array of `Jobs` and checks if the `pid` matches the `pid` of any job in the array. If a
    match is found and the `Status` of the job is not -1, the `Status` is set to -1 and a message is
    printed indicating that the job has been completed. */
    if (pid < 0) {
        return;
    } else {
        for(int i = 0; i < JobsNum; i++){
            if((int)pid == Jobs[i].pid) {
                if (Jobs[i].Status != -1) {
                    Jobs[i].Status = -1;
                    printf("Completed: [%d]   %d   %s \n",  JobsNum , Jobs[i].pid, Jobs[i].Name);
                    break;
                }
            }
        }
    }
    return;
}

/**
 * The function `executeBackgroundProcess` executes a given command in the background and stores
 * information about the job in a data structure.
 * 
 * @param argumentCount The parameter `argumentCount` represents the number of arguments passed to the
 * function, including the name of the program itself.
 * @param arguments The `arguments` parameter is an array of strings, where each string represents a
 * command-line argument. The `argumentCount` parameter is an integer that specifies the number of
 * arguments in the `arguments` array.
 */
void executeBackgroundProcess(int argumentCount, char *arguments[]) {
    int pid = fork();

    /* Checking if the process ID (pid) is equal to 0. If it is, it sets the process group ID to the 
    current process ID using the setpgrp() function. */
    if(pid == 0) {
        setpgrp();
        arguments[argumentCount - 1] = NULL; 
        execvp(arguments[0], arguments); 
        exit(EXIT_SUCCESS);
    } else {
        /* Calculating the total length of the strings in the `arguments` array and storing it in the
        variable `len`. It then assigns the process ID (`pid`) to the `pid` field of the 'Jobs[JobsNum]'
        struct. */
        int len = strlen(arguments[0]);
        for (int i = 1; i < argumentCount - 1; i++) {
            len += strlen(arguments[i]);
        }
        
        /* Assigning a value to the "pid" member of the "Jobs" array at index "JobsNum". It is also 
        allocating memory for the "Name" member of the "Jobs" array at index "JobsNum" and copying the 
        string value from the "arguments" array at index 0 into the "Name" member. */
        Jobs[JobsNum].pid = pid;
        Jobs[JobsNum].Name = malloc(len * sizeof(char) + 2);
        strcpy(Jobs[JobsNum].Name, arguments[0]);

        /* Concatenating multiple strings together. It starts by initializing a for loop that iterates 
        from 1  to `argumentCount - 1`. Inside the loop, it uses the `strcat` function to concatenate the 
        string `" "` and the string `arguments[i]` to the `Jobs[JobsNum].Name` string. This process is 
        repeated for each iteration of the loop, effectively appending each `arguments[i]` string to the
        `Jobs[JobsNum].Name` string. */
        for (int i = 1; i < argumentCount - 1; i++) {
            strcat(Jobs[JobsNum].Name, " ");
            strcat(Jobs[JobsNum].Name, arguments[i]);
        }
        /* Setting the status of a job to 1, assigning an index to the job, incrementing the number of 
        jobs, and printing a message indicating that a background job has started. */
        Jobs[JobsNum].Status = 1;
        Jobs[JobsNum].Index = JobsNum;
        JobsNum++;
        fprintf(stderr, "Background job started: [%d] %d %s \n",JobsNum, Jobs[JobsNum - 1].pid, Jobs[JobsNum - 1].Name);
        return;
    }
}

/**
 * The function "jobs" sorts an array of structs and prints the index, status, process ID, and name of
 * each job that is not marked as completed.
 * 
 * @param argumentCount The parameter `argumentCount` represents the number of command-line arguments
 * passed to the program, including the name of the program itself.
 * @param arguments The "arguments" parameter is an array of strings (char pointers) that represents
 * the command-line arguments passed to the "jobs" function. The "argumentCount" parameter is an
 * integer that represents the number of command-line arguments passed to the function.
 */
void jobs(int argumentCount, char *arguments[]) {
    // Sort the array of structs
    qsort(Jobs, JobsNum, sizeof(Jobs[0]), customComparator);
    /* Iterating through an array of jobs. It checks if the status of a job is -1, and if so, it 
    continues to the next iteration. If the status is not -1, it prints the index of the job, 
    followed by "Running", the process ID, and the name of the job. */
    for(int i = 0; i < JobsNum; i++){
        if (Jobs[i].Status == -1)
            continue;

        printf("[%d] ",Jobs[i].Index + 1);
        printf("Running ");
        printf("%d %s\n", Jobs[i].pid,  Jobs[i].Name);
    }
} 

/**
 * The function `cd` changes the current directory based on the given command argument, which can be a
 * path or an environment variable.
 * 
 * @param numArguments The `numArguments` parameter represents the number of arguments passed to the
 * `cd` function.
 * @param commandArgument The `commandArgument` parameter is a string that represents the argument
 * passed to the `cd` function. It is the directory path that the user wants to change to.
 */
void cd(int numArguments, char *commandArgument) {

    int needsExpansion = 0;
    /* Checking if the variable `numArguments` is equal to 1 and returning if it is. */
    if (numArguments == 1)
        return;

    /* Checking if the string `commandArgument` contains the character '$'. If it does, the
    variable `needsExpansion` is set to 1. */
    if(strchr(commandArgument, '$') != NULL) {
        needsExpansion = 1;
    }
    /* Checking if a variable called "needsExpansion" is true. If it is true, the code retrieves
    the value of an environment variable specified in the "commandArgument" string. It then
    attempts to change the current directory to the path specified by the environment variable. If
    the directory change is successful, it calls the "getCurrentDir()" function. If any errors
    occur during the process, an error message is printed using the "perror()" function. */
    if (needsExpansion) {
        char* path = getenv(strtok(strtok(commandArgument, "="), "$"));
        if (path == NULL) {
            perror("cd ");
            return;
        }
        if (chdir(path) < 0) {
            perror("cd ");
            return;
        }    
        getCurrentDir();
    }
    /* It checks if the "cd" command is followed by a valid directory name as an argument. If the
    directory exists, it changes the current working directory to the specified directory using the
    chdir() function. If the directory does not exist or there is an error in changing the directory,
    it prints an error message using perror() and returns. Finally, it calls the getCurrentDir() 
    function to display the current working directory. */
    else {
        if (chdir(commandArgument) < 0) {
            perror("cd ");
            return;
        }

        getCurrentDir();
    }
    return;
}

/**
 * The function "export" takes a command argument, tokenizes it, checks if the value needs expansion,
 * and sets the environment variable accordingly.
 * 
 * @param commandArgument The `commandArgument` parameter is a string that represents the argument
 * passed to the `export` function.
 */
void export(char *commandArgument) {
	int numCommands = 0;
    char* envVar[2];
	tokenizeInput(envVar, commandArgument, "=", &numCommands);
    char* env = envVar[0];
    char* val = envVar[1];
    int needsExpansion = 0;

    /* Checking if the string `val` contains the character '$'. If it does, the variable 
    `needsExpansion` is set to 1. */
    if(strchr(val, '$') != NULL) {
        needsExpansion = 1;
    }

    /* Checking if the variable `needsExpansion` is equal to 1. If it is, then it retrieves the 
    value of an environment variable specified by `envVar[1]`, expands any variables within the 
    value, and sets the value of the environment variable specified by `env` to the expanded value. 
    If the value of the environment variable is not found or if there is an error setting the 
    environment variable, an error message is printed. */
    if(needsExpansion == 1) {
        char* value = getenv(strtok(strtok(envVar[1], "$"), "$"));
        if (value == NULL) {
            perror("export ");
            return;
        }
        if (setenv(env, value, 1) < 0) {
            perror("export ");
            return;        
    }
    /* Setting an environment variable with the name specified by the variable "env" and the value 
    specified by the variable "val". The third argument "1" indicates that the variable should be 
    overwritten if it already exists. */
    else {
        setenv(env, val, 1);
    }
    }
}

/**
 * The function `ls` uses the `fork` and `execlp` system calls to execute the `ls` command with 
 * different options based on the number of arguments passed to it.
 * 
 * @param numArguments The number of arguments passed to the function, including the command itself.
 * @param commandArgument The `commandArgument` parameter is a string that represents the argument
 * passed to the `ls` function. It can be either "-a", "-l", or NULL.
 */
void ls(int numArguments, char *commandArgument) {
    /* Checks the number of command line arguments passed to it. If there are exactly 2 arguments, 
    it checks the value of the second argument (commandArgument). */
    if (numArguments == 2) {
        if (strcmp (commandArgument, "-a") == 0 ) {
            int pid = fork();
            /* The code is using the `execlp` function to execute the `ls` command with the `-a`
            flag in the child process. If the `pid` variable is 0, indicating that it is the child
            process, it will execute the `ls` command. If the `pid` variable is not 0, indicating
            that it is the parent process, it will wait for the child process to finish executing
            before continuing. */
            if (pid == 0){
                execlp("/bin/ls", "ls", "-a", NULL);
            }
            else {
                wait(NULL);
            }
        }
        /* Checking if the command argument is "-l". If it is, it creates a child process using the 
        fork() function. In the child process, it uses the execlp() function to execute the "ls -l" 
        command, which lists the files and directories in the current directory in long format. The 
        parent process waits for the child process to finish executing before continuing. */
        else if (strcmp(commandArgument, "-l") == 0 ) {
            int pid = fork();
            if (pid == 0){
                execlp("/bin/ls", "ls", "-l" , NULL);
            } else {
                wait(NULL);
            }
        }
    }
    /* Checks if the variable `numArguments` is equal to 1. If it is, it creates a child process using 
    the `fork()` function. In the child process, it uses the `execlp()` function to execute the `ls` 
    command from the `/bin/ls` directory. In the parent process, it waits for the child process to 
    finish executing using the `wait()` function. */
    if (numArguments == 1) {
        int pid = fork();
        if (pid == 0){
            execlp("/bin/ls", "ls", NULL);
        } else {
            wait(NULL);
        }
    }
}

/**
 * The function "echo" prints the command line arguments, expanding environment variables if necessary.
 * 
 * @param numArguments The parameter `numArguments` represents the number of arguments passed to the
 * `echo` function.
 * @param commandArgument The `commandArgument` parameter is an array of strings that represents the
 * arguments passed to the `echo` function. Each element in the array is a command line argument.
 */
void echo (int numArguments, char *commandArgument[]){
    if (numArguments == 1)
        return;

    char * fileStatus;
    int needsExpansion = 0;

    /* Checking if the string in `commandArgument[1]` contains the character '$'. If it does, the
    variable `needsExpansion` is set to 1. */
    if (strchr(commandArgument[1], '$') != NULL) {
        needsExpansion = 1;
    }    
    /* Checking if a variable called `needsExpansion` is true. If it is true, the code checks if the 
    string in `commandArgument[1]` contains a forward slash ("/"). If it does, the code uses `strtok` 
    to extract the part of the string before the slash and stores it in `fileStatus`. Then, it uses 
    `strtok` again to extract the part of the string after the slash and stores it in `fileStatus`. */
    if (needsExpansion) {
        if (strchr(commandArgument[1], '/') != NULL) {
            fileStatus = strtok(commandArgument[1], " /");
            fileStatus = strtok(NULL, " /");
            printf("%s/%s\n", getenv(strtok(strtok(commandArgument[1], "$"), "$")), fileStatus);
        } else {
            printf("%s\n", getenv(strtok(strtok(commandArgument[1], "$"), "$")));
        }
        return; }
    /* Checking if the number of arguments is greater than 1. If it is, then it loops through each 
    argument and checks if it contains the '#' character. If it does, it returns from the function. 
    Otherwise, it prints each argument after removing any single or double quotes using the strtok 
    function. Finally, it prints a new line character and returns. */
    else {
        if (numArguments > 1) {
            for (int i = 1; i < numArguments; i++) {
                if (strchr(commandArgument[i], '#')) {
                    return;
                }
                printf("%s ", strtok(strtok(commandArgument[i], "\'\""), "\'\""));
            }
        }
        printf("\n");
        return;
    }
}    

/**
 * The function `cmdHandler` handles different commands entered by the user, including background
 * processes, piping, redirection, built-in commands (cd, pwd, echo, jobs, ls, exit, quit, export,
 * kill), and executing foreground processes.
 * 
 * @return void, so it is not returning any value.
 */
void cmdHandler() {   
    int numCommands = 0;
    tokenizeInput(commandList, inputBuffer, "\n", &numCommands);

    for (int i = 0; i < numCommands; i++) {
        char tempStr[10000];
        strcpy(tempStr, commandList[i]);
        int argumentCount = 0;
        tokenizeInput(arguments, commandList[i], " \t", &argumentCount);

    /* Checking if the last argument in the "arguments" array is "&" using the strcmp function. If 
    it is "&", it calls the executeBackgroundProcess function with the argumentCount and arguments as 
    parameters. This suggests that the code is checking if the user wants to execute the process in 
    the background. */
    if(strcmp(arguments[argumentCount - 1],"&") == 0){
	    executeBackgroundProcess(argumentCount, arguments);
            return; 
    }
    /**
     * The above function checks for various commands such as cd, pwd, echo, jobs, ls, exit, quit,
     * export, comments, and kill, and executes the corresponding actions.
     * 
     * @param  - `arguments`: an array of strings, where each string represents a command-line argument
     */
    else if(checkForPipes(arguments, argumentCount) == 1){ 
        piping(tempStr, argumentCount);
    } else {
        // Check redirection
        if (checkRedirection(argumentCount, arguments) == 1) {
            redirectionHandler(argumentCount, arguments);
        } else {
		    // If the list is empty then simply return
            if (argumentCount == 0 || arguments[0] == NULL){
                return;
            }

            // Check for cd.
            else if(strcmp(arguments[0], "cd") == 0) {
                cd(argumentCount, arguments[1]);
            }

            // Check for pwd
            else if(strcmp(arguments[0], "pwd") == 0) {
                char myPwd[SIZE];
                if(getcwd(myPwd, SIZE) == NULL) {
                    perror("");
                    exit(0);
                }
                printf("%s\n", myPwd);
                return;
            }

            // Check for echo
            else if(strcmp(arguments[0], "echo") == 0) {
                echo (argumentCount, arguments);
                printf("\n");
            }

		     // Check for jobs
            else if(strcmp(arguments[0], "jobs") == 0) {
                jobs(argumentCount, arguments);
            }

            // Check for ls
            else if(strcmp(arguments[0], "ls") == 0) {
                int fileStatus = argumentCount;
                if (strchr(arguments[argumentCount - 1], '#'))
                    fileStatus--;
                ls(fileStatus, arguments[1]);
            }

            // Check for exit
            else if(strcmp(arguments[0], "exit") == 0) {
                exit(0);
            }
		
            // Check for quit
            else if (strcmp(arguments[0], "quit") == 0){
                exit(0);
            }
		  
            // Check for export 
            else if(strcmp(arguments[0], "export") == 0) {
                export(arguments[1]);
            }
            // Check for comments 
            else if( (strcmp(arguments[0], "#") == 0) || (strchr(arguments[0], '#'))) {
                return;
            }
            // Check for kill 
            else if(strcmp(arguments[0], "kill") == 0) {
                kill(atoi(arguments[2]), atoi(arguments[1]));
                return;
            } else {
                executeForegroundProcess(argumentCount, arguments);
            }
        }
    }
        // check if any child process terminated
        signal(SIGCHLD, handleSIGCHLD);
    }
    return;
}

/*
The function reads input from the user and clears the terminal screen if the input is "clear".
*/
void getinputBuffer(){
    inputBuffer = (char *)malloc(SIZE);
    fgets(inputBuffer, SIZE, stdin);
    if (strcmp(inputBuffer, "clear") == 0) {
        printf("\033[H\033[J");  // clears the current screen in the terminal
    }
}

/*
The function "print" prints the current directory in red color with the prompt "[QUASH]$  ".
*/
void print(){
    getCurrentDir();
    setTextColorRed();  // makes text red
    printf("[QUASH]$   ");
    resetTextColor();  // resets text color
}


/**
 * The main function of the Quash program, which initializes variables, sets up signal handling, gets
 * user input, and handles commands.
 * 
 * @return The main function is returning 0.
 */
int main(){
/* print the message "Welcome to Quash...." followed
by two new lines. */
	printf("Welcome to Quash.... \n \n"); // print "Welcome to Quash...." followed by two new lines.

    JobsNum = 0;
    numForegroundProcesses = 0;

    /* An infinite loop that continuously prompts the user for input and handles the input commands. It 
    sets up a signal handler for the SIGCHLD signal, which is used to handle child processes. It then 
    calls functions to get the current directory, print the prompt in red, get the input buffer, handle 
    the input commands, and free the input buffer. */
    while (1){
        signal(SIGCHLD, handleSIGCHLD);

        getCurrentDir();  // Gets current directory
        print();  // Calls print function to print prompt in red
        getinputBuffer();
        cmdHandler();   
        free(inputBuffer);
   }
   return(0);
}


