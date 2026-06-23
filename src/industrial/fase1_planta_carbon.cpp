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



namespace Industrial {

    void Hilo_Mezcladora(Industrial::EstadoPlanta &estado) {
        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
        }

        this_thread::sleep_for(chrono::milliseconds(400));

        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
        }
    }

    void Hilo_Horno(Industrial::EstadoPlanta &estado) {
        bool recursos_ok = false;
        float coque_req = 50.0f;
        float brea_req = 12.0f;

        {
            lock_guard<mutex> lock(candado_planta);
            if (estado.coque_kg >= coque_req && estado.brea_kg >= brea_req) {
                estado.coque_kg -= coque_req;
                estado.brea_kg -= brea_req;
                recursos_ok = true;
                cout << "[Horno - PID: " << estado.pid_proceso << "] [Materia Prima] Consumiendo coque y brea del inventario local para el ciclo térmico.\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] Iniciando ciclo de cocción en horno (Espera activa de 3s).\n";
            } else {
                cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN.\n";
                estado.coque_kg += 150.0f; // Simulación de reabastecimiento
                estado.brea_kg += 40.0f;
            }
        }

        if (recursos_ok) {
            this_thread::sleep_for(chrono::milliseconds(800));

            {
                lock_guard<mutex> lock(candado_planta);
                if (shared_planta != nullptr) {
                    shared_planta->anodos_producidos++;
                }
                estado.anodos_producidos++;
                cout << "---------------------------------------------------------\n";
                cout << "[Horno - PID: " << estado.pid_proceso << "] [MEMORIA] ¡Ánodo Cocido generado con éxito! Inyectando recurso en RAM global. [Total SHM: " << (shared_planta ? shared_planta->anodos_producidos : 0) << "]\n";
            }
        }
    }
}


