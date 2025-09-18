#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int n1, n2;
    unsigned long long fact1 = 1, fact2 = 1;

    printf("Ingrese primer número: ");
    scanf("%d", &n1);
    printf("Ingrese segundo número: ");
    scanf("%d", &n2);

    pid_t pid = fork();

    if (pid < 0) {
        printf("Error en el fork()\n");
        exit(1);
    } 
    else if (pid == 0) { // hijo
        for (int i = 1; i <= n1; i++) {
            fact1 *= i;
        }
        printf("Hijo: factorial de %d = %llu\n", n1, fact1);
        exit(0);
    } 
    else { // padre
        for (int i = 1; i <= n2; i++) {
            fact2 *= i;
        }
        printf("Padre: factorial de %d = %llu\n", n2, fact2);
        exit(0);
    }

    return 0;
}
