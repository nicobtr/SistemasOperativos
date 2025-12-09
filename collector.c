#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_HOSTS 10
#define BUFFER_SIZE 1024
#define PORT_DEFAULT 8888
#define UPDATE_INTERVAL 2

// Estructura para almacenar información de cada host
struct host_info {
    char ip[32];
    float cpu_usage;
    float cpu_user;
    float cpu_system;
    float cpu_idle;
    float mem_used_mb;
    float mem_free_mb;
    float swap_total_mb;
    float swap_free_mb;
    time_t last_update;
    int has_cpu_data;
    int has_mem_data;
};

// Variables globales compartidas
struct host_info hosts[MAX_HOSTS];
pthread_mutex_t hosts_mutex = PTHREAD_MUTEX_INITIALIZER;

// Buscar o crear entrada para un host
int find_or_create_host(const char *ip) {
    int empty_slot = -1;
    
    for (int i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].ip[0] == '\0' && empty_slot == -1) {
            empty_slot = i;
        }
        if (strcmp(hosts[i].ip, ip) == 0) {
            return i;
        }
    }
    
    if (empty_slot != -1) {
        strncpy(hosts[empty_slot].ip, ip, sizeof(hosts[empty_slot].ip) - 1);
        hosts[empty_slot].ip[sizeof(hosts[empty_slot].ip) - 1] = '\0';
        hosts[empty_slot].last_update = time(NULL);
        return empty_slot;
    }
    
    return -1;
}

// Actualizar datos de memoria
void update_mem_data(const char *ip, float mem_used, float mem_free,
                     float swap_total, float swap_free) {
    pthread_mutex_lock(&hosts_mutex);
    
    int idx = find_or_create_host(ip);
    if (idx >= 0) {
        hosts[idx].mem_used_mb = mem_used;
        hosts[idx].mem_free_mb = mem_free;
        hosts[idx].swap_total_mb = swap_total;
        hosts[idx].swap_free_mb = swap_free;
        hosts[idx].has_mem_data = 1;
        hosts[idx].last_update = time(NULL);
    }
    
    pthread_mutex_unlock(&hosts_mutex);
}

// Actualizar datos de CPU
void update_cpu_data(const char *ip, float cpu_usage, float cpu_user,
                     float cpu_system, float cpu_idle) {
    pthread_mutex_lock(&hosts_mutex);
    
    int idx = find_or_create_host(ip);
    if (idx >= 0) {
        hosts[idx].cpu_usage = cpu_usage;
        hosts[idx].cpu_user = cpu_user;
        hosts[idx].cpu_system = cpu_system;
        hosts[idx].cpu_idle = cpu_idle;
        hosts[idx].has_cpu_data = 1;
        hosts[idx].last_update = time(NULL);
    }
    
    pthread_mutex_unlock(&hosts_mutex);
}

// Procesar mensaje recibido
void process_message(const char *message) {
    char type[10], ip[32];
    float values[5];
    
    // Parsear mensaje
    int parsed = sscanf(message, "%[^;];%[^;];%f;%f;%f;%f",
                       type, ip, &values[0], &values[1],
                       &values[2], &values[3]);
    
    if (parsed >= 3) {
        if (strcmp(type, "MEM") == 0) {
            if (parsed >= 6) {
                update_mem_data(ip, values[0], values[1], values[2], values[3]);
            }
        } else if (strcmp(type, "CPU") == 0) {
            if (parsed >= 6) {
                update_cpu_data(ip, values[0], values[1], values[2], values[3]);
            }
        }
    }
}

// Función para limpiar pantalla
void clear_screen() {
    printf("\033[2J\033[H");
}

// Mostrar tabla de hosts
void display_table() {
    clear_screen();
    printf("=========================================================================================================\n");
    printf("MONITOR DISTRIBUIDO DE CPU Y MEMORIA\n");
    printf("=========================================================================================================\n");
    printf("%-15s %-8s %-10s %-10s %-10s %-15s %-15s %-10s\n",
           "IP", "CPU%", "USER%", "SYS%", "IDLE%",
           "MEM_USED(MB)", "MEM_FREE(MB)", "STATUS");
    printf("---------------------------------------------------------------------------------------------------------\n");
    
    pthread_mutex_lock(&hosts_mutex);
    
    time_t now = time(NULL);
    for (int i = 0; i < MAX_HOSTS; i++) {
        if (hosts[i].ip[0] != '\0') {
            // Verificar si hay datos recientes (últimos 10 segundos)
            if (now - hosts[i].last_update > 10) {
                printf("%-15s %-8s %-10s %-10s %-10s %-15s %-15s %-10s\n",
                       hosts[i].ip, "--", "--", "--", "--", "--", "--", "OFFLINE");
                continue;
            }
            
            // Mostrar datos disponibles
            if (hosts[i].has_cpu_data && hosts[i].has_mem_data) {
                printf("%-15s %-8.1f %-10.1f %-10.1f %-10.1f %-15.1f %-15.1f %-10s\n",
                       hosts[i].ip,
                       hosts[i].cpu_usage,
                       hosts[i].cpu_user,
                       hosts[i].cpu_system,
                       hosts[i].cpu_idle,
                       hosts[i].mem_used_mb,
                       hosts[i].mem_free_mb,
                       "ONLINE");
            } else if (hosts[i].has_cpu_data) {
                printf("%-15s %-8.1f %-10.1f %-10.1f %-10.1f %-15s %-15s %-10s\n",
                       hosts[i].ip,
                       hosts[i].cpu_usage,
                       hosts[i].cpu_user,
                       hosts[i].cpu_system,
                       hosts[i].cpu_idle,
                       "--", "--", "CPU_ONLY");
            } else if (hosts[i].has_mem_data) {
                printf("%-15s %-8s %-10s %-10s %-10s %-15.1f %-15.1f %-10s\n",
                       hosts[i].ip,
                       "--", "--", "--", "--",
                       hosts[i].mem_used_mb,
                       hosts[i].mem_free_mb,
                       "MEM_ONLY");
            }
        }
    }
    
    pthread_mutex_unlock(&hosts_mutex);
    
    printf("=========================================================================================================\n");
    printf("Actualizado: %s", ctime(&now));
    printf("Esperando conexiones de agentes...\n");
}

// Hilo para mostrar la tabla periódicamente
void* display_thread(void* arg) {
    (void)arg; // Marcar parámetro como no usado para evitar warning
    
    while (1) {
        display_table();
        sleep(UPDATE_INTERVAL);
    }
    return NULL;
}

// Manejar conexión de cliente
void* handle_client(void* arg) {
    int client_sock = *((int*)arg);
    free(arg);
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Procesar cada línea del mensaje
        char *line = strtok(buffer, "\n");
        while (line != NULL) {
            process_message(line);
            line = strtok(NULL, "\n");
        }
    }
    
    close(client_sock);
    return NULL;
}

// Inicializar servidor
int start_server(int port) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error creando socket servidor");
        return -1;
    }
    
    // Configurar opciones del socket
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Error en setsockopt");
        close(server_sock);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_sock);
        return -1;
    }
    
    if (listen(server_sock, 10) < 0) {
        perror("Error en listen");
        close(server_sock);
        return -1;
    }
    
    printf("Servidor iniciado en puerto %d\n", port);
    return server_sock;
}

int main(int argc, char *argv[]) {
    int port = PORT_DEFAULT;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Inicializar estructura de hosts
    memset(hosts, 0, sizeof(hosts));
    
    // Iniciar servidor
    int server_sock = start_server(port);
    if (server_sock < 0) {
        return 1;
    }
    
    // Crear hilo para mostrar la tabla
    pthread_t display_tid;
    if (pthread_create(&display_tid, NULL, display_thread, NULL) != 0) {
        perror("Error creando hilo de visualización");
        close(server_sock);
        return 1;
    }
    
    // Aceptar conexiones
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (*client_sock < 0) {
            perror("Error aceptando conexión");
            free(client_sock);
            continue;
        }
        
        printf("Nueva conexión desde %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        // Crear hilo para manejar cliente
        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, handle_client, client_sock) != 0) {
            perror("Error creando hilo para cliente");
            close(*client_sock);
            free(client_sock);
            continue;
        }
        
        pthread_detach(client_tid);
    }
    
    close(server_sock);
    return 0;
}
