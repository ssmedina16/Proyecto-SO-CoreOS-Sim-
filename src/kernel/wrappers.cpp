#include "../../include/kernel/wrappers.hpp"
#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal> 

extern volatile sig_atomic_t system_running;

namespace Industrial {

    void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
        PCB pcb_fase1 = { 101, "Planta de Carbon", ProcessState::LISTO, 3, 0.0, 25.0 };
        EstadoPlanta estado = { 101, 500.0f, 120.0f, 0 };

        scheduler.encolarProceso(pcb_fase1, 3);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(4));
            if (!system_running) break;
            
            if (pcb_fase1.estado == ProcessState::BLOQUEADO) {
                Hilo_Mezcladora(estado);
                Hilo_Horno(estado);
                
                pcb_fase1.rafaga_estimada = 25.0;
                pcb_fase1.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(pcb_fase1, 3);
            }
        }
    }

    void wrapper_fase2_logistica(MLFQScheduler& scheduler) {
        PCB pcb_fase2 = { 201, "Logistica y Transporte", ProcessState::LISTO, 2, 0.0, 8.0 };
        scheduler.encolarProceso(pcb_fase2, 2);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            if (!system_running) break;

            if (pcb_fase2.estado == ProcessState::BLOQUEADO) {
                simular_escaneo_tolvas();

                pcb_fase2.rafaga_estimada = 8.0;
                pcb_fase2.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(pcb_fase2, 2);
            }
        }
    }

    void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler) {
        PCB pcb_fase3 = { 301, "Celdas Reduccion y GTC", ProcessState::LISTO, 1, 0.0, 30.0 };
        EstadoCelda celdas[5];
        for (int i = 0; i < 5; ++i) {
            celdas[i] = {i + 1, 958.0f, 0.0f, 200.0f, 0.0f};
        }
        std::mutex mtx_celdas;

        scheduler.encolarProceso(pcb_fase3, 1);

        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            if (!system_running) break;

            if (pcb_fase3.estado == ProcessState::BLOQUEADO) {
                for (int i = 0; i < 5; ++i) {
                    if (shared_planta != nullptr) {
                        celdas[i].alumina_kg = shared_planta->tolvas_celdas[i];
                    }
                    Hilo_Celda(celdas[i], mtx_celdas);
                }

                simular_reciclaje_gtc(mtx_celdas);

                pcb_fase3.rafaga_estimada = 30.0;
                pcb_fase3.cambiarEstado(ProcessState::LISTO);
                scheduler.encolarProceso(pcb_fase3, 1);
            }
        }
    }
}