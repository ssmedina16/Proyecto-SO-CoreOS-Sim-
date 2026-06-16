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
            // Sincronizar estado local con la tolva global
            mi_estado.alumina_kg = TOLVAS_CELDAS_GLOBAL[mi_estado.id_celda - 1];

            float alumina_req = 200.0f;
            float anodo_req = 45.0f;

            bool produccion_ok = false;

            // Bloqueo solo para verificar y consumir recursos
            {
                lock_guard<mutex> lock(mtx);

                // Priorizar alúmina enriquecida
                if (env.alumina_enriquecida >= alumina_req && mi_estado.anodo_carbon_kg >= anodo_req)
                {
                    env.alumina_enriquecida -= alumina_req;
                    mi_estado.anodo_carbon_kg -= anodo_req;
                    produccion_ok = true;

                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] CONSUMIDA Alúmina Enriquecida.\n" << flush;
                }
                else if (mi_estado.alumina_kg >= alumina_req && mi_estado.anodo_carbon_kg >= anodo_req)
                {
                    mi_estado.alumina_kg -= alumina_req;
                    mi_estado.anodo_carbon_kg -= anodo_req;
                    produccion_ok = true;
                }
            }

            if (produccion_ok)
            {
                // Guardar el remanente de alúmina en la tolva global
                TOLVAS_CELDAS_GLOBAL[mi_estado.id_celda - 1] = mi_estado.alumina_kg;

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
            celdas[i] = {i + 1, 958.0f, TOLVAS_CELDAS_GLOBAL[i], 200.0f, 0.0f};
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
