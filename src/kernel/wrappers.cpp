#include "../../include/kernel/wrappers.hpp"
#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal> 
#include <mutex>

extern volatile sig_atomic_t system_running;

namespace Industrial {

    std::mutex mutex_gtc;
    
    void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
        // Nace en la Cola 3 (FCFS) por ser un proceso lento de horneado
        PCB pcb_fase1 = { 101, "Planta de Carbon", ProcessState::LISTO, 3, 0.0, 25.0 };
        
        // REGISTRO CORRECTO: Pasamos la dirección de memoria de nuestro PCB local
        scheduler.encolarProceso(&pcb_fase1, 3);

        while (system_running) {
            // El planificador cambiará el estado de este objeto real a BLOQUEADO al terminar su ráfaga de CPU
            if (pcb_fase1.estado == ProcessState::BLOQUEADO) {
                
                std::cout << "\n[KERNEL] -> Despertando Fase 1 en Planta de Carbón...\n";
                
                // === LLAMADA A FASE 1 (fases_produccion.hpp) ===
                // Invoca la función principal de producción de ánodos
                Industrial::fase_planta_carbon(); 
                
                // Re-encolar administrativamente en FCFS tras culminar el ciclo de trabajo físico
                pcb_fase1.rafaga_estimada = 25.0;
                pcb_fase1.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase1, 3);
            } else {
                // Pequeña espera de cortesía (10ms) para no sobrecargar la CPU física de tu equipo
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void wrapper_fase2_logistica(MLFQScheduler& scheduler) {
        // Nace en la Cola 2 (SJF) para optimizar el transporte neumático ágil
        PCB pcb_fase2 = { 201, "Logistica y Transporte", ProcessState::LISTO, 2, 0.0, 8.0 };
        
        scheduler.encolarProceso(&pcb_fase2, 2);

        while (system_running) {
            if (pcb_fase2.estado == ProcessState::BLOQUEADO) {
                
                std::cout << "\n[KERNEL] -> Despertando Fase 2 (Sistema de Logística)...\n";

                // === LLAMADA A FASE 2 (fases_produccion.hpp) ===
                // Activa el escaneo y reabastecimiento de alúmina en las tolvas
                Industrial::fase_logistica_transporte();

                // Re-encolar administrativamente en la cola de prioridad de trabajos cortos (SJF)
                pcb_fase2.rafaga_estimada = 8.0;
                pcb_fase2.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase2, 2);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler) {
        // Nace en la Cola 1 (Round Robin) para la gestión de misión crítica
        PCB pcb_fase3 = { 301, "Celdas Reduccion y GTC", ProcessState::LISTO, 1, 0.0, 30.0 };
        
        scheduler.encolarProceso(&pcb_fase3, 1);

        while (system_running) {
            if (pcb_fase3.estado == ProcessState::BLOQUEADO) {
                
                std::cout << "\n[KERNEL] -> Despertando Fase 3 (Celdas) y Fase 4 (GTC)...\n";

                // === LLAMADA A FASE 3 (fases_produccion.hpp) ===
                // Pasamos MAX_CELDAS (5) tal como solicita el parámetro de tu firma
                Industrial::fase_celdas_reduccion(Industrial::MAX_CELDAS);

                // === LLAMADA A FASE 4 (fases_produccion.hpp) ===
                // El sistema de lavado químico procesa los gases emitidos usando el mutex requerido
                Industrial::fase_reciclaje_gtc(mutex_gtc);

                // Re-encolar administrativamente en Cola 1 para Round Robin continuo
                pcb_fase3.rafaga_estimada = 30.0;
                pcb_fase3.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase3, 1);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
}