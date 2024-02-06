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
void exec_cmd(int argc, char **argv);
void update_status(int exit_status);
void replace_status(char **tokens, int n_tokens);

int last_status = 0; // storing the last exit status so i can update the variable

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
	    } else if (strcmp(tokens[0], "echo") == 0) {
		replace_status(tokens, n_tokens);
		exec_cmd(n_tokens, tokens);
	    } else {
		exec_cmd(n_tokens, tokens);
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
            return 1; // exit status if i call cd 1 2, wrong no. of arguments
        }

        const char *dir = (argc == 1) ? getenv("HOME") : argv[1];

        if (chdir(dir) == -1) {
           fprintf(stderr, "cd: %s\n", strerror(errno));
           return 1; // exit status if i call cd /not-real, aka fake dir 
        }

        return 0; // a successful cd

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

void exec_cmd(int argc, char **argv) {
    pid_t pid = fork();

    if (pid == -1) {
	perror("fork");
	exit(EXIT_FAILURE);
    } else if (pid == 0) { // the child process
	// re-enabling ctrl C according to part 1 instructions
	signal(SIGINT, SIG_DFL);

	// checking for redirection
	int input_fd = STDIN_FILENO;
	int output_fd = STDOUT_FILENO;
	int i;
	for (i = 0; i < argc, i++) {
	    if (strcmp(argv[i], "<") == 0) { // input redirection
		input_fd = open(argv[i + 1], O_RDONLY); 
		if (input_fd == -1) {
		    perror("open");
		    exit(EXIT_FAILURE);
		}
		argv[i] = NULL; // removes the < from argv
		break;
	    } else if (strcmp(argv[i], ">") == 0 { // output redirection
	        output_fd = open(argv[i + 1], O_CREAT | O_WRONLY | O_TRUNC, 0777);
		// notes for self:
		// file is created if it doesn't exist O_CREAT
		// file is opened for writing if it does O_WRONLY
		// if it has content it will be truncated first O_TRUNC
		// 0777 is the permissions, readable, writable, and executable by everyone.
		if (output_fd == -1) {
		    perror("open");
		    exit(EXIT_FAILURE);
		}
		argv[i] = NULL; // removes the > from argv
		break;
	    }
	}

	// redirect stdin
	if (input_fd != STDIN_FILENO) {
		if (dup2(input_fd, STDIN_FILENO) == -1) {
		    perror("dup2");
		    exit(EXIT_FAILURE);
		}
		close(input_fd); // close the descriptor
	}

	// same logic but for stdout
	if (output_fd != STDOUT_FILENO) {
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(output_fd); // Close unnecessary file descriptor
        }

	
	if (execvp(argv[0], argv) == -1) {
	    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
	    exit(EXIT_FAILURE);
	}
    } else { // parent process
	int status;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status)) {
	    int exit_status = WEXITSTATUS(status);
	    update_status(exit_status);
	} else {
	    fprintf(stderr, "Child process terminated abnormally\n");
	    update_status(EXIT_FAILURE);
	}
    }
}


void update_status(int exit_status) {
    last_status = exit_status;
}


void replace_status(char **tokens, int n_tokens) {
    char exit_status_str[16];
    sprintf(exit_status_str, "%d", last_status);

    for (int i = 0; i < n_tokens; i++) {
	if (strcmp(tokens[i], "$?") == 0) {
	    tokens[i] = exit_status_str;
	}
    }
}
