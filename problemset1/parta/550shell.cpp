#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>

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

        // create pipes
        std::vector<struct pipe> pipes;
        for (int i = 0; i < n_execs; i++) {
            struct pipe p;
            if (pipe(p.pipe_fd) == -1) {
                perror("cannot create pipe");
            }
            pipes.push_back(p);
        }

        for (int i = 0; i < n_execs; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("cannot fork");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                // handle file descriptors

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
                int status;
                waitpid(pid, &status, 0);
            }
        }

        delete[] input;
    }

    return 0;
}
