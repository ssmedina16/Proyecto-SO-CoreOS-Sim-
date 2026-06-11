#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <unistd.h>  // Para getpid()
#include <cstdlib>   // Para std::exit()

// 1. MANEJO DE SEÑALES ATÓMICAS: Declaración externa segura para IPC
extern volatile sig_atomic_t system_running;

/**
 * 2. FLUJO SECUENCIAL DE RELOJ INDUSTRIAL: Sub-etapa Mezcladora
 * Simula la carga de materias primas (coque y brea) con retardo de 800ms.
 */
void ejecutar_ciclo_mezcladora() {
    if (!system_running) return;

    std::cout << "[Mezcladora - PID: " << getpid() << "] Recibiendo materias primas (coque y brea)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    
    // Guarda de estado inmediata tras delay
    if (!system_running) return;

    std::cout << "[Mezcladora - PID: " << getpid() << "] Masa anódica preparada." << std::endl;
}

/**
 * 2. FLUJO SECUENCIAL DE RELOJ INDUSTRIAL: Sub-etapa Horno
 * Simula el ciclo térmico de cocción con retardo de 3000ms.
 */
void ejecutar_ciclo_horno() {
    if (!system_running) return;

    std::cout << "[Horno - PID: " << getpid() << "] Iniciando ciclo térmico de cocción (3s)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // Guarda de estado inmediata tras delay
    if (!system_running) return;

    std::cout << "[Horno - PID: " << getpid() << "] Ciclo completado. Ánodos listos." << std::endl;
}

namespace Industrial {

    /**
     * Módulo 1: Planta de Carbón (Proceso Hijo)
     * Ejecuta las sub-etapas de forma lineal dentro de su ciclo de vida.
     */
    void fase_planta_carbon() {
        std::cout << "[Planta Carbón - PID: " << getpid() << "] Proceso iniciado correctamente." << std::endl;

        // Bucle principal del proceso hijo
        while (system_running) {
            ejecutar_ciclo_mezcladora();
            
            if (!system_running) break;

            ejecutar_ciclo_horno();
            
            if (!system_running) break;

            std::cout << "[Planta Carbón - PID: " << getpid() << "] Ciclo de producción finalizado. Reiniciando..." << std::endl;
        }

        // 3. APAGADO LIMPIO: El proceso hijo debe terminar mediante exit(0)
        std::cout << "[Planta Carbón - PID: " << getpid() << "] Finalizando proceso de forma segura..." << std::endl;
        std::exit(0);
    }

    /**
     * 5. RESTRICCIÓN DE STUBS PARA COMPAÑEROS
     */
    void fase_logistica_transporte() {
        // TODO: Asignado a otros compañeros
    }

    void fase_celdas_reduccion() {
        // TODO: Asignado a otros compañeros
    }

    void fase_reciclaje_gtc() {
        // TODO: Asignado a otros compañeros
    }

    void fase_trasiego_crisol() {
        // TODO: Asignado a otros compañeros
    }
}
