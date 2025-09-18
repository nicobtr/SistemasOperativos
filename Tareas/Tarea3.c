#include <stdio.h>   // printf
#include <stdlib.h>  // exit
#include <unistd.h>  // fork, pipe

int main() {

    long long max = 10000000LL;
    double suma_positiva = 0.0;
    double suma_negativa = 0.0;

    int pipefd[2];
    int r = pipe(pipefd);

    if (r < 0) {
        perror("Error en pipe()");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork()");
        exit(1);
    } else if (pid == 0) { // hijo
        close(pipefd[0]); // cierra descriptor de lectura

        for (long long i = 1; i < max; i += 4) {
            suma_positiva += 1.0 / (double)i;
        }

        r = write(pipefd[1], &suma_positiva, sizeof(suma_positiva));
        if (r != sizeof(suma_positiva)) {
            perror("Error en write()");
            close(pipefd[1]);
            exit(1);
        }

        close(pipefd[1]);
        exit(0);

    } else { // padre
        close(pipefd[1]); // cierra descriptor de escritura

        double sum_pos = 0.0;

        for (long long i = 3; i < max; i += 4) {
            suma_negativa += 1.0 / (double)i;
        }

        r = read(pipefd[0], &sum_pos, sizeof(sum_pos));
        if (r != sizeof(sum_pos)) {
            perror("Error en read()");
            close(pipefd[0]);
            exit(1);
        }

        double pi_aproximado = 4.0 * (sum_pos - suma_negativa);
        printf("pi = %.12f\n", pi_aproximado);

        close(pipefd[0]);
        exit(0);
    }
}
