/*
 * file:        shell3650.c
 * description: skeleton code for a simple shell
 *
 * Peter Desnoyers, Northeastern Fall 2023
 */

/* <> means don't check the local directory */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/* "" means check the local directory */
#include "parser.h"

/* you'll need these includes later: */
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

int cd_cmd(int argc, char **argv);
int pwd_cmd(int argc, char **argv);
int exit_cmd(int argc, char **argv);


int main(int argc, char **argv)
{
    bool interactive = isatty(STDIN_FILENO); /* see: man 3 isatty */
    FILE *fp = stdin;

    if (argc == 2) {
        interactive = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL) {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE); /* see: man 3 exit */
        }
    }
    if (argc > 2) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, SIG_IGN); /* ignore SIGINT=^C */

    char line[1024], linebuf[1024];
    const int max_tokens = 32;
    char *tokens[max_tokens];
    int status = 0;

    /* loop:
     *   if interactive: print prompt
     *   read line, break if end of file
     *   tokenize it
     *   print it out <-- your logic goes here
     */
    while (true) {
        if (interactive) {
            /* print prompt. flush stdout, since normally the tty driver doesn't
             * do this until it sees '\n'
             */
            printf("sh3650> ");
            fflush(stdout);
        }

        /* see: man 3 fgets (fgets returns NULL on end of file)
         */
        if (!fgets(line, sizeof(line), fp))
            break;

        /* read a line, tokenize it, and print it out
         */
        int n_tokens = parse(line, max_tokens, tokens, linebuf, sizeof(linebuf));

        /* replace the code below with your shell:
         */
	if (n_tokens > 0) {
	    if (strcmp(tokens[0], "cd") ==0) {
		status = cd_cmd(n_tokens, tokens);
	    } else if (strcmp(tokens[0], "pwd") == 0) {
		status = pwd_cmd(n_tokens, tokens);
	    } else if (strcmp(tokens[0], "exit") == 0) {
		status = exit_cmd(n_tokens, tokens);
	    } else {
       		printf("line:");
       		for (int i = 0; i < n_tokens; i++)
            	    printf(" '%s'", tokens[i]);
        	printf("\n");
	    }
    	}
    }

    if (interactive)            /* make things pretty */
        printf("\n");           /* try deleting this and then quit with ^D */

    return status;
}


int cd_cmd(int argc, char **argv) {
        if (argc > 2) {
            fprintf(stderr, "cd: wrong number of arguments\n");
            return 1;
        }

        const char *dir = (argc == 1) ? getenv("HOME") : argv[1];

        if (chdir(dir) == -1) {
           fprintf(stderr, "cd: %s\n", strerror(errno));
           return 1;
        }

        return 0;

    }


int pwd_cmd(int argc, char **argv) {
        if (argc > 2) {
            fprintf(stderr, "pwd: too many arguments\n");
            return 1;
        }

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("pwd");
            return 1;
        }

        printf("%s\n", cwd);
        return 0;
    }


int exit_cmd(int argc, char **argv) {
        if (argc > 2) {
            fprintf(stderr, "exit: too many arguments\n");
            return 1;
        }


        if (argc == 1) {
            exit(0);
        } else {
            int status = atoi(argv[1]);
            exit(status);
        }
    }
