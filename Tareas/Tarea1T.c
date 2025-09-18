#include <stdio.h>
#include <stdlib.h>

struct Persona {

	char *nombre, *apellido;
	int edad;
	float estatura;

};


int main() {

	struct Persona p1;
	p1.nombre = malloc(32);
	p1.apellido = malloc(32);

	printf("Ingrese nombre: ");
	scanf("%s", p1.nombre);
	printf("Ingrese apellido: ");
        scanf("%s", p1.apellido);
	printf("Ingrese edad: ");
	scanf("%d", &p1.edad);
	printf("Ingrese estatura (cm): ");
	scanf("%f", &p1.estatura);

	FILE *datos = fopen("DatosPersona.txt", "w");
	
	if(datos == NULL) {
		
		printf("\nError al abrir/crear el archivo");
		return 0;
		
	}

	fprintf(datos, "Nombre: %s\nApellido: %s\n", p1.nombre, p1.apellido);
	fprintf(datos, "Edad: %d\nEstatura: %.2f\n", p1.edad, p1.estatura);

	fclose(datos);

	datos = fopen("DatosPersona.txt", "r");
        
        if(datos == NULL) {
                
                printf("\nError al abrir el archivo");
                return 0;
                
        }
	
	printf("\n");
	char *linea = malloc(100);
	while(fgets(linea, 100, datos) != NULL) {

		printf("%s", linea);
	}

	fclose(datos);
	
	free(p1.nombre);
	free(p1.apellido);
	free(linea);

	return 0;
}
