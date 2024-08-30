/*******************************************************************************
 * Name        : minishell.c
 * Author      : Jacob Roessler and Kevin Ha
 * Date        : 4/8/2021
 * Description : C based implementation of the shell.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pwd.h>

/**************************************************************
 * TODO
 * 1. Confirm ^C
 * 2. On the doc, when you do sleep 10 in ^C, should you newline?
 * 3. nocommand on the doc kicks you out backwards one dir?
 * 4. I changed strlen to add 1, STRLEN DOES NOT INCLUDE \0!!!
**************************************************************/
#define BRIGHTBLUE "\x1b[34;1m" 
#define DEFAULT    "\x1b[0m"

volatile sig_atomic_t signal_val = false;

void catch_signal(int sig) {
    signal_val = true;
}

int wordCount (char *input){
    char in[strlen(input) + 1];
    strcpy(in, input);
    int wordCount = 0;
    char *token = strtok(in, " ");
    while( token != NULL) {
        wordCount++;
        token = strtok(NULL, " ");
    }
    return wordCount;
}

int get_dir() {
    char cwd[PATH_MAX];

    if(getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("[" BRIGHTBLUE "%s" DEFAULT "]$ ", cwd);
        fflush(stdout);
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }
}

int main(int argc, char **argv) {
    struct sigaction action;
    struct passwd *pwd;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = catch_signal;
    //action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action, NULL) == -1) {
        fprintf(stderr, "Cannot register signal handler. %s.\n", strerror(errno));
        return EXIT_FAILURE;
    }

       
    while (1) {
        //printf("while\n");
        if(!signal_val) {
            get_dir();
            int bytes;
            char buf[PATH_MAX];
            bytes = read(STDIN_FILENO, buf, sizeof(buf));
            
            //If SIGINT is encountered or no input is given, reprompt.
            if(signal_val == true ) {
                printf("\n");
                continue;
            }
            if(bytes < 0) {
                if(errno == EINTR) {
                    continue;
                }

                fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
                continue;
            }
            if(bytes == 0) {
                printf("\n");
                continue;
            }

            if(buf[0] == '\n') {
                continue;
            }

            buf[bytes - 1] = '\0';

            int size = wordCount(buf);
            char *arguments[size];
            //Strcpy is dangerous, I changed something here. +1.
            char input[strlen(buf) + 1];
            strcpy(input, buf);

            char *token = strtok(input, " ");
            arguments[0] = token;
            int i = 1;
            while( token != NULL) {
                token = strtok(NULL, " ");
                arguments[i] = token;
                i++;
            }
            //The very weird cause of </dev/urandom causing a segfault.
            if(arguments[0] == NULL) {
                continue;
            }

            //printf("%s", buf);
            //printf("%d", strcmp(buf, "exit"));
            //Exit command.
            if(strcmp(arguments[0], "exit") == 0) {
                //printf("what\n");
                exit(EXIT_SUCCESS);
            }
            //append literally null to end of the pseudo-argv array. *argv has null in the end*
            if(strcmp(arguments[0], "cd") == 0) {
               if( (pwd = getpwuid(getuid()) ) == NULL) {
                   fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                   return EXIT_FAILURE;
               }
               else {
                    //printf("Args: %s.\n", arguments[1]);
                    if(size > 2){
                        fprintf(stderr, "Error: Too many arguments to cd.\n");
                    }
                    else if(size == 1 || (strcmp(arguments[1], "~") == 0)) {
                        chdir(pwd->pw_dir); 
                    }
                    else if (chdir(arguments[1]) == -1){
                        fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", arguments[1] 
                                ,strerror(errno));
                    }
                
                    continue;
                }
                   
                   //printf("%s\n", pwd->pw_name);
               
            }

            char *execArgs[size+1];
            for(int i = 0; i < size; i++){
                execArgs[i] = arguments[i];
            }
            execArgs[size] = NULL;
            
            pid_t pid = fork();
            if( pid < 0){
                fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
                
            }
            if( pid == 0 ) {
                if ( execvp(arguments[0], execArgs) == -1){
                    fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                    exit(EXIT_FAILURE);
                }
            }
            if(pid > 0) {
                while(wait(NULL) > 0);
            }
     
        }
        else {
            signal_val = false;
        }
    }

    return EXIT_SUCCESS;
}