#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <mutex>

using namespace std;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Candado para la consola dentro de este proceso
static mutex candado_consola;

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

    void Hilo_Celda(EstadoCelda &mi_estado, InventarioAmbiental &env, mutex &mtx)
    {
        while (system_running)
        {
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

                    candado_consola.lock();
                    cout << "[Celda #" << mi_estado.id_celda << "] CONSUMIDA Alúmina Enriquecida.\n";
                    candado_consola.unlock();
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
                mi_estado.aluminio_producido += 100.0f;

                // Generar gases
                {
                    lock_guard<mutex> lock(mtx);
                    env.gases_acumulados += 50.0f;
                }

                candado_consola.lock();
                cout << "[Celda #" << mi_estado.id_celda << "] REDUCCIÓN OK | Al: " << mi_estado.aluminio_producido << "kg | Gases +50kg\n";
                candado_consola.unlock();
            }
            else
            {
                candado_consola.lock();
                cout << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS.\n";
                candado_consola.unlock();
            }
            this_thread::sleep_for(chrono::milliseconds(2500));
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas)
    {
        signal(SIGTERM, recibir_signal_apagado_f3);

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
            celdas[i] = {i + 1, 958.0f, 1000.0f, 200.0f, 0.0f};
            hilos.push_back(thread(Hilo_Celda, ref(celdas[i]), ref(inventario_ambiental), ref(candado_inventario)));
        }

        for (auto &h : hilos)
            if (h.joinable())
                h.join();

        cout << "[Celdas Reducción] Proceso finalizado.\n";
        exit(0);
    }
}
