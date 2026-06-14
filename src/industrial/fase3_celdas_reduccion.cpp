#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <mutex>
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Candado para la consola dentro de este proceso
static mutex candado_celdas;

/**
 * Manejador de señales interno para el proceso de Celdas.
 */
static void recibir_signal_apagado_f3(int sig) {
    if (sig == SIGTERM) {
        system_running = 0;
    }
}

namespace Industrial {

    void Hilo_Celda(EstadoCelda &mi_estado) {
        while (system_running) {
            float alumina_req = 200.0f;
            float anodo_req = 45.0f;

            if (mi_estado.alumina_kg >= alumina_req && mi_estado.anodo_carbon_kg >= anodo_req) {
                mi_estado.alumina_kg -= alumina_req;
                mi_estado.anodo_carbon_kg -= anodo_req;
                mi_estado.aluminio_producido += 100.0f;

                candado_celdas.lock();
                cout << "[Celda #" << mi_estado.id_celda << "] REDUCCIÓN OK | Al: " << mi_estado.aluminio_producido << "kg\n";
                candado_celdas.unlock();
            } else {
                candado_celdas.lock();
                cout << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS.\n";
                candado_celdas.unlock();
            }
            this_thread::sleep_for(chrono::milliseconds(2500));
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas) {
        signal(SIGTERM, recibir_signal_apagado_f3);
        
        // Obtener el segmento de memoria compartida
        key_t key = 12345;
        int shmid = shmget(key, sizeof(EstadoCelda) * cantidad_celdas, 0666);
        if (shmid < 0) {
            cerr << "[Celdas Reducción - Error] No se pudo conectar a la memoria compartida." << endl;
            exit(1);
        }
        EstadoCelda* celdas = (EstadoCelda*) shmat(shmid, nullptr, 0);

        cout << "\n>>> [PROCESO] INICIANDO FASE 3: CELDAS DE REDUCCIÓN - PID: " << getpid() << " <<<\n\n";

        vector<thread> hilos;
        for (int i = 0; i < cantidad_celdas; ++i) {
            hilos.push_back(thread(Hilo_Celda, ref(celdas[i])));
        }

        for (auto &h : hilos) if (h.joinable()) h.join();

        cout << "[Celdas Reducción] Proceso finalizado.\n";
        shmdt(celdas);
        exit(0);
    }
}
