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
        // Ejecuta un único ciclo de carga y preparación de masa por cada activación de CPU
        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
            log_csv_f1("MEZCLADORA_CARGANDO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
        }

        this_thread::sleep_for(chrono::milliseconds(400)); // Retardo de simulación de ráfaga

        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
            log_csv_f1("MEZCLADORA_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
        }
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    void Hilo_Horno(Industrial::EstadoPlanta &estado) {
        float coque_req = 50.0f;
        float brea_req = 12.0f;

        if (estado.coque_kg >= coque_req && estado.brea_kg >= brea_req) {
            estado.coque_kg -= coque_req;
            estado.brea_kg -= brea_req;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "[Horno - PID: " << estado.pid_proceso << "] [Materia Prima] Consumiendo coque y brea del inventario local para el ciclo térmico.\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] Iniciando ciclo de cocción en horno (Espera activa de CPU virtual).\n";
                log_csv_f1("HORNO_COCCION_INICIO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
            }

            this_thread::sleep_for(chrono::milliseconds(600)); // Tiempo de cocción por este cuanto

            if (shared_planta != nullptr) {
                shared_planta->anodos_producidos++;
            }
            estado.anodos_producidos++;

            {
                lock_guard<mutex> lock(candado_planta);
                cout << "---------------------------------------------------------\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] [MEMORIA] ¡Ánodo Cocido generado con éxito! Inyectando recurso en RAM global. [Total SHM: " 
                     << (shared_planta ? shared_planta->anodos_producidos : 0) << "]\n";
                log_csv_f1("HORNO_ANODO_LISTO", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
            }
        } else {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN. Reabasteciendo inventario local...\n";
            log_csv_f1("HORNO_FALTA_RECURSOS", estado.pid_proceso, estado.coque_kg, estado.brea_kg, estado.anodos_producidos);
            estado.coque_kg += 150.0f;
            estado.brea_kg += 40.0f;
        }
    }

    void fase_planta_carbon() {
        // Usamos una variable estática para que el inventario local de la planta no se reinicie en cada turno
        static EstadoPlanta mi_estado = { getpid(), 500.0f, 120.0f, 0 };

        cout << "\n>>> [PROCESO] INICIANDO FASE 1: PLANTA DE CARBÓN - PID: " << mi_estado.pid_proceso << " <<<\n\n";
        log_csv_f1("INICIADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, mi_estado.anodos_producidos);

        // Corre de manera secuencial y ordenada un paso de producción y devuelve el control al Kernel
        Hilo_Mezcladora(mi_estado);
        Hilo_Horno(mi_estado);

        cout << "[Planta Carbón] Proceso finalizado.\n";
        log_csv_f1("FINALIZADO", mi_estado.pid_proceso, mi_estado.coque_kg, mi_estado.brea_kg, mi_estado.anodos_producidos);
    }
}