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
using namespace Industrial;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Candado exclusivo: Protege el archivo .log de las colisiones entre hilos internos
static mutex candado_log_f3;

/**
 * Manejador de señales interno para el proceso de Celdas.
 */
static void recibir_signal_apagado_f3(int sig)
{
    if (sig == SIGTERM)
    {
        system_running = 0;
    }
}

namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado, InventarioAmbiental &env, mutex &mtx, ofstream &log_file)
    {
        while (system_running)
        {
            float alumina_req = 200.0f;

            bool produccion_ok = false;

            // Bloqueo de exclusión mutua para asegurar que ningún otro hilo de este proceso
            // acceda al inventario o a la memoria compartida simultáneamente durante el consumo.
            {
                lock_guard<mutex> lock(mtx);

                // Verificación de recursos en Memoria Compartida (SHM)
                // Se requiere al menos un ánodo (producido en Fase 1) y 200kg alúmina (gestionada en Fase 2)
                if (env.alumina_enriquecida >= alumina_req && shared_planta->anodos_producidos >= 1)
                {
                    env.alumina_enriquecida -= alumina_req;
                    shared_planta->anodos_producidos--; // Consumo directo de SHM
                    produccion_ok = true;

                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] CONSUMIDA Alúmina Enriquecida y Ánodo de SHM.\n" << flush;
                }
                else if (shared_planta->tolvas_celdas[mi_estado.id_celda - 1] >= alumina_req && shared_planta->anodos_producidos >= 1)
                {
                    shared_planta->tolvas_celdas[mi_estado.id_celda - 1] -= alumina_req; // Resta directamente de la tolva en RAM real
                    shared_planta->anodos_producidos--; // Consumo de ánodo de SHM
                    produccion_ok = true;
                }
            }

            if (produccion_ok)
            {
                mi_estado.aluminio_producido += 100.0f;

                // Generar gases
                {
                    lock_guard<mutex> lock(mtx);
                    env.gases_acumulados += 50.0f;
                }

                lock_guard<mutex> log_lock(candado_log_f3);
                log_file << "[Celda #" << mi_estado.id_celda << "] REDUCCIÓN OK | Al: " << mi_estado.aluminio_producido << "kg | Gases +50kg\n" << flush;
            }
            else
            {
                lock_guard<mutex> log_lock(candado_log_f3);
                log_file << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS.\n" << flush;
            }
            this_thread::sleep_for(chrono::milliseconds(2500));
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas)
    {
        signal(SIGTERM, recibir_signal_apagado_f3);

        // Apertura del flujo de archivo truncando registros previos
        ofstream log_file("logs/fase3_reduccion.log", ios::out | ios::trunc);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo crear el archivo de log de la Fase 3." << endl;
            exit(1);
        }

        log_file << "=== INICIALIZACIÓN DE LOG DE REDUCCIÓN (FASE 3) ===\n" << flush;

        InventarioAmbiental inventario_ambiental;
        mutex candado_inventario;
        vector<thread> hilos;
        vector<EstadoCelda> celdas(cantidad_celdas);

        cout << "\n>>> [PROCESO] INICIANDO FASE 3 (RED) Y FASE 4 (GTC) - PID: " << getpid() << " <<<\n\n";

        // Lanzar GTC
        hilos.push_back(thread(fase_reciclaje_gtc, ref(inventario_ambiental), ref(candado_inventario)));

        // Lanzar Celdas
        for (int i = 0; i < cantidad_celdas; ++i)
        {
            celdas[i] = {i + 1, 958.0f, shared_planta->tolvas_celdas[i], 200.0f, 0.0f};
            hilos.push_back(thread(Hilo_Celda, ref(celdas[i]), ref(inventario_ambiental), ref(candado_inventario), ref(log_file)));
        }

        for (auto &h : hilos)
            if (h.joinable())
                h.join();

        log_file << "[Celdas Reducción] Proceso global finalizado.\n" << flush;
        log_file.close();
        exit(0);
    }
}
