#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

// Referencia a la variable global de control definida en main.cpp
extern bool system_running;

// --- FUNCIONES INTERNAS DE SIMULACIÓN (TRABAJADORES CONCURRENTES) ---

/**
 * Hilo_Mezcladora: Simula la recepción de coque y brea, y el proceso de mezclado.
 */
void Hilo_Mezcladora() {
    std::cout << "[Planta Carbón] Mezcladora iniciada." << std::endl;
    
    while (system_running) {
        std::cout << "[Mezcladora] Recibiendo materias primas (coque y brea)..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(800)); // Simulación de carga
        
        std::cout << "[Mezcladora] Mezclando componentes a alta temperatura..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // Simulación de mezclado
        
        std::cout << "[Mezcladora] Mezcla lista. Transfiriendo masa anódica al Horno." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "[Planta Carbón] Mezcladora finalizada." << std::endl;
}

/**
 * Hilo_Horno: Simula el ciclo térmico de cocción de los ánodos.
 */
void Hilo_Horno() {
    std::cout << "[Planta Carbón] Horno de Cocción iniciado." << std::endl;
    
    while (system_running) {
        std::cout << "[Horno] Esperando carga de masa anódica..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!system_running) break;

        std::cout << "[Horno] Iniciando ciclo térmico de cocción..." << std::endl;
        // Simulación de cocción (delay más largo para representar proceso térmico)
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cout << "[Horno] Ciclo completado. Ánodos verdes listos para transporte." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "[Planta Carbón] Horno finalizado." << std::endl;
}

namespace Industrial {

    /**
     * Módulo 1: Planta de Carbón (Hilo Gestor)
     * Coordina la Mezcladora y el Horno en paralelo.
     */
    void fase_planta_carbon() {
        std::cout << "[Módulo 1] Iniciando Planta de Carbón (Hilo Gestor)..." << std::endl;

        // Lanzar hilos trabajadores concurrentes
        std::thread worker_mezcladora(Hilo_Mezcladora);
        std::thread worker_horno(Hilo_Horno);

        // El gestor espera a que los hilos trabajadores terminen (cuando system_running sea false)
        if (worker_mezcladora.joinable()) {
            worker_mezcladora.join();
        }
        
        if (worker_horno.joinable()) {
            worker_horno.join();
        }

        std::cout << "[Módulo 1] Planta de Carbón apagada correctamente." << std::endl;
    }

    // --- STUBS DE OTROS MÓDULOS 

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
