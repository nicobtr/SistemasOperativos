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

// Estructura para almacenar estadísticas de CPU
struct cpu_stats {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
};

// Leer estadísticas de CPU desde /proc/stat
int read_cpu_stats(struct cpu_stats *stats) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        perror("Error abriendo /proc/stat");
        return -1;
    }

    char line[256];
    if (fgets(line, sizeof(line), f)) {
        sscanf(line, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
               &stats->user, &stats->nice, &stats->system,
               &stats->idle, &stats->iowait, &stats->irq,
               &stats->softirq, &stats->steal);
    }

    fclose(f);
    return 0;
}

// Calcular porcentajes de CPU
void calculate_cpu_percentages(struct cpu_stats *prev, struct cpu_stats *curr,
                              float *usage, float *user_pct,
                              float *system_pct, float *idle_pct) {
    unsigned long prev_total = prev->user + prev->nice + prev->system +
                               prev->idle + prev->iowait + prev->irq +
                               prev->softirq + prev->steal;
    
    unsigned long curr_total = curr->user + curr->nice + curr->system +
                               curr->idle + curr->iowait + curr->irq +
                               curr->softirq + curr->steal;
    
    unsigned long total_diff = curr_total - prev_total;
    
    if (total_diff == 0) {
        *usage = *user_pct = *system_pct = *idle_pct = 0.0;
        return;
    }
    
    // Calcular diferencias
    unsigned long user_diff = (curr->user - prev->user) +
                             (curr->nice - prev->nice);
    unsigned long system_diff = (curr->system - prev->system) +
                               (curr->irq - prev->irq) +
                               (curr->softirq - prev->softirq);
    unsigned long idle_diff = (curr->idle - prev->idle) +
                             (curr->iowait - prev->iowait);
    
    // Calcular porcentajes
    *user_pct = 100.0 * user_diff / total_diff;
    *system_pct = 100.0 * system_diff / total_diff;
    *idle_pct = 100.0 * idle_diff / total_diff;
    *usage = 100.0 - *idle_pct;
}

// Función para conectar al servidor (igual que en agent_mem)
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

    printf("Agente de CPU iniciado para IP: %s\n", agent_ip);

    struct cpu_stats prev_stats, curr_stats;
    memset(&prev_stats, 0, sizeof(prev_stats));
    
    // Leer estadísticas iniciales
    if (read_cpu_stats(&prev_stats) < 0) {
        close(sock);
        return 1;
    }

    sleep(1); // Pequeña pausa para primera medición

    while (1) {
        // Leer estadísticas actuales
        if (read_cpu_stats(&curr_stats) < 0) {
            sleep(INTERVAL);
            continue;
        }

        // Calcular porcentajes
        float cpu_usage, user_pct, system_pct, idle_pct;
        calculate_cpu_percentages(&prev_stats, &curr_stats,
                                 &cpu_usage, &user_pct,
                                 &system_pct, &idle_pct);

        // Formatear mensaje
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message),
                "CPU;%s;%.2f;%.2f;%.2f;%.2f\n",
                agent_ip, cpu_usage, user_pct, system_pct, idle_pct);

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
        
        // Actualizar estadísticas previas
        memcpy(&prev_stats, &curr_stats, sizeof(struct cpu_stats));
        
        sleep(INTERVAL);
    }

    close(sock);
    return 0;
}
