#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <mutex>

using namespace std;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Candado para la consola dentro de este proceso
static mutex candado_planta;

/**
 * Manejador de señales interno para el proceso de la Planta.
 */
static void recibir_signal_apagado_f1(int sig) {
    if (sig == SIGTERM) {
        system_running = 0;
    }
}

namespace Industrial {

    void Hilo_Mezcladora(Industrial::EstadoPlanta &estado) {
        while (system_running) {
            {
                lock_guard<mutex> lock(candado_planta);
                cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
            }

            this_thread::sleep_for(chrono::milliseconds(800));
            if (!system_running) break;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
            }
            this_thread::sleep_for(chrono::milliseconds(200));
        }
    }

    void Hilo_Horno(Industrial::EstadoPlanta &estado) {
        while (system_running) {
            bool recursos_ok = false;
            float coque_req = 50.0f;
            float brea_req = 12.0f;

            {
                lock_guard<mutex> lock(candado_planta);
                if (estado.coque_kg >= coque_req && estado.brea_kg >= brea_req) {
                    estado.coque_kg -= coque_req;
                    estado.brea_kg -= brea_req;
                    recursos_ok = true;
                    cout << "[Horno - PID: " << estado.pid_proceso << "] Iniciando ciclo de cocción (3s).... [Quedan: " 
                         << estado.coque_kg << "kg Coque | " << estado.brea_kg << "kg Brea]\n";
                } else {
                    cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN.\n";
                }
            }

            if (recursos_ok) {
                this_thread::sleep_for(chrono::milliseconds(3000));
                if (!system_running) break;

                {
                    lock_guard<mutex> lock(candado_planta);
                    estado.anodos_producidos++;
                    cout << "[Horno - PID: " << estado.pid_proceso << "] Ánodos listos: " << estado.anodos_producidos << "\n\n";
                }
            } else {
                this_thread::sleep_for(chrono::milliseconds(1500));
            }
        }
    }

    void fase_planta_carbon() {
        signal(SIGTERM, recibir_signal_apagado_f1);
        EstadoPlanta mi_estado = { getpid(), 500.0f, 120.0f, 0 };

        cout << "\n>>> [PROCESO] INICIANDO FASE 1: PLANTA DE CARBÓN - PID: " << mi_estado.pid_proceso << " <<<\n\n";

        thread t1(Hilo_Mezcladora, ref(mi_estado));
        thread t2(Hilo_Horno, ref(mi_estado));

        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();

        cout << "[Planta Carbón] Proceso finalizado.\n";
        exit(0);
    }
}
