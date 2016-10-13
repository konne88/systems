#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>

#define STDIN_FD  0
#define STDOUT_FD 1

struct pipe {
    int pipe_fd[2];
};

void parse_cmd(char *cmd, std::vector<std::vector<std::string>> &result) {
    char *token, *arg;

    token = strtok(cmd, "|");
    std::vector<char*> execs;
    while(token != NULL) {
        execs.push_back(token);
        token = strtok(NULL, "|");
    }
    
    int n_execs = execs.size();
    for (int i = 0; i < n_execs; i++) {
        arg = strtok(execs[i], " \t");
        std::vector<std::string> cmd_tokens;
        while(arg != NULL) {
            cmd_tokens.push_back(std::string(arg));
            arg = strtok(NULL, " \t");
        }
        result.push_back(cmd_tokens);
    }
}

int main(int argc, char *argv[]) {
    std::string input_str;
    char *input;
    while (std::getline(std::cin, input_str, '\n')) {
        input = new char[input_str.length()+1];
        strcpy(input, input_str.c_str());

        std::vector<std::vector<std::string>> execs;
        parse_cmd(input, execs);
        int n_execs = execs.size();

        struct pipe p;
        int in_fd = STDIN_FD;

        for (int i = 0; i < n_execs; i++) {
            // create pipe
            if (pipe(p.pipe_fd) == -1) {
                perror("cannot create pipe");
            }
            // fork child process
            pid_t pid = fork();
            if (pid == -1) {
                perror("cannot fork");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                // handle file descriptors
                close(p.pipe_fd[0]);
                dup2(in_fd, STDIN_FD);
                close(in_fd);

                if (i < n_execs - 1) {
                    dup2(p.pipe_fd[1], STDOUT_FD);
                }
                close(p.pipe_fd[1]);

                // execute command
                std::vector<char*> cmd;
                int n_cmd = execs[i].size();
                for (int j = 0; j < n_cmd; j++) {
                    cmd.push_back(&execs[i][j][0]);
                }
                cmd.push_back(0);
                if (execvp(cmd[0], &cmd[0]) == -1) {
                    perror("execution error");
                }
            } else {
                close(p.pipe_fd[1]);
                if (in_fd != STDIN_FD)
                    close(in_fd);
                in_fd = p.pipe_fd[0];
                int status;
                waitpid(pid, &status, 0);
            }
        }

        delete[] input;
    }

    return 0;
}
