#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define INTERVAL 2

// Estructura para almacenar información de memoria
struct mem_info {
    long mem_total;
    long mem_available;
    long mem_free;
    long swap_total;
    long swap_free;
};

// Función para leer información de /proc/meminfo
int read_meminfo(struct mem_info *info) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        perror("Error abriendo /proc/meminfo");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "MemTotal:")) {
            sscanf(line, "MemTotal: %ld kB", &info->mem_total);
        } else if (strstr(line, "MemAvailable:")) {
            sscanf(line, "MemAvailable: %ld kB", &info->mem_available);
        } else if (strstr(line, "MemFree:")) {
            sscanf(line, "MemFree: %ld kB", &info->mem_free);
        } else if (strstr(line, "SwapTotal:")) {
            sscanf(line, "SwapTotal: %ld kB", &info->swap_total);
        } else if (strstr(line, "SwapFree:")) {
            sscanf(line, "SwapFree: %ld kB", &info->swap_free);
        }
    }

    fclose(f);
    return 0;
}

// Función para conectar al servidor
int connect_to_server(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Error en dirección IP");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error conectando al servidor");
        close(sock);
        return -1;
    }

    printf("Conectado al recolector en %s:%d\n", ip, port);
    return sock;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <ip_recolector> <puerto> <ip_logica_agente>\n", argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *agent_ip = argv[3];

    // Conectar al servidor
    int sock = connect_to_server(server_ip, port);
    if (sock < 0) {
        return 1;
    }

    printf("Agente de memoria iniciado para IP: %s\n", agent_ip);

    while (1) {
        struct mem_info info;
        memset(&info, 0, sizeof(info));

        // Leer información de memoria
        if (read_meminfo(&info) < 0) {
            sleep(INTERVAL);
            continue;
        }

        // Calcular métricas
        float mem_used_mb = (info.mem_total - info.mem_available) / 1024.0;
        float mem_free_mb = info.mem_free / 1024.0;
        float swap_total_mb = info.swap_total / 1024.0;
        float swap_free_mb = info.swap_free / 1024.0;

        // Formatear mensaje
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message),
                "MEM;%s;%.2f;%.2f;%.2f;%.2f\n",
                agent_ip, mem_used_mb, mem_free_mb,
                swap_total_mb, swap_free_mb);

        // Enviar datos al servidor
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Error enviando datos");
            // Intentar reconectar
            close(sock);
            sock = connect_to_server(server_ip, port);
            if (sock < 0) {
                sleep(5);
                continue;
            }
        }

        printf("Enviado: %s", message);
        sleep(INTERVAL);
    }

    close(sock);
    return 0;
}
