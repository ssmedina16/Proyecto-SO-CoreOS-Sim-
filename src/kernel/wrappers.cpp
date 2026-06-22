#include "../../include/kernel/wrappers.hpp"
#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal> 

extern volatile sig_atomic_t system_running;

namespace Industrial {

    void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
        // Nace legítimamente en la Cola 3 (FCFS) por ser un proceso lento de horneado
        PCB pcb_fase1 = { 101, "Planta de Carbon", ProcessState::LISTO, 3, 0.0, 25.0 };
        
        float coque_kg = 500.0f;
        float brea_kg = 120.0f;

        // Registrar inicialmente en el planificador
        scheduler.encolarProceso(pcb_fase1, 3);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            if (!system_running) break;
            
            if (pcb_fase1.estado == ProcessState::BLOQUEADO) {
                // --- REPRODUCCIÓN EXACTA DE LOS LOGS ORIGINALES DE FASE 1 ---
                std::cout << "[Mezcladora - PID: " << pcb_fase1.id << "] Cargando materias primas...\\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                std::cout << "[Mezcladora - PID: " << pcb_fase1.id << "] Masa preparada.\\n\\n";

                float coque_req = 50.0f;
                float brea_req = 12.0f;

                if (coque_kg >= coque_req && brea_kg >= brea_req) {
                    coque_kg -= coque_req;
                    brea_kg -= brea_req;
                    
                    std::cout << "[Horno - PID: " << pcb_fase1.id << "] [Materia Prima] Consumiendo coque y brea del inventario local para el ciclo térmico.\\n";
                    std::cout << "[Horno - PID: " << pcb_fase1.id << "] Iniciando ciclo de cocción en horno (Espera activa de 3s).\\n";
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(800));

                    if (shared_planta != nullptr) {
                        shared_planta->anodos_producidos++;
                    }
                    
                    std::cout << "---------------------------------------------------------\\n";
                    std::cout << "[Horno - PID: " << pcb_fase1.id << "] [MEMORIA] ¡Ánodo Cocido generado con éxito! Inyectando recurso en RAM global. [Total SHM: " 
                              << (shared_planta ? shared_planta->anodos_producidos : 0) << "]\\n";
                    
                    // Re-encolar administrativamente en FCFS
                    pcb_fase1.rafaga_estimada = 25.0;
                    pcb_fase1.cambiarEstado(ProcessState::LISTO);
                    scheduler.encolarProceso(pcb_fase1, 3);
                } else {
                    std::cout << "[Horno - PID: " << pcb_fase1.id << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN.\\n";
                    coque_kg += 150.0f;
                    brea_kg += 40.0f;
                }
            }
        }
    }

    void wrapper_fase2_logistica(MLFQScheduler& scheduler) {
        // Nace en la Cola 2 (SJF) para optimizar el transporte neumático ágil
        PCB pcb_fase2 = { 201, "Logistica y Transporte", ProcessState::LISTO, 2, 0.0, 8.0 };
        scheduler.encolarProceso(pcb_fase2, 2);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            if (!system_running) break;

            if (pcb_fase2.estado == ProcessState::BLOQUEADO) {
                // --- REPRODUCCIÓN EXACTA DE LOS LOGS ORIGINALES DE FASE 2 ---
                std::cout << "[Logística] [DMA Scan] Escaneando mapa de tolvas en memoria compartida...\\n";

                if (shared_planta != nullptr) {
                    for (int i = 0; i < MAX_CELDAS; ++i) {
                        if (shared_planta->tolvas_celdas[i] < 500.0f) {
                            float nivel_actual = shared_planta->tolvas_celdas[i];
                            float cantidad_a_enviar = 1000.0f - nivel_actual;

                            if (shared_planta->silo_alumina >= cantidad_a_enviar) {
                                shared_planta->silo_alumina -= cantidad_a_enviar;
                                shared_planta->tolvas_celdas[i] += cantidad_a_enviar;

                                std::cout << "[Logística] [ALERTA] Celda #" << (i + 1) << " requiere reabastecimiento. Nivel actual: " << nivel_actual << " kg (Umbral < 500kg).\\n"
                                          << "[Logística] [IPC] Retirando recursos del Silo Central. [Silo SHM Restante: " << shared_planta->silo_alumina << " kg]\\n"
                                          << "[Logística] [TRANSFERENCIA OK] Inyectando recursos en la dirección de memoria de la Celda #" << (i + 1) << ". [Tolva SHM: " << shared_planta->tolvas_celdas[i] << " kg]\\n"
                                          << "---------------------------------------------------------\\n";
                            }
                        }
                    }
                }

                // Re-encolar administrativamente en la cola de prioridad de trabajos cortos (SJF)
                pcb_fase2.rafaga_estimada = 8.0;
                pcb_fase2.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(pcb_fase2, 2);
            }
        }
    }

    void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler) {
        // Nace en la Cola 1 (Round Robin) para la gestión de misión crítica
        PCB pcb_fase3 = { 301, "Celdas Reduccion y GTC", ProcessState::LISTO, 1, 0.0, 30.0 };
        float aluminio_producido[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

        scheduler.encolarProceso(pcb_fase3, 1);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!system_running) break;

            if (pcb_fase3.estado == ProcessState::BLOQUEADO) {
                if (shared_planta != nullptr) {
                    
                    // --- REPRODUCCIÓN EXACTA DE LOS LOGS ORIGINALES DE FASE 3 ---
                    for (int i = 0; i < 5; ++i) {
                        int id_celda = i + 1;
                        std::cout << "[Celda #" << id_celda << "] [SHM Read] Leyendo memoria compartida -> Ánodos listos: " 
                                  << shared_planta->anodos_producidos << " | Alúmina Eco: " 
                                  << shared_planta->alumina_enriquecida << " kg\\n";

                        float alumina_req = 200.0f;
                        bool produccion_ok = false;

                        if (shared_planta->alumina_enriquecida >= alumina_req && shared_planta->anodos_producidos >= 1) {
                            shared_planta->alumina_enriquecida -= alumina_req;
                            shared_planta->anodos_producidos--;
                            produccion_ok = true;
                            std::cout << "[Celda #" << id_celda << "] [RECURSO ECO] Alúmina Enriquecida del GTC de la Fase 4 detectada en RAM. Consumiendo con prioridad verde.\\n";
                        }
                        else if (shared_planta->tolvas_celdas[i] >= alumina_req && shared_planta->anodos_producidos >= 1) {
                            shared_planta->tolvas_celdas[i] -= alumina_req;
                            shared_planta->anodos_producidos--;
                            produccion_ok = true;
                        }

                        if (produccion_ok) {
                            aluminio_producido[i] += 100.0f;
                            shared_planta->gases_acumulados += 50.0f;

                            std::cout << "[Celda #" << id_celda << "] >>> NÚCLEO DE ELECTRÓLISIS ACTIVO <<< | Reducción Exitosa | Ánodo Consumido | Aluminio Líquido Acumulado: " 
                                      << aluminio_producido[i] << " kg | [SHM Gases Inyectados: +50kg]\\n"
                                      << "---------------------------------------------------------\\n";
                        } else {
                            std::cout << "[Celda #" << id_celda << "] FALTAN RECURSOS.\\n";
                        }
                    }

                    // --- REPRODUCCIÓN EXACTA DE LOS LOGS ORIGINALES DE FASE 4 (GTC) ---
                    std::cout << "[GTC - Garbage Collector] Escaneando el registro de gases residuales globales en la memoria...\\n";
                    if (shared_planta->gases_acumulados >= 100.0f) {
                        float gases_capturados = shared_planta->gases_acumulados;
                        shared_planta->gases_acumulados = 0.0f;
                        shared_planta->alumina_enriquecida += (gases_capturados * 0.5f);

                        std::cout << "[GTC] [ALERTA] Nivel crítico de emisiones detectado (" << gases_capturados << " kg). Iniciando ciclo atómico de lavado químico...\\n"
                                  << "[GTC] [MEMORIA] Registro 'gases_acumulados' purgado a 0.0f en la RAM real. Espacio virtual liberado.\\n"
                                  << "[GTC] [RECICLAJE] Materia prima regenerada: +" << (gases_capturados * 0.5f) << " kg de Alúmina Enriquecida escrita en RAM global. [Total SHM Eco: " << shared_planta->alumina_enriquecida << "]\\n"
                                  << "---------------------------------------------------------\\n";
                    }
                }

                // Re-encolar administrativamente en Cola 1 para Round Robin continuo
                pcb_fase3.rafaga_estimada = 30.0;
                pcb_fase3.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(pcb_fase3, 1);
            }
        }
    }
}