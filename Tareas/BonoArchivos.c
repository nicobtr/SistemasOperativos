#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	
	long long max = 10000000LL;
	double suma_positiva = 0.0;
	double suma_negativa = 0.0;

	pid_t pid = fork();
	
	if(pid<0) {
	
		perror("Error al crear el proceso hijo: ");
		exit(1);
		
	} else if (pid == 0) {
	
		for(long long i = 1; i < max; i+=4) {
		
			suma_positiva += 1.0/ (double)i;
		}

		FILE *dato = fopen("dato.bin", "wb");
		if(dato == NULL) {
			
			perror("Error en fopen()");	
			exit(1);
		}
		
		if(fwrite(&suma_positiva, sizeof(suma_positiva), 1, dato) != 1) {

			perror("Error en fwrite()");
			fclose(dato);
			exit(1);
		 } 
		 
  		 fclose(dato);
		 exit(0);

	} else {
		
		wait(NULL);
		double sum_pos = 0.0;
		for(long long i = 3; i < max; i+=4) {
		
			suma_negativa += 1.0 / (double)i;		

		}

		FILE *dato = fopen("dato.bin", "rb");
		if(dato == NULL) {

			perror("Error en fopen()");
			exit(1);

		}
		if(fread(&sum_pos, sizeof(sum_pos), 1, dato) != 1) {
			
			perror("Error en fread()");
			fclose(dato);
			exit(1);			
	
		}
		
		double pi_aproximado = 4.0*(sum_pos - suma_negativa);
		printf("pi = %.12f\n", pi_aproximado);
		fclose(dato);
		exit(0);
	}
	

}
