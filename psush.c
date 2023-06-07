#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "psush.h"

char *history[maxhistory];
int historycount = 0;
pid_t running = 0;
unsigned short is_verbose = 0;

void handle_signal(int signum) {
	if (signum == SIGINT) {
		if (running > 0) {
			printf("child killed\n");
			kill(running, SIGINT);
		}
	}
}

void execmd(char *cmd) {
	char *arg[maxarg];
	int argcount = 0;

	char *pipecmd[maxpipe];
	int pipecount = 0;
	int fd[maxpipe - 1][2];
	pid_t pid;
	char *token;

	int input = 0;
	int output = 0;
	char *ifile = NULL;
	char *ofile = NULL;

	token = strtok(cmd, "|");
	while (token != NULL && pipecount < maxpipe) {
		pipecmd[pipecount] = token;
		pipecount++;
		token = strtok(NULL, "|");	
	}



	for (int i = 0; i < pipecount; i++) {
		argcount = 0;
		token = strtok(pipecmd[i], " ");
		while (token != NULL && argcount < maxarg - 1) {
			arg[argcount] = token;
			argcount++;
			token = strtok(NULL, " ");
		}
		arg[argcount] = NULL;

		for(int j = 0; j < argcount; j++) {
			if(strcmp(arg[j], "<") == 0) {
				input = 1;
				ifile = arg[j + 1];
				arg[j] = NULL;
				//arg[j + 1] = NULL;
			} else if(strcmp(arg[j], ">") == 0) {
				output = 1;
				ofile = arg[j + 1];
				arg[j] = NULL;
			}
		}


		if (i < pipecount - 1) {
			pipe(fd[i]);
		}

		pid = fork();
		if (pid == 0) {
			signal(SIGINT, SIG_DFL);
			
			//connect to read end
			if( i != 0) {
				dup2(fd[i - 1][0], STDIN_FILENO);
				close(fd[i - 1][0]);
			}	

			//connect to write end
			if(i != pipecount - 1) {
				dup2(fd[i][1], STDOUT_FILENO);
				close(fd[i][1]);
			}
			
			//close
			for (int j = 0; j < pipecount - 1; j++) {
				close(fd[j][0]);
				close(fd[j][1]);
			}
	
			if(input) {
				int infile = open(ifile, O_RDONLY);
				dup2(infile, STDIN_FILENO);
				close(infile);
			}

			if(output) {
				int outfile = open(ofile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				dup2(outfile, STDOUT_FILENO);
				close(outfile);
			}
	
			//Execute
			if (strcmp(arg[0], "bye") == 0){
				kill(getpid(), SIGINT);
				exit(0);
			} else if(strcmp(arg[0], "cd") == 0) {
				if(argcount > 1) {
					chdir(arg[1]);
				} else {
					chdir(getenv("HOME"));
				}
			} else if(strcmp(arg[0], "cwd") == 0) {
				char cwd[1024];
				getcwd(cwd, sizeof(cwd));
				printf("cwd: %s\n", cwd);
			} else if(strcmp(arg[0], "history") == 0) {
				for(int j = 0; j < historycount; j++) {
					printf("%d %s\n", j + 1, history[j]);
				}
			} else if(strcmp(arg[0], "echo") == 0) {
				for(int j = 1; j < argcount; j++) {
					printf("%s ", arg[j]);
				}
				printf("\n");
			} else {
				execvp(arg[0], arg);
				printf("%s: Command not found\n", arg[0]);
				exit(EXIT_FAILURE);
			}
		} else if (pid > 0){
			running = pid;

			if(i != 0) {
				close(fd[i - 1][0]);
			}
			if(i != pipecount - 1) {
				close(fd[i][1]);
			}
		} else {
			perror("fork");
			exit(EXIT_FAILURE);
		}
	}
	
	for(int i = 0; i < pipecount - 1; i++) {
		close(fd[i][0]);
		close(fd[i][1]);
	}

	for(int i = 0; i < pipecount; i++) {
		wait(NULL);
	}

	running = 0;
}

void options(int argc, char *argv[]) {
	int opt;

	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
			case 'h':
				fprintf(stdout, "Something helpful\n\n");
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				is_verbose++;
				if (is_verbose) {
					fprintf(stderr, "verbose: verbose option selected: %d\n", is_verbose);
				}
				break;
			case '?':
				fprintf(stderr, "*** Unknown option used, ignoreing. ***\n");
				break;
			default:
				fprintf(stderr, "*** Oops, somethign strange happened <%c> ... ignoring ...***\n", opt);
		}
	}
}


int main(int argc, char *argv[])
{
	char cmd[maxcmdlen];
	char prompt[promptlen] = {'\0'};
	char cwd[maxcmdlen];
	char hostname[maxcmdlen];

	signal(SIGINT, handle_signal);
	options(argc, argv);
	
	if(gethostname(hostname, sizeof(hostname)) == -1) {
		return EXIT_FAILURE;
	}

	while(1) {
		if(isatty(STDOUT_FILENO)) {
			sprintf(prompt, " %s %s \n%s@%s# ",
			PROMPT_STR,
			getcwd(cwd, sizeof(cwd)),
			getenv("LOGNAME"),
			hostname
			);
		fputs(prompt, stdout);
		}
		if(fgets(cmd, sizeof(cmd), stdin) == NULL) {
			break;
		}
		cmd[strcspn(cmd, "\n")] = '\0';

		if(historycount < maxhistory) {
			history[historycount] = strdup(cmd);
			historycount++;
		} else {
			free(history[0]);
			memmove(history, history + 1, (maxhistory - 1) * sizeof(char *));
			history[maxhistory - 1] = strdup(cmd);
		}
		execmd(cmd);
		if(strcmp(cmd, "bye") == 0) {
			break;
		}
	}

	//Free history
	for(int i = 0; i < historycount; i++) {
		free(history[i]);
	}
	return EXIT_SUCCESS;
}
