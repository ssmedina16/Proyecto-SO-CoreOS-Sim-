#include "../../include/kernel/wrappers.hpp"
#include "../../include/industrial/fases_produccion.hpp"
#include "../../include/kernel/sincronizacion.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <csignal>
#include <vector>
#include <mutex>

extern volatile sig_atomic_t system_running;

namespace Industrial {

    std::mutex mutex_gtc;

    // ── Recursos de logging CSV para Fase 5 (Trasiego del Crisol) ──
    static std::ofstream csv_f5;
    static std::mutex csv_mutex_f5;
    static auto start_time_f5 = std::chrono::steady_clock::now();

    static void log_csv_f5(const std::string &event, int crisol_id, int celda_id,
                           float kg_transferidos, float crisol1_nivel, float crisol2_nivel,
                           int lingotes_totales) {
        std::lock_guard<std::mutex> lock(csv_mutex_f5);
        if (!csv_f5.is_open()) {
            bool is_empty = false;
            {
                std::ifstream check("logs/fase5_crisol.csv");
                if (!check || check.peek() == std::ifstream::traits_type::eof()) {
                    is_empty = true;
                }
            }
            csv_f5.open("logs/fase5_crisol.csv", std::ios::out | std::ios::app);
            if (is_empty) {
                csv_f5 << "timestamp_ms,evento,crisol_id,celda_id,kg_transferidos,crisol1_nivel,crisol2_nivel,lingotes_totales\n" << std::flush;
            }
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_f5).count();
        csv_f5 << elapsed << "," << event << "," << crisol_id << "," << celda_id << ","
               << kg_transferidos << "," << crisol1_nivel << "," << crisol2_nivel << "," << lingotes_totales << "\n" << std::flush;
    }

    void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
        PCB pcb_fase1 = { 101, "Planta de Carbon", ProcessState::LISTO, 1, 0.0, 15.0 };
        pcb_fase1.estado_compartido = &pcb_fase1.estado;
        scheduler.encolarProceso(&pcb_fase1, 1);

        while (system_running) {
            if (pcb_fase1.estado == ProcessState::BLOQUEADO) {
                std::cout << "\n[KERNEL] -> Despertando Fase 1 en Planta de Carbón...\n";
                Industrial::fase_planta_carbon();

                pcb_fase1.rafaga_estimada = 15.0;
                pcb_fase1.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase1, 1);

                // Cadencia: Fase 1 produce 4 ánodos rápidamente cada 800 ms
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void wrapper_fase2_logistica(MLFQScheduler& scheduler) {
        PCB pcb_fase2 = { 201, "Logistica y Transporte", ProcessState::LISTO, 1, 0.0, 8.0 };
        pcb_fase2.estado_compartido = &pcb_fase2.estado;
        scheduler.encolarProceso(&pcb_fase2, 1);

        while (system_running) {
            if (pcb_fase2.estado == ProcessState::BLOQUEADO) {
                std::cout << "\n[KERNEL] -> Despertando Fase 2 (Logística)...\n";
                Industrial::fase_logistica_transporte();

                pcb_fase2.rafaga_estimada = 8.0;
                pcb_fase2.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase2, 1);

                // Cadencia: Logística re-escanea tolvas cada 2 segundos
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }

    void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler) {
        PCB pcb_fase3 = { 301, "Celdas Reduccion y GTC", ProcessState::LISTO, 1, 0.0, 20.0 };
        pcb_fase3.estado_compartido = &pcb_fase3.estado;
        scheduler.encolarProceso(&pcb_fase3, 1);

        while (system_running) {
            if (pcb_fase3.estado == ProcessState::BLOQUEADO) {
                std::cout << "\n[KERNEL] -> Despertando Fase 3 (Celdas) y Fase 4 (GTC)...\n";

                Industrial::fase_celdas_reduccion(Industrial::MAX_CELDAS);
                Industrial::fase_reciclaje_gtc(mutex_gtc);

                pcb_fase3.rafaga_estimada = 20.0;
                pcb_fase3.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase3, 1);

                // Cadencia: Electrólisis ocurre cada 2.5 segundos
                std::this_thread::sleep_for(std::chrono::milliseconds(2500));
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

        static int lingotes_totales = 0;
        static int turno_crisol = 0;
        static int celda_inicio_trasiego = 0;

        // ── Log CSV: Evento de arranque ──
        log_csv_f5("INICIADO", 0, 0, 0.0f,
                   lista_crisoles[0].aluminio_recolectado,
                   lista_crisoles[1].aluminio_recolectado,
                   lingotes_totales);

        while (system_running) {
            if (pcb_fase5.estado == ProcessState::BLOQUEADO) {
                // Alternar crisoles de manera rotativa (Round-Robin entre crisoles)
                for (int step_c = 0; step_c < 2; ++step_c) {
                    int c = (turno_crisol + step_c) % 2;
                    lista_crisoles[c].en_operacion = true;

                    for (int step_i = 0; step_i < 5; ++step_i) {
                        if (!system_running) break;
                        int i = (celda_inicio_trasiego + step_i) % 5;

                        // Si la celda tiene >= 200kg de aluminio y el crisol tiene espacio disponible
                        if (shared_planta != nullptr && shared_planta->aluminio_producido[i] >= 200.0f
                            && (lista_crisoles[c].aluminio_recolectado + shared_planta->aluminio_producido[i]) <= (lista_crisoles[c].capacidad_max + 100.0f)) {
                            
                            std::cout << "[Aviso] Hilo_Crisol #" << lista_crisoles[c].id_crisol 
                                      << " detecta aluminio en Celda #" << (i + 1) 
                                      << " (" << shared_planta->aluminio_producido[i] << " kg)\n";
                            
                            pcb_crisol[c].estado = EstadoProceso::BLOQUEADO;
                            pcb_crisol[c].esperando_por = 1; // 1 = ID de kmutex_crisol_succion

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
                                          << " kg retirados de Celda #" << (i + 1) 
                                          << " por Crisol #" << lista_crisoles[c].id_crisol << "\n";

                                log_csv_f5("VACIADO_CELDA", lista_crisoles[c].id_crisol, i + 1,
                                           material_transferido,
                                           lista_crisoles[0].aluminio_recolectado,
                                           lista_crisoles[1].aluminio_recolectado,
                                           lingotes_totales);

                                ksync.unlock(1, scheduler);
                                break;
                            } else {
                                log_csv_f5("DEADLOCK_PREVENIDO", lista_crisoles[c].id_crisol, i + 1,
                                           0.0f,
                                           lista_crisoles[0].aluminio_recolectado,
                                           lista_crisoles[1].aluminio_recolectado,
                                           lingotes_totales);
                            }
                        }
                    }

                    // --- FUNDICIÓN Y MOLDEO DE LINGOTES DE ALUMINIO ---
                    // El crisol debe ACUMULAR aluminio hasta alcanzar su capacidad máxima antes de fundir.
                    // 1 Lingote de Aluminio = 100 kg de aluminio líquido succionado
                    if (lista_crisoles[c].aluminio_recolectado >= lista_crisoles[c].capacidad_max) {
                        int nuevos_lingotes = (int)(lista_crisoles[c].aluminio_recolectado / 100.0f);
                        float metal_moldeado = nuevos_lingotes * 100.0f;
                        lista_crisoles[c].aluminio_recolectado -= metal_moldeado;
                        lingotes_totales += nuevos_lingotes;

                        std::cout << "[Fundición] Crisol #" << lista_crisoles[c].id_crisol 
                                  << " moldeo " << nuevos_lingotes << " lingote(s) de aluminio (" 
                                  << metal_moldeado << " kg). [Total Lingotes: " << lingotes_totales << "]\n";

                        log_csv_f5("MOLDEO_LINGOTE", lista_crisoles[c].id_crisol, 0,
                                   metal_moldeado,
                                   lista_crisoles[0].aluminio_recolectado,
                                   lista_crisoles[1].aluminio_recolectado,
                                   lingotes_totales);
                    }

                    lista_crisoles[c].en_operacion = false;
                }

                turno_crisol = (turno_crisol + 1) % 2;
                celda_inicio_trasiego = (celda_inicio_trasiego + 1) % 5;

                pcb_fase5.rafaga_estimada = 15.0;
                pcb_fase5.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(&pcb_fase5, 1);

                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // ── Log CSV: Evento de finalización ──
        log_csv_f5("FINALIZADO", 0, 0, 0.0f,
                   lista_crisoles[0].aluminio_recolectado,
                   lista_crisoles[1].aluminio_recolectado,
                   lingotes_totales);
        if (csv_f5.is_open()) csv_f5.close();

        std::cout << "[Fundición] Resumen final: " << lingotes_totales << " lingotes de aluminio producidos.\n";
    }
}