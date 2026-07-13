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
        bool is_empty = false;
        {
            ifstream check("logs/fase4_gtc.csv");
            if (!check || check.peek() == ifstream::traits_type::eof()) {
                is_empty = true;
            }
        }
        csv_f4.open("logs/fase4_gtc.csv", ios::out | ios::app);
        if (is_empty) {
            csv_f4 << "timestamp_ms,evento,gases_capturados,alumina_enriquecida_generada\n" << flush;
        }
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f4).count();
    csv_f4 << elapsed << "," << event << "," << gases << "," << alumina_gen << "\n" << flush;
}

namespace Industrial
{
    void fase_reciclaje_gtc(mutex &mtx)
    {
        // !!! bucle while(system_running) ELIMINADO !!!
        ofstream log_file("logs/fase4_gtc.log", ios::out | ios::app);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo abrir el archivo de log de la Fase 4." << endl;
            return;
        }

        log_csv_f4("INICIADO", 0.0f, 0.0f);
        {
            lock_guard<mutex> log_lock(candado_log_f4);
            log_file << "[GTC - Garbage Collector] Escaneando el registro de gases residuales globales en la memoria...\n" << flush;
        }

        float gases_capturados = 0.0f;

        // Sección Crítica: Captura atómica de gases y conversión ecológica en RAM global
        {
            lock_guard<mutex> lock(mtx);
            
            if (shared_planta != nullptr && shared_planta->gases_acumulados >= 100.0f) {
                // 1. Extraemos los gases residuales
                gases_capturados = shared_planta->gases_acumulados;
                
                // 2. Liberación/Limpieza del espacio virtual ("Garbage Collection" ambiental)
                shared_planta->gases_acumulados = 0.0f;
                
                // 3. Regeneración química: Escribimos el recurso reciclado en la SHM global
                shared_planta->alumina_enriquecida += (gases_capturados * 0.5f);
            }
        }

        if (gases_capturados > 0) {
            lock_guard<mutex> log_lock(candado_log_f4);
            log_file << "[GTC] [ALERTA] Nivel crítico de emisiones detectado (" << gases_capturados << " kg). Iniciando ciclo atómico de lavado químico...\n"
                     << "[GTC] [MEMORIA] Registro 'gases_acumulados' purgado a 0.0f en la RAM real. Espacio virtual liberado.\n"
                     << "[GTC] [RECICLAJE] Materia prima regenerada: +" << (gases_capturados * 0.5f) << " kg de Alúmina Enriquecida escrita en RAM global. [Total SHM Eco: " << shared_planta->alumina_enriquecida << "]\n"
                     << "---------------------------------------------------------\n" << flush;
            
            cout << "[GTC] ¡Ciclo de limpieza completado! Gases purgados. Nueva Alúmina Enriquecida: " << shared_planta->alumina_enriquecida << " kg\n";
            log_csv_f4("CAPTURA_GASES", gases_capturados, gases_capturados * 0.5f);
        }

        log_csv_f4("FINALIZADO", 0.0f, 0.0f);
        log_file.close();
        if (csv_f4.is_open()) csv_f4.close();
    }
}
