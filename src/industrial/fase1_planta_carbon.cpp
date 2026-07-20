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
        bool is_empty = false;
        {
            ifstream check("logs/fase1_planta.csv");
            if (!check || check.peek() == ifstream::traits_type::eof()) {
                is_empty = true;
            }
        }
        csv_f1.open("logs/fase1_planta.csv", ios::out | ios::app);
        if (is_empty) {
            csv_f1 << "timestamp_ms,evento,pid,coque_kg,brea_kg,anodos_producidos\n" << flush;
        }
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f1).count();
    csv_f1 << elapsed << "," << event << "," << pid << "," << coque << "," << brea << "," << anodos << "\n" << flush;
}
namespace Industrial {
    void Hilo_Mezcladora(Industrial::EstadoPlanta &estado) {
        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
            log_csv_f1("MEZCLADORA_CARGANDO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
        }

        this_thread::sleep_for(chrono::milliseconds(150));

        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
            log_csv_f1("MEZCLADORA_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    void Hilo_Horno(Industrial::EstadoPlanta &estado) {
        float coque_req = 40.0f;
        float brea_req = 10.0f;

        if (estado.coque_kg >= coque_req && estado.brea_kg >= brea_req) {
            estado.coque_kg -= coque_req;
            estado.brea_kg -= brea_req;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "[Horno - PID: " << estado.pid_proceso << "] [Materia Prima] Consumiendo coque y brea del inventario local para el ciclo térmico.\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] Iniciando ciclo de cocción en horno (Espera activa de CPU virtual).\n";
                int total_shm_anodos = shared_planta ? shared_planta->anodos_producidos : estado.anodos_producidos;
                log_csv_f1("HORNO_COCCION_INICIO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, total_shm_anodos);
            }

            this_thread::sleep_for(chrono::milliseconds(150));

            if (shared_planta != nullptr) {
                shared_planta->anodos_producidos += 4;
            }
            estado.anodos_producidos += 4;
            int total_shm = shared_planta ? shared_planta->anodos_producidos : estado.anodos_producidos;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "---------------------------------------------------------\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] [MEMORIA] ¡4 Ánodos Cocidos generados con éxito! Inyectando recursos en RAM global. [Total SHM: " 
                     << total_shm << "]\n";
                log_csv_f1("HORNO_ANODO_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, total_shm);
            }
        } else {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN. Reabasteciendo inventario local...\n";
            estado.coque_kg += 1000.0f;
            estado.brea_kg += 300.0f;
            int total_shm = shared_planta ? shared_planta->anodos_producidos : estado.anodos_producidos;
            log_csv_f1("HORNO_FALTA_RECURSOS", estado.pid_proceso, estado.coque_kg, estado.brea_kg, total_shm);
        }
    }

    void fase_planta_carbon() {
        static EstadoPlanta mi_estado = { getpid(), 2000.0f, 500.0f, 0 };
        int current_anodes = shared_planta ? shared_planta->anodos_producidos : mi_estado.anodos_producidos;

        cout << "\n>>> [PROCESO] INICIANDO FASE 1: PLANTA DE CARBÓN - PID: " << mi_estado.pid_proceso << " <<<\n\n";
        log_csv_f1("INICIADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, current_anodes);

        Hilo_Mezcladora(mi_estado);
        Hilo_Horno(mi_estado);

        current_anodes = shared_planta ? shared_planta->anodos_producidos : mi_estado.anodos_producidos;
        cout << "[Planta Carbón] Proceso finalizado.\n";
        log_csv_f1("FINALIZADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, current_anodes);
    }
}