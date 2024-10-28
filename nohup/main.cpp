#include <iostream>
#include <stdio.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void parentTask();
void childTask(const char *path);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Use: " << argv[0] << " <path to program>" << std::endl;
        return -1;
    }

    pid_t child_pid;
    if ((child_pid = fork()) == -1) {
        std::cerr << "Failed create subtask" << std::endl;
        return -1;
    }

    if (child_pid == 0) {
        childTask(argv[1]);
    } else {
        exit(EXIT_SUCCESS);
    }

    return 0;
}

void childTask(const char *path) {
    pid_t detached_pid;
    if ((detached_pid = setsid()) < 0) {
        std::cerr << "Failed detach " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Detached pid: " << detached_pid << std::endl; 

    int devNull = open("/dev/null", O_RDWR);
    if (devNull == -1) {
        std::cerr << "Failed open /dev/null" << std::endl;
        exit(EXIT_FAILURE);
    }

    dup2(devNull, STDOUT_FILENO);
    dup2(devNull, STDERR_FILENO);
    dup2(devNull, STDIN_FILENO);
    close(devNull);

    if (execlp(path, path, NULL) == -1) {
        std::cerr << "Failed to start " << path << std::endl;
        exit(EXIT_FAILURE);
    }
}