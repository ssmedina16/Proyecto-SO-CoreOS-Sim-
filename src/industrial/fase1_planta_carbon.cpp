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
        // Ejecuta un único ciclo de carga y preparación de masa por cada activación de CPU
        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
        }

        this_thread::sleep_for(chrono::milliseconds(400)); // Retardo de simulación de ráfaga

        {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
        }
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
            }
        } else {
            lock_guard<mutex> lock(candado_planta);
            cout << "[Horno - PID: " << estado.pid_proceso << "] FALTAN MATERIAS PRIMAS EN LA PLANTA DE CARBÓN. Reabasteciendo inventario local...\n";
            estado.coque_kg += 150.0f;
            estado.brea_kg += 40.0f;
        }
    }

    void fase_planta_carbon() {
        // Usamos una variable estática para que el inventario local de la planta no se reinicie en cada turno
        static EstadoPlanta mi_estado = { getpid(), 500.0f, 120.0f, 0 };

        // Corre de manera secuencial y ordenada un paso de producción y devuelve el control al Kernel
        Hilo_Mezcladora(mi_estado);
        Hilo_Horno(mi_estado);
    }
}