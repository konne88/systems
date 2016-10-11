#include <stdio.h>
#include <string.h>
#include <unidstd.h>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::vector<std::string>> parse_cmd(char *cmd) {
    std::vector<std::vector<std::string>> result;
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

    return result;
}

int main(int argc, char *argv[]) {
    std::string input_str;
    char *input;
    while (std::getline(std::cin, input_str, '\n')) {
        input = new char[input_str.length()+1];
        strcpy(input, input_str.c_str());

        std::vector<std::vector<std::string>> execs = parse_cmd(input);
        int n_execs = execs.size();


        // std::vector<pid_t> children;
        // std::vector<pipe_fd> pipes;

        // for (size_t i = 0; i+1 < cmds.size(); ++i) {
        //   pipe_fd p;
        //   pipes.push_back(p);
        // }

        // for (size_t i = 0; i < cmds.size(); ++i) {
        //     pid_t pid = fork();
        //     if (pid == 0) {
        //         if (i != 0) {
        //         close(pipes[i-1].fd_out);
        //         dup2(pipes[i-1].fd_in, STDIN_FILENO);
        //     }
        //     if (i + 1 != cmds.size()) {
        //         close(pipes[i].fd_in);
        //         dup2(pipes[i].fd_out, STDOUT_FILENO);        
        //     }        
        // }
        
        // for (size_t i = 0; i < children.size(); ++i) {
        //     int status;
        //     waitpid(children[i], &status, 0);
        // }

        delete[] input;
    }

    return 0;
}
