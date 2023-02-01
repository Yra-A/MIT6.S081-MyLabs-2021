#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void fmtpath(char *path) {
    char *p;
    for (p = path + strlen(path); p >= path && *p != '/'; p--);
    *(++p) = 0;
}

void find(char *path, char *fn) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 读取当前路径下的所有数据
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        if (de.inum == 0) {
            continue;
        }
        memcpy(p, de.name, DIRSIZ); // 将名字加在 buf 后面
        p[DIRSIZ] = 0;
    
        if (stat(buf, &st) < 0) { 
            fprintf(2, "ls: cannot stat %s\n", buf);
        }

        switch(st.type) {
            case T_FILE: // 如果是文件
                if (strcmp(p, fn) == 0) { // 文件名 和 fn 相同则输出
                    fprintf(1, "%s\n", buf);
                }
                break;

            case T_DIR: // 如果是目录
                if (strcmp(p, ".") != 0 && strcmp(p, "..") != 0) { // 且不是 . 和 ..
                    find(buf, fn); // 递归查询这个目录
                }
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(2, "find: miss parameter\n");
        exit(1);
    }

    char *path = argv[1]; // path 
    char *fn = argv[2]; // file name
    find(path, fn);

    exit(0);
}