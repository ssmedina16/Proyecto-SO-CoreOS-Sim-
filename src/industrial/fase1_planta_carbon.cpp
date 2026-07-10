#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <fstream>
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

// Recursos y funciones de registro CSV para Fase 1
static ofstream csv_f1;
static mutex csv_mutex_f1;
static auto start_time_f1 = chrono::steady_clock::now();

static void log_csv_f1(const string &event, int pid, float coque, float brea, int anodos) {
    lock_guard<mutex> lock(csv_mutex_f1);
    if (!csv_f1.is_open()) {
        csv_f1.open("logs/fase1_planta.csv", ios::out | ios::trunc);
        csv_f1 << "timestamp_ms,evento,pid,coque_kg,brea_kg,anodos_producidos\n" << flush;
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f1).count();
    csv_f1 << elapsed << "," << event << "," << pid << "," << coque << "," << brea << "," << anodos << "\n" << flush;
}

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
                log_csv_f1("MEZCLADORA_CARGANDO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
            }

            this_thread::sleep_for(chrono::milliseconds(800));
            if (!system_running) break;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
                log_csv_f1("MEZCLADORA_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
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
                    log_csv_f1("HORNO_COCCION_INICIO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
                } else {
                    cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN.\n";
                    log_csv_f1("HORNO_FALTA_RECURSOS", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
                }
            }

            if (recursos_ok) {
                this_thread::sleep_for(chrono::milliseconds(3000));
                if (!system_running) break;

                {
                    lock_guard<mutex> lock(candado_planta);
                    estado.anodos_producidos++;
                    cout << "[Horno - PID: " << estado.pid_proceso << "] Ánodos listos: " << estado.anodos_producidos << "\n\n";
                    log_csv_f1("HORNO_ANODO_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
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
        log_csv_f1("INICIADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, mi_estado.anodos_producidos);

        thread t1(Hilo_Mezcladora, ref(mi_estado));
        thread t2(Hilo_Horno, ref(mi_estado));

        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();

        cout << "[Planta Carbón] Proceso finalizado.\n";
        log_csv_f1("FINALIZADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, mi_estado.anodos_producidos);
        if (csv_f1.is_open()) csv_f1.close();
        exit(0);
    }
}
