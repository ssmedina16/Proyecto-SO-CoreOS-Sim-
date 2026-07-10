#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <mutex>

using namespace std;
using namespace Industrial;

extern volatile sig_atomic_t system_running;

// Candado exclusivo: Protege el archivo .log del GTC de las colisiones entre hilos
static mutex candado_log_f4;

// Recursos y funciones de registro CSV para Fase 4
static ofstream csv_f4;
static mutex csv_mutex_f4;
static auto start_time_f4 = chrono::steady_clock::now();

static void log_csv_f4(const string &event, float gases, float alumina_gen) {
    lock_guard<mutex> lock(csv_mutex_f4);
    if (!csv_f4.is_open()) {
        csv_f4.open("logs/fase4_gtc.csv", ios::out | ios::trunc);
        csv_f4 << "timestamp_ms,evento,gases_capturados,alumina_enriquecida_generada\n" << flush;
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f4).count();
    csv_f4 << elapsed << "," << event << "," << gases << "," << alumina_gen << "\n" << flush;
}

namespace Industrial
{
    void fase_reciclaje_gtc(InventarioAmbiental &env, mutex &mtx)
    {
        // Apertura del flujo de archivo truncando registros previos
        ofstream log_file("logs/fase4_gtc.log", ios::out | ios::trunc);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo crear el archivo de log de la Fase 4." << endl;
            exit(1);
        }

        log_file << "=== INICIALIZACIÓN DE LOG DE GTC (FASE 4) ===\n" << flush;
        log_csv_f4("INICIADO", 0.0f, 0.0f);

        while (system_running)
        {
            this_thread::sleep_for(chrono::seconds(5));

            float gases_capturados = 0.0f;

            {
                lock_guard<mutex> lock(mtx);
                if (env.gases_acumulados >= 100.0f)
                {
                    gases_capturados = env.gases_acumulados;
                    env.gases_acumulados = 0.0f;
                    env.alumina_enriquecida += (gases_capturados * 0.5f);
                }
            }

            if (gases_capturados > 0)
            {
                lock_guard<mutex> log_lock(candado_log_f4);
                log_file << "[GTC] Capturados " << gases_capturados
                         << "kg de gases. Alúmina Enriquecida generada: "
                         << (gases_capturados * 0.5f) << "kg.\n" << flush;
                log_csv_f4("CAPTURA_GASES", gases_capturados, gases_capturados * 0.5f);
            }
        }

        log_file << "[GTC] Proceso global finalizado.\n" << flush;
        log_csv_f4("FINALIZADO", 0.0f, 0.0f);
        
        log_file.close();
        if (csv_f4.is_open()) csv_f4.close();
    }
}
