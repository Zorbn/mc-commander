#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

#define MAX_MSG_LEN 1024
#define MAX_INPUT_LEN 1024
#define MAX_TOKENS 100
#define MAX_NAME_LEN 17 // 16 characters (defined by Minecraft) + \0

// Check if any input is available on stdin.
bool hasInput() {
	int stdinNum = 0;
	
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 1;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(stdinNum, &fdset);

	return select(1, &fdset, NULL, NULL, &timeout) == 1;
}

int main(void) {
	char stdinBuffer[MAX_INPUT_LEN];
	char inBuffer[MAX_MSG_LEN];
	char parseBuffer[MAX_MSG_LEN];

	chdir("../");

	int childPipes[2];
	int parentPipes[2];
	if (pipe(childPipes) < 0 || pipe(parentPipes) < 0) {
		puts("Failed to create pipes.");
		exit(-1);
	}

	pid_t pid = fork();

	if (pid == -1) {
		puts("Failed to fork.");
		exit(-1);
	} else if (pid == 0) {
		// This is the child.
		if (dup2(childPipes[1], STDOUT_FILENO) == -1) {
			puts("Failed to redirect stdout.");
			exit(-1);
		}

		if (dup2(parentPipes[0], STDIN_FILENO) == -1) {
			puts("Failed to redirect stdin.");
			exit(-1);
		}

		close(parentPipes[0]);
		close(parentPipes[1]);
		close(childPipes[0]);
		close(childPipes[1]);

		printf("Server is running on pid: %d\n", getpid());
		fflush(stdout);
		execlp("./run.sh", "./run.sh", NULL);
	} else {
		// This is the parent.
		close(parentPipes[0]);
		close(childPipes[1]);

		int pipeWrite = parentPipes[1];
		int pipeRead = childPipes[0];

		if (fcntl(pipeRead, F_SETFL, O_NONBLOCK) < 0) {
			printf("Failed to set reading pipe to non-blocking.");
			exit(-1);
		}

		puts("Created the server process");

		char *delimiter = " ";
		char *tokens[MAX_TOKENS];
		char playerName[MAX_NAME_LEN];

		while (true) {
			if (hasInput()) {
				fgets(stdinBuffer, MAX_INPUT_LEN, stdin);
				write(pipeWrite, stdinBuffer, strlen(stdinBuffer));
			}

			memset(inBuffer, 0, MAX_MSG_LEN);

			if (read(pipeRead, inBuffer, MAX_MSG_LEN) > 0) {
				strncpy(parseBuffer, inBuffer, MAX_MSG_LEN);

				char *token = strtok(parseBuffer, delimiter);
				int tokenI = 0;
				while (token != NULL && tokenI < MAX_TOKENS) {
					printf("%s ", token);
					token = strtok(NULL, delimiter);
					tokens[tokenI] = token;

					if (tokenI == 3 && token[0] == '!') {
						// Token is the start of a command.
						// Fetch the player's name.
						int nameTokenLen = strlen(tokens[2]);
						strncpy(playerName, &tokens[2][1], nameTokenLen - 2);

						if (strncmp(&token[1], "help", 4) == 0) {
							char *msg = "say You asked for help, but that command is currently unavailable, sorry!\n";
							write(pipeWrite, msg, strlen(msg));
						}
					}

					++tokenI;
				}
			}
		}

		puts("Goodbye!");
	}
}
