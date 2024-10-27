#include <iostream>
#include <stdio.h> 
#include <unistd.h> 
#include <sys/wait.h>
#include <filesystem>
#include <fstream>
#include <vector>

int stdout_pipe[2];
int stderr_pipe[2];

pid_t child_pid;

void parentTask();
void childTask(const char *path);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Use: " << argv[0] << " <path to program>" << std::endl;
        return -1;
    }

    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        std::cerr << "Failed create pipe" << std::endl;
        return -1;
    }

    if ((child_pid = fork()) == -1) {
        std::cerr << "Failed create subtask" << std::endl;
        return -1;
    }

    if (child_pid == 0) {
        childTask(argv[1]);
    }
    else {
        parentTask();
    }

    return 0;
}

int writeToFile(const std::vector<std::string> &lines, const std::string &fileName) {
    if (!std::filesystem::exists(fileName)) {
        std::ofstream newFile(fileName);
        if (!newFile) {
            std::cerr << "Error creating file: " << fileName << std::endl;
            return -1;
        }
        newFile.close();
    }

    std::ofstream file(fileName, std::ios::app | std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing: " << fileName << std::endl;
        return -1;
    }

    for (auto &line : lines) {
        file.write(line.c_str(), line.size());
    }

    file.close();
    return 0;
}

void parentTask() {
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    int status;
    waitpid(child_pid, &status, 0);

    if (!WIFEXITED(status)) {
        std::cerr << "Child process exited with code " << WEXITSTATUS(status) << std::endl;
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        exit(EXIT_FAILURE);
    }

    char buff[1024];
    ssize_t count;

    std::vector<std::string> outLines;
    while ((count = read(stdout_pipe[0], buff, sizeof(buff))) > 0) {
        std::string line(buff, count);
        outLines.push_back(line);
    }

    std::vector<std::string> errLines;
    while ((count = read(stderr_pipe[0], buff, sizeof(buff))) > 0) {
        std::string line(buff, count);
        errLines.push_back(line);
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (writeToFile(outLines, "stdouts") == -1) {
        std::cerr << "Failed write stdouts" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (writeToFile(errLines, "stderrs") == -1) {
        std::cerr << "Failed write stderrs" << std::endl;
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void childTask(const char *path) {
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    if (execlp(path, path, NULL) == -1) {
        std::cerr << "Failed to start " << path << std::endl;
        exit(EXIT_FAILURE);
    }
}