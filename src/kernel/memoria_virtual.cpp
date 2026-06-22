#include "../../include/kernel/memoria_virtual.hpp"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

namespace Industrial {

    void* inicializar_memoria_virtual() {
        // 1. Abrir/Crear el objeto de memoria compartida POSIX
        int shm_fd = shm_open("/shm_venalum", O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("Error en shm_open");
            return nullptr;
        }

        // 2. Configurar el tamaño físico en la RAM
        if (ftruncate(shm_fd, sizeof(MemoriaCompartidaPlanta)) == -1) {
            perror("Error en ftruncate");
            close(shm_fd);
            return nullptr;
        }

        // 3. Mapear el objeto en el espacio de memoria virtual del proceso
        void* ptr = mmap(NULL, sizeof(MemoriaCompartidaPlanta), 
                         PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        if (ptr == MAP_FAILED) {
            perror("Error en mmap");
            close(shm_fd);
            return MAP_FAILED;
        }

        // Una vez mapeado, el descriptor de archivo ya no es necesario
        close(shm_fd);

        // 4. Inicialización por software de los recursos industriales
        MemoriaCompartidaPlanta* shm_planta = static_cast<MemoriaCompartidaPlanta*>(ptr);
        shm_planta->silo_alumina = 50000.0f;
        for (int i = 0; i < 5; ++i) {
            shm_planta->tolvas_celdas[i] = 400.0f;
        }
        shm_planta->anodos_producidos = 0;
        
        // Inicialización de las variables globales para el Garbage Collection (Fase 4)
        shm_planta->gases_acumulados = 0.0f;
        shm_planta->alumina_enriquecida = 0.0f;

        // Inicialización para la Fase 5
        for (int i = 0; i < 5; ++i) {
            shm_planta->aluminio_producido[i] = 0.0f;
        }

        std::cout << "[Kernel] Memoria Virtual Compartida inicializada correctamente en RAM." << std::endl;
        return ptr;
    }

    void liberar_memoria_virtual() {
        if (shared_planta != nullptr && shared_planta != MAP_FAILED) {
            // Desmapear la memoria
            if (munmap(shared_planta, sizeof(MemoriaCompartidaPlanta)) == -1) {
                perror("Error en munmap");
            }
            // Eliminar el objeto del sistema operativo
            if (shm_unlink("/shm_venalum") == -1) {
                perror("Error en shm_unlink");
            } else {
                std::cout << "[Kernel] Memoria Virtual Compartida liberada y desenlazada (shm_unlink)." << std::endl;
            }
        }
    }
}
