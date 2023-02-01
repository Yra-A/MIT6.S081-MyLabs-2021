#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

#define ARGLEN 50

int main(int argc, char *argv[]) {

    char *cmd = argv[1];
    int N = argc - 1;
    char *para[MAXARG]; // 所有参数
    for (int i = 0; i < N; i++) {
        para[i] = argv[i + 1];
    }
    char **init_para = para + N; // init_para 指向右边指令参数的末尾
    char arg[MAXARG * ARGLEN]; // 内存池，存放标准输入读取进来的参数
    char *p = arg; // 用来在 arg 中存储参数，参数之间用 '\0' 分割
    char ch;
    int cnt = 0;
    char **lp = init_para; // lp 用来移动并存储标准输入当前一行的参数
    while (read(0, &ch, 1) != 0) {
        if (ch == ' ' || ch == '\n') {
            *p = 0; // 一个参数或者一行读完，0 分割
            *lp++ = arg + ARGLEN * cnt; // lp 指向 arg 中当前所有参数存放的末尾
            cnt++; // 参数个数加一
            p = arg + ARGLEN * cnt; // p 指向了下一个参数的开头存放的位置
            
            if (ch == '\n') { // 如果是换行符，则要执行 exec 了
                *lp = 0; // para 以 0 为结尾作为参数结束的标志
                if (fork() == 0) { //创建子进程执行 exec
                    exec(cmd, para);
                    exit(0);
                } else {
                    wait(0);
                    lp = init_para; // lp 回到 init_para，重新读取下一行的所有参数
                }
            }
        } else {
            *p++ = ch;
        }
    }
    exit(0);
}