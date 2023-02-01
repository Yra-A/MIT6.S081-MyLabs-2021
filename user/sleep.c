#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(2, "sleep: missing parameter\n");
        exit(1);
    }

    int n = atoi(argv[1]);
    if (sleep(n) < 0) {
        fprintf(2, "sleep: sleep error\n");
        exit(1);
    }
    exit(0);
}