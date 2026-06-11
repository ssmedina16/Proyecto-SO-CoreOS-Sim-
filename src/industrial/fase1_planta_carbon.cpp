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
            candado_planta.lock();
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Cargando materias primas...\n";
            candado_planta.unlock();

            this_thread::sleep_for(chrono::milliseconds(800));
            if (!system_running) break;

            candado_planta.lock();
            cout << "[Mezcladora - PID: " << estado.pid_proceso << "] Masa preparada.\n\n";
            candado_planta.unlock();
            this_thread::sleep_for(chrono::milliseconds(200));
        }
    }

    void Hilo_Horno(Industrial::EstadoPlanta &estado) {
        while (system_running) {
            candado_planta.lock();
            cout << "[Horno - PID: " << estado.pid_proceso << "] Iniciando ciclo de cocción (3s)....\n";
            candado_planta.unlock();

            this_thread::sleep_for(chrono::milliseconds(3000));
            if (!system_running) break;

            estado.anodos_producidos++;
            candado_planta.lock();
            cout << "[Horno - PID: " << estado.pid_proceso << "] Ánodos listos: " << estado.anodos_producidos << "\n\n";
            candado_planta.unlock();
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
