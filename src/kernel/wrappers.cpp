#include "../../include/kernel/wrappers.hpp"
#include "../../include/industrial/fases_produccion.hpp"
#include "../../include/kernel/sincronizacion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <vector>
#include <mutex>

extern volatile sig_atomic_t system_running;

namespace Industrial {

    std::mutex mutex_gtc;
    
    void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
        // Nace en la Cola 3 (FCFS) por ser un proceso lento de horneado
        PCB pcb_fase1 = { 101, "Planta de Carbon", ProcessState::LISTO, 3, 0.0, 25.0 };
        pcb_fase1.estado_compartido = &pcb_fase1.estado;
        
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
        pcb_fase2.estado_compartido = &pcb_fase2.estado;
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
        pcb_fase3.estado_compartido = &pcb_fase3.estado;
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

    void wrapper_fase5_trasiego(MLFQScheduler& scheduler) {
        // Nace legítimamente en la Cola 1 (Round Robin)
        PCB pcb_fase5 = { 501, "Trasiego Crisol", ProcessState::LISTO, 1, 0.0, 15.0 };
        pcb_fase5.estado_compartido = &pcb_fase5.estado;
        scheduler.encolarProceso(&pcb_fase5, 1);

        // Registrar el KSemaphore de crisoles en el motor de sincronizacion
        // (2 crisoles pueden operar en paralelo sobre distintas celdas)
        KernelSyncEngine& ksync = KernelSyncEngine::getInstance();
        ksync.registrarSemaforo(10, "crisoles_disponibles", 2);

        // Estructura para los 2 crisoles
        std::vector<EstadoCrisol> lista_crisoles(2);
        for (int i = 0; i < 2; ++i) {
            lista_crisoles[i].id_crisol = i + 1;
            lista_crisoles[i].capacidad_max = 300.0f; // Umbral crítico para simulación
            lista_crisoles[i].aluminio_recolectado = 0.0f;
            lista_crisoles[i].en_operacion = false;
        }

        // Estructura PCB simulada para el Crisol
        PCB_Venalum pcb_crisol[2];
        for (int i = 0; i < 2; ++i) {
            pcb_crisol[i].PID = (i + 1) + 200; // 201, 202
            pcb_crisol[i].Tipo_Entidad = 2;   // 2 = Crisol
            pcb_crisol[i].estado = EstadoProceso::EJECUTANDO;
            pcb_crisol[i].program_counter = 0;
            pcb_crisol[i].base_limit = sizeof(float) * 5;
            pcb_crisol[i].cuota_alumina = (int)lista_crisoles[i].capacidad_max;
            pcb_crisol[i].esperando_por = 0;
        }

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(6));
            if (!system_running) break;

            if (pcb_fase5.estado == ProcessState::BLOQUEADO) {
                // --- EJECUCIÓN EXACTA DE LA FASE 5 (TRASIEGO) EN HILOS ---
                for (int c = 0; c < 2; ++c) {
                    lista_crisoles[c].en_operacion = true;
                    for (int i = 0; i < 5; ++i) {
                        if (!system_running) break;

                        if (shared_planta != nullptr && shared_planta->aluminio_producido[i] >= lista_crisoles[c].capacidad_max) {
                            std::cout << "[Aviso] Hilo_Crisol #" << lista_crisoles[c].id_crisol 
                                      << " detecta exceso en Celda #" << (i + 1) 
                                      << " (" << shared_planta->aluminio_producido[i] << " kg)\n";
                            
                            pcb_crisol[c].estado = EstadoProceso::BLOQUEADO;
                            pcb_crisol[c].esperando_por = 1; // 1 = ID de kmutex_crisol_succion
                            std::cout << "[Kernel OS] PCB PID " << pcb_crisol[c].PID 
                                      << " cambiado a BLOQUEADO esperando kmutex_crisol_succion (ID: 1)\n";

                            // Exclusion mutua via KernelSyncEngine (detecta deadlocks + herencia de prio)
                            PCB pcb_proxy;
                            pcb_proxy.id              = pcb_crisol[c].PID;
                            pcb_proxy.nombre_fase     = "Hilo_Crisol_" + std::to_string(c + 1);
                            pcb_proxy.estado          = ProcessState::LISTO;
                            pcb_proxy.prioridad_actual= 1;
                            pcb_proxy.tiempo_ejecutado= 0.0;
                            pcb_proxy.rafaga_estimada = 5.0;

                            bool lock_ok = ksync.lock(1, pcb_proxy, scheduler);
                            if (lock_ok) {
                                pcb_crisol[c].estado = EstadoProceso::EJECUTANDO;
                                pcb_crisol[c].esperando_por = 0;

                                float material_transferido = shared_planta->aluminio_producido[i];
                                shared_planta->aluminio_producido[i] = 0.0f;
                                lista_crisoles[c].aluminio_recolectado += material_transferido;

                                std::cout << "[Exito] Succion finalizada: " << material_transferido 
                                          << " kg retirados de la Celda #" << (i + 1) << "\n";

                                ksync.unlock(1, scheduler);
                            } else {
                                std::cout << "[KernelSyncEngine] Operacion de vaciado cancelada por prevencion de Deadlock.\n";
                            }
                            break; // Sigue al siguiente crisol/ciclo
                        }
                    }
                    lista_crisoles[c].en_operacion = false;
                }

                // Re-encolar en planificador
                pcb_fase5.rafaga_estimada = 15.0;
                pcb_fase5.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase5, 1);
            }
        }

        // Resumen final de recolección al apagar
        std::cout << "[Logística y Trasiego] Resumen de recolección de Crisoles:\n";
        for (int i = 0; i < 2; ++i) {
            std::cout << "  * Crisol #" << lista_crisoles[i].id_crisol 
                      << ": " << lista_crisoles[i].aluminio_recolectado << " kg recolectados.\n";
        }
        std::cout << "[Logística y Trasiego] Proceso finalizado.\n";
    }
}