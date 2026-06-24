#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <mutex>

using namespace std;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Parámetros operativos (simulación interna)
static const int MAX_SOLICITUDES = 8;
static const int SLEEP_MS = 1500;

// Candado para logs dentro del proceso de logística
static mutex candado_logistica;

namespace Industrial {

    void fase_logistica_transporte() {
        // !!! bucle while(system_running) ELIMINADO !!!
        // !!! exit(0) REMOVIDO COMPLETAMENTE !!!

        // Cambiado a ios::app para acumular las transferencias a lo largo de la simulación
        std::ofstream log_file("logs/fase2_logistica.log", std::ios::out | std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Error Crítico: No se pudo abrir el archivo de log de Logística." << std::endl;
            return;
        }

        {
            lock_guard<mutex> lg(candado_logistica);
            cout << "[Logística] [DMA Scan] Escaneando mapa de tolvas en memoria compartida...\n";
        }

        if (shared_planta != nullptr) {
            for (int i = 0; i < MAX_CELDAS; ++i) {
                if (shared_planta->tolvas_celdas[i] < 500.0f) {
                    float nivel_actual = shared_planta->tolvas_celdas[i];
                    float cantidad_a_enviar = 1000.0f - nivel_actual;

                    if (shared_planta->silo_alumina >= cantidad_a_enviar) {
                        shared_planta->silo_alumina -= cantidad_a_enviar;
                        shared_planta->tolvas_celdas[i] += cantidad_a_enviar;

                        {
                            lock_guard<mutex> lg(candado_logistica);
                            log_file << "[Logística] [ALERTA] Celda #" << (i + 1) << " requiere reabastecimiento. Nivel actual: " << nivel_actual << " kg (Umbral < 500kg).\n"
                                     << "[Logística] [IPC] Retirando recursos del Silo Central. [Silo SHM Restante: " << shared_planta->silo_alumina << " kg]\n"
                                     << "[Logística] [TRANSFERENCIA OK] Inyectando recursos en la dirección de memoria de la Celda #" << (i + 1) << ". [Tolva SHM: " << shared_planta->tolvas_celdas[i] << " kg]\n"
                                     << "---------------------------------------------------------\n" << std::flush;
                            
                            cout << "[Logística] [ALERTA] Inyectando recursos neumáticos en Celda #" << (i + 1) << "\n";
                        }

                        // Simula el retardo atómico de la transferencia por este turno
                        this_thread::sleep_for(chrono::milliseconds(200));
                    }
                }
            }
        }

        log_file.close();
    }

} 
