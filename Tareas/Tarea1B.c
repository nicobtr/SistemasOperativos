#include <stdio.h>
#include <stdlib.h>

struct Persona {

	char nombre[32], apellido[32];
	int edad;
	float estatura;

};

int main() {

	struct Persona p1, p2;
	
	printf("Ingrese nombre: ");
	scanf("%s", p1.nombre);
	printf("Ingrese apellido: ");
	scanf("%s", p1.apellido);
	printf("Ingrese edad: ");
	scanf("%d", &p1.edad);
	printf("Ingrese estatura (cm): ");
	scanf("%f", &p1.estatura);

	FILE *datos = fopen("DatosPersona.bin", "wb");
	if(datos == NULL) {

		printf("\nError al abrir/crear el archivo");
		return 1;

	}

	fwrite(&p1, sizeof(struct Persona), 1, datos);
	fclose(datos);


	datos = fopen("DatosPersona.bin", "rb");
	if(datos == NULL) {
	
		printf("\nError al abrir el archivo");
		return 1;

	}

	fread(&p2, sizeof(struct Persona), 1, datos);
	fclose(datos);

	printf("\nNombre: %s\n", p2.nombre);
	printf("Apellido: %s\n", p2.apellido);
	printf("Edad: %d\n", p2.edad);
	printf("Estatura: %.2f\n", p2.estatura);
		

	return 0;
}
