#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int pipe_fds[2]; // 管道读写口 0写 1读
    if (pipe(pipe_fds) < 0) {
        fprintf(2, "pipe: error\n");
        exit(1);
    }
    int pid = fork();
    if (pid == 0) { // child process
        char *ch = "";
        read(pipe_fds[0], ch, 1);
        fprintf(1, "%d: received ping\n", pid);
        exit(0);
    } else { // parent process
        char *ch = "a";
        write(pipe_fds[1], ch, 1); 
        wait(0);
        fprintf(1, "%d: received pong\n", pid);
    }
    exit(0);
}