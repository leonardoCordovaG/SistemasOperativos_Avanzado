#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int total = getreadcount();
    printf("Total de llamadas a read() ejecutadas hasta ahora: %d\n", total);
    exit(0);
}