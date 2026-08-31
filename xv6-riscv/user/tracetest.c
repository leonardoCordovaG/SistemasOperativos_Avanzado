#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int i;
    char *nargv[MAXARG];

    if (argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')) {
        fprintf(2, "Uso: %s mascara comando [args...]\n", argv[0]);
        exit(1);
    }

    if (trace(atoi(argv[1])) < 0) {
        fprintf(2, "%s: trace falló\n", argv[0]);
        exit(1);
    }

    // Arma el nuevo argv, saltando argv[0] (trace) y argv[1] (la máscara)
    for (i = 2; i < argc && i < MAXARG; i++) {
        nargv[i - 2] = argv[i];
    }
    nargv[i - 2] = 0;

    exec(nargv[0], nargv);

    // Si exec regresa, algo falló
    fprintf(2, "%s: exec %s falló\n", argv[0], nargv[0]);
    exit(1);

}