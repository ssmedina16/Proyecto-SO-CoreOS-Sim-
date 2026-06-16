#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
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

        cout << "\n>>> [PROCESO] INICIANDO FASE 2: LOGÍSTICA Y TRANSPORTE - PID: " << getpid() << " <<<\n\n";

        // Bucle principal iterativo (simula solicitudes y transferencias internamente)
        while (system_running) {
            struct Solicitud { int id; int cantidad; };
            Solicitud solicitudes[MAX_SOLICITUDES];
            int solicitudes_count = 0;

            // Generar solicitudes de ejemplo 
            for (int i = 0; i < MAX_SOLICITUDES; ++i) {
                // generar cantidades variadas 
                solicitudes[i].id = i;
                solicitudes[i].cantidad = 500 + (i * 300); // 500,800,1100,...
                solicitudes_count++;
            }

            // Orden SJF por cantidad ascendente (selection sort)
            for (int i = 0; i < solicitudes_count - 1; ++i) {
                int min_idx = i;
                for (int j = i + 1; j < solicitudes_count; ++j) {
                    if (solicitudes[j].cantidad < solicitudes[min_idx].cantidad) min_idx = j;
                }
                if (min_idx != i) {
                    Solicitud tmp = solicitudes[i];
                    solicitudes[i] = solicitudes[min_idx];
                    solicitudes[min_idx] = tmp;
                }
            }

            // Procesar solicitudes (simulado)
            for (int s = 0; s < solicitudes_count && system_running; ++s) {
                int id = solicitudes[s].id;
                int cantidad = solicitudes[s].cantidad;

                {
                    // proteger logs con mutex
                    lock_guard<mutex> lg(candado_logistica);
                    cout << "[Logística] Procesando solicitud (simulada) -> ID: " << id
                         << " | Cantidad: " << cantidad << " unidades\n";
                }

                // simular tiempo de transferencia
                this_thread::sleep_for(chrono::milliseconds(200 + (cantidad % 400)));

                {
                    lock_guard<mutex> lg(candado_logistica);
                    cout << "[Logística] Transferencia simulada completa para ID: " << id << " (" << cantidad << " u)\n";
                }

                this_thread::sleep_for(chrono::milliseconds(150));
            }

            // dormir antes del siguiente ciclo
            this_thread::sleep_for(chrono::milliseconds(SLEEP_MS));
        }

        cout << "[Logística] Finalizando proceso de logística.\n";
        exit(0);
    }

} 
