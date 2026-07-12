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

// Recursos y funciones de registro CSV para Fase 3
static ofstream csv_f3;
static mutex csv_mutex_f3;
static auto start_time_f3 = chrono::steady_clock::now();

static void log_csv_f3(const string &event, int celda_id, float temp, float alumina, float anodo_carbon, float aluminio, float gases) {
    lock_guard<mutex> lock(csv_mutex_f3);
    if (!csv_f3.is_open()) {
        bool is_empty = false;
        {
            ifstream check("logs/fase3_reduccion.csv");
            if (!check || check.peek() == ifstream::traits_type::eof()) {
                is_empty = true;
            }
        }
        csv_f3.open("logs/fase3_reduccion.csv", ios::out | ios::app);
        if (is_empty) {
            csv_f3 << "timestamp_ms,evento,celda_id,temperatura_bano,alumina_kg,anodo_carbon_kg,aluminio_producido,gases_acumulados\n" << flush;
        }
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f3).count();
    csv_f3 << elapsed << "," << event << "," << celda_id << "," << temp << "," << alumina << "," << anodo_carbon << "," << aluminio << "," << gases << "\n" << flush;
}

namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado, mutex &mtx, ofstream &log_file)
    {
        // !!! bucle while(system_running) ELIMINADO !!!
        float alumina_req = 200.0f;
        bool produccion_ok = false;
        bool consumida_enriquecida = false;

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
                    consumida_enriquecida = true;
                    
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
            float gases_actuales = 0.0f;
            {
                lock_guard<mutex> lock(mtx);
                if (shared_planta != nullptr) {
                    shared_planta->gases_acumulados += 50.0f;
                    gases_actuales = shared_planta->gases_acumulados;
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

            float alumina_actual = 0.0f;
            if (shared_planta != nullptr) {
                alumina_actual = consumida_enriquecida ? shared_planta->alumina_enriquecida : shared_planta->tolvas_celdas[mi_estado.id_celda - 1];
            }
            log_csv_f3(consumida_enriquecida ? "REDUCCION_ENRIQUECIDA_OK" : "REDUCCION_OK", 
                       mi_estado.id_celda, mi_estado.temperatura_bano, alumina_actual, 
                       1.0f, mi_estado.aluminio_producido, gases_actuales);
        } else {
            lock_guard<mutex> log_lock(candado_log_f3);
            log_file << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS EN SHM (Ánodos o Alúmina).\n" << flush;

            float alumina_actual = 0.0f;
            float gases_actuales = 0.0f;
            if (shared_planta != nullptr) {
                alumina_actual = shared_planta->tolvas_celdas[mi_estado.id_celda - 1];
                gases_actuales = shared_planta->gases_acumulados;
            }
            log_csv_f3("FALTA_RECURSOS", mi_estado.id_celda, mi_estado.temperatura_bano, 
                       alumina_actual, 0.0f, mi_estado.aluminio_producido, gases_actuales);
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

        log_csv_f3("INICIADO", 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

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

        log_csv_f3("FINALIZADO", 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        log_file.close();
    }
}
