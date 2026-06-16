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

// Manejador de señal
static void recibir_signal_apagado_f2(int sig) {
    if (sig == SIGTERM) {
        system_running = 0;
    }
}

namespace Industrial {

    void fase_logistica_transporte() {
        signal(SIGTERM, recibir_signal_apagado_f2);

        std::ofstream log_file("logs/fase2_logistica.log", std::ios::out | std::ios::trunc);
        if (!log_file.is_open()) {
            std::cerr << "Critical Error: Could not create Phase 2 log file." << std::endl;
            exit(1);
        }
        log_file << "=== INITIALIZATION OF LOGISTICAL LOG (PHASE 2) ===\n" << std::flush;

        cout << "\n>>> [PROCESO] INICIANDO FASE 2: LOGÍSTICA Y TRANSPORTE (SIMULADA) - PID: " << getpid() << " <<<\n\n";

        // Bucle principal iterativo (escaneo de tolvas globales)
        while (system_running) {
            for (int i = 0; i < MAX_CELDAS; ++i) {
                if (TOLVAS_CELDAS_GLOBAL[i] < 500.0f) {
                    float cantidad_a_enviar = 1000.0f - TOLVAS_CELDAS_GLOBAL[i];

                    if (SILO_ALUMINA_GLOBAL >= cantidad_a_enviar) {
                        SILO_ALUMINA_GLOBAL -= cantidad_a_enviar;
                        TOLVAS_CELDAS_GLOBAL[i] += cantidad_a_enviar;

                        {
                            // proteger logs con mutex
                            lock_guard<mutex> lg(candado_logistica);
                            log_file << "[Logística] RELLENANDO Celda #" << (i + 1)
                                     << " | Cantidad: " << cantidad_a_enviar << " kg | Silo Restante: " 
                                     << SILO_ALUMINA_GLOBAL << " kg\n" << std::flush;
                        }

                        // simular tiempo de transferencia neumática
                        this_thread::sleep_for(chrono::milliseconds(800));
                    }
                }
            }

            // dormir antes del siguiente ciclo de escaneo
            this_thread::sleep_for(chrono::milliseconds(SLEEP_MS));
        }

        log_file << "[Logística] Finalizando proceso de logística.\n" << std::flush;
        log_file.close();
        exit(0);
    }

} 
