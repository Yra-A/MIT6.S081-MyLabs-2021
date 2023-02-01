#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int cnt = 0;

void work(int pl[])  {
    close(pl[1]); // 无需写入
    int n;
    if (read(pl[0], &n, sizeof(n)) == 0) { // 如果写口关闭了，read 将会返回 0，即最后一个数字读完后要退出进程
        exit(0);
    }
    fprintf(1, "prime %d\n", n); // 第一个未被筛去的数字，一定是质数

    int pr[2]; // 右管道，用来传递当前未被筛去的数组
    pipe(pr);

    if (fork() == 0) { // 子进程递归进行下一轮质数筛
        work(pr);
    } else {
        close(pr[0]); // 父进程无需读入
        int x = 0;
        while (read(pl[0], &x, sizeof(x)) != 0) { //从 pl 读取剩余传递来的数字
            if (x % n != 0) { // 如果未被筛去则进入下一轮筛
                write(pr[1], &x, sizeof(x));
            }
        }
        close(pr[1]); // 一定要关闭写入符，否则读第一个质数不会变成 0，那么会一直递归下去了。
        wait(0); // 父进程等待
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    
    int pipe_fds[2];
    pipe(pipe_fds);

    if (fork() == 0) {
        close(pipe_fds[1]); // 子进程无需写入
        work(pipe_fds);
    } else {
        close(pipe_fds[0]); // 父进程无需读入
        for (int i = 2; i <= 35; i++) {
            write(pipe_fds[1], &i, sizeof(i)); // 传递数字
        }
        close(pipe_fds[1]); // 一定要关闭写入符，否则读第一个质数不会变成 0，那么会一直递归下去了。
        wait(0); // 一定要等待
    }
    exit(0);
}