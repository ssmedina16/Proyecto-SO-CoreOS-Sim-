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

static void log_csv_f3(const string &event, int celda_id, float temp, float alumina, float anodo_carbon, float aluminio, float gases, float eco_stock) {
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
            csv_f3 << "timestamp_ms,evento,celda_id,temperatura_bano,alumina_kg,anodo_carbon_kg,aluminio_producido,gases_acumulados,alumina_enriquecida_stock\n" << flush;
        }
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f3).count();
    csv_f3 << elapsed << "," << event << "," << celda_id << "," << temp << "," << alumina << "," << anodo_carbon << "," << aluminio << "," << gases << "," << eco_stock << "\n" << flush;
}

namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado, mutex &mtx, ofstream &log_file)
    {
        float alumina_req = 200.0f;
        float carbon_por_ciclo = 10.0f;   // Desgaste del ánodo por ciclo de reducción
        float carbon_min_alarma = 45.0f;  // Umbral de reposición del ánodo
        bool produccion_ok = false;
        bool consumida_enriquecida = false;

        {
            lock_guard<mutex> lock(mtx);

            if (shared_planta != nullptr) {
                {
                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] [SHM Read] Leyendo memoria compartida -> Ánodos listos: " 
                             << shared_planta->anodos_producidos << " | Alúmina Eco: " 
                             << shared_planta->alumina_enriquecida << " kg | Carbón local: "
                             << mi_estado.anodo_carbon_kg << " kg\n" << flush;
                }

                // Reposición del ánodo local: si cae por debajo del umbral de alarma,
                // consumir 1 ánodo global del inventario para reemplazar el ánodo desgastado
                if (mi_estado.anodo_carbon_kg < carbon_min_alarma && shared_planta->anodos_producidos >= 1) {
                    shared_planta->anodos_producidos--;
                    mi_estado.anodo_carbon_kg = 200.0f;
                    lock_guard<mutex> log_lock(candado_log_f3);
                    log_file << "[Celda #" << mi_estado.id_celda << "] [ANODO REPUESTO] Carbón bajo (< "
                             << carbon_min_alarma << " kg). Consumiendo 1 ánodo del stock global. "
                             << "Carbón local reiniciado a 200 kg. Stock global restante: "
                             << shared_planta->anodos_producidos << "\n" << flush;
                }

                // La celda puede producir si tiene suficiente carbón local y alumina disponible
                if (mi_estado.anodo_carbon_kg >= carbon_por_ciclo) {
                    float eco_disponible = shared_planta->alumina_enriquecida;
                    float tolva_disponible = shared_planta->tolvas_celdas[mi_estado.id_celda - 1];

                    if (eco_disponible + tolva_disponible >= alumina_req) {
                        float tomar_eco = std::min(eco_disponible, alumina_req);
                        float tomar_tolva = alumina_req - tomar_eco;

                        shared_planta->alumina_enriquecida -= tomar_eco;
                        shared_planta->tolvas_celdas[mi_estado.id_celda - 1] -= tomar_tolva;

                        // Consumir carbón del ánodo local (desgaste por ciclo)
                        mi_estado.anodo_carbon_kg -= carbon_por_ciclo;
                        produccion_ok = true;
                        if (tomar_eco > 0) consumida_enriquecida = true;

                        lock_guard<mutex> log_lock(candado_log_f3);
                        log_file << "[Celda #" << mi_estado.id_celda << "] [RECURSO CONSUMIDO] Alúmina: "
                                 << tomar_eco << " kg Enriquecida (GTC) + " << tomar_tolva
                                 << " kg Tolva. Carbón local: " << mi_estado.anodo_carbon_kg << " kg.\n" << flush;
                    }
                }
            }
        }

        if (produccion_ok) {
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
            log_file << "[Celda #" << mi_estado.id_celda << "] >>> NÚCLEO DE ELECTRÓLISIS ACTIVO <<< | Reducción Exitosa | Carbón: "
                     << mi_estado.anodo_carbon_kg << " kg | Aluminio Líquido Acumulado: " 
                     << mi_estado.aluminio_producido << " kg | [SHM Gases Inyectados: +50kg]\n"
                     << "---------------------------------------------------------\n" << flush;
            
            cout << "[Celda #" << mi_estado.id_celda << "] Reducción de aluminio completada con éxito.\n";

            float alumina_actual = 0.0f;
            float eco_stock = shared_planta ? shared_planta->alumina_enriquecida : 0.0f;
            if (shared_planta != nullptr) {
                alumina_actual = shared_planta->tolvas_celdas[mi_estado.id_celda - 1];
            }
            // Reportar carbón LOCAL de la celda (no el stock global de ánodos)
            log_csv_f3(consumida_enriquecida ? "REDUCCION_ENRIQUECIDA_OK" : "REDUCCION_OK", 
                       mi_estado.id_celda, mi_estado.temperatura_bano, alumina_actual, 
                       mi_estado.anodo_carbon_kg, mi_estado.aluminio_producido, gases_actuales, eco_stock);
        } else {
            lock_guard<mutex> log_lock(candado_log_f3);
            log_file << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS (Alúmina insuficiente o carbón agotado).\n" << flush;

            float alumina_actual = 0.0f;
            float gases_actuales = 0.0f;
            float eco_stock = shared_planta ? shared_planta->alumina_enriquecida : 0.0f;
            if (shared_planta != nullptr) {
                alumina_actual = shared_planta->tolvas_celdas[mi_estado.id_celda - 1];
                gases_actuales = shared_planta->gases_acumulados;
            }
            // Reportar carbón LOCAL de la celda (no el stock global de ánodos)
            log_csv_f3("FALTA_RECURSOS", mi_estado.id_celda, mi_estado.temperatura_bano, 
                       alumina_actual, mi_estado.anodo_carbon_kg, mi_estado.aluminio_producido, gases_actuales, eco_stock);
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas)
    {
        ofstream log_file("logs/fase3_reduccion.log", ios::out | ios::app);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo abrir el archivo de log de la Fase 3." << endl;
            return;
        }

        float eco_stock = shared_planta ? shared_planta->alumina_enriquecida : 0.0f;
        log_csv_f3("INICIADO", 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, eco_stock);

        // anodo_carbon_kg inicia en 200.0f para coincidir con el estado inicial del frontend
        static vector<EstadoCelda> celdas = {
            {1, 958.0f, 400.0f, 200.0f, 0.0f},
            {2, 958.0f, 400.0f, 200.0f, 0.0f},
            {3, 958.0f, 400.0f, 200.0f, 0.0f},
            {4, 958.0f, 400.0f, 200.0f, 0.0f},
            {5, 958.0f, 400.0f, 200.0f, 0.0f}
        };

        static mutex mtx_inventario_local;
        static int celda_inicio = 0;

        for (int step = 0; step < cantidad_celdas; ++step) {
            int idx = (celda_inicio + step) % cantidad_celdas;
            Hilo_Celda(celdas[idx], mtx_inventario_local, log_file);
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        celda_inicio = (celda_inicio + 1) % cantidad_celdas;

        eco_stock = shared_planta ? shared_planta->alumina_enriquecida : 0.0f;
        log_csv_f3("FINALIZADO", 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, eco_stock);
        log_file.close();
    }
}
