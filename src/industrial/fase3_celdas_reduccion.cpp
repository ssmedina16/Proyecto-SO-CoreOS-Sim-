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


// Candado exclusivo: Protege el archivo .log de las colisiones entre hilos internos
static mutex candado_log_f3;


namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado, mutex &mtx, ofstream &log_file)
    {
        // !!! bucle while(system_running) ELIMINADO !!!
        float alumina_req = 200.0f;
        bool produccion_ok = false;

        // Bloqueo de exclusión mutua para acceder de forma segura a la memoria compartida virtual (SHM)
        {
            lock_guard<mutex> lock(mtx);

            if (shared_planta != nullptr) {
                {
                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] [SHM Read] Leyendo memoria compartida -> Ánodos listos: " 
                             << shared_planta->anodos_producidos << " | Alúmina Eco: " 
                             << shared_planta->alumina_enriquecida << " kg\n" << flush;
                }

                // Prioridad Verde: Consumir primero la Alúmina Enriquecida del GTC (Fase 4)
                if (shared_planta->alumina_enriquecida >= alumina_req && shared_planta->anodos_producidos >= 1) {
                    shared_planta->alumina_enriquecida -= alumina_req;
                    shared_planta->anodos_producidos--;
                    produccion_ok = true;
                    
                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] [RECURSO ECO] Alúmina Enriquecida consumida con prioridad verde desde la RAM.\n" << flush;
                }
                // Opción normal: Consumir de las tolvas locales alimentadas por la Fase 2
                else if (shared_planta->tolvas_celdas[mi_estado.id_celda - 1] >= alumina_req && shared_planta->anodos_producidos >= 1) {
                    shared_planta->tolvas_celdas[mi_estado.id_celda - 1] -= alumina_req;
                    shared_planta->anodos_producidos--;
                    produccion_ok = true;
                }
            }
        }

        if (produccion_ok) {
            // Inyectamos gases residuales resultantes del proceso químico en la SHM global
            {
                lock_guard<mutex> lock(mtx);
                if (shared_planta != nullptr) {
                    shared_planta->gases_acumulados += 50.0f;
                    shared_planta->aluminio_producido[mi_estado.id_celda - 1] += 100.0f;
                    mi_estado.aluminio_producido = shared_planta->aluminio_producido[mi_estado.id_celda - 1];
                } else {
                    mi_estado.aluminio_producido += 100.0f;
                }
            }

            lock_guard<mutex> log_lock(candado_log_f3);
            log_file << "[Celda #" << mi_estado.id_celda << "] >>> NÚCLEO DE ELECTRÓLISIS ACTIVO <<< | Reducción Exitosa | Ánodo Consumido | Aluminio Líquido Acumulado: " 
                     << mi_estado.aluminio_producido << " kg | [SHM Gases Inyectados: +50kg]\n"
                     << "---------------------------------------------------------\n" << flush;
            
            cout << "[Celda #" << mi_estado.id_celda << "] Reducción de aluminio completada con éxito.\n";
        } else {
            lock_guard<mutex> log_lock(candado_log_f3);
            log_file << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS EN SHM (Ánodos o Alúmina).\n" << flush;
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas)
    {
        // !!! exit(0) ELIMINADO !!!
        ofstream log_file("logs/fase3_reduccion.log", ios::out | ios::app);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo abrir el archivo de log de la Fase 3." << endl;
            return;
        }

        // Vector estático para que el estado térmico y el aluminio acumulado de las celdas persistan entre llamadas de la CPU
        static vector<EstadoCelda> celdas = {
            {1, 958.0f, 400.0f, 100.0f, 0.0f},
            {2, 958.0f, 400.0f, 100.0f, 0.0f},
            {3, 958.0f, 400.0f, 100.0f, 0.0f},
            {4, 958.0f, 400.0f, 100.0f, 0.0f},
            {5, 958.0f, 400.0f, 100.0f, 0.0f}
        };

        static mutex mtx_inventario_local;

        // Se ejecuta una sola ráfaga de electrólisis para cada celda de forma controlada y segura
        for (int i = 0; i < cantidad_celdas; ++i) {
            Hilo_Celda(celdas[i], mtx_inventario_local, log_file);
        }

        log_file.close();
    }
}
