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

namespace Industrial
{
    void fase_reciclaje_gtc(mutex &mtx)
    {
        // Apertura del flujo de archivo truncando registros previos
        ofstream log_file("logs/fase4_gtc.log", ios::out | ios::trunc);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo crear el archivo de log de la Fase 4." << endl;
            exit(1);
        }

        log_file << "=== INICIALIZACIÓN DE LOG DE GTC (FASE 4) ===\n" << flush;

        while (system_running)
        {
            this_thread::sleep_for(chrono::seconds(5));

            float gases_capturados = 0.0f;

            {
                // Protegemos el acceso a la memoria compartida global (SHM)
                lock_guard<mutex> lock(mtx);
                
                // El recolector de basura (GC) verifica si hay suficientes residuos acumulados en la RAM
                if (shared_planta->gases_acumulados >= 100.0f)
                {
                    // 1. Extraemos los gases residuales de forma atómica
                    gases_capturados = shared_planta->gases_acumulados;
                    
                    // 2. Limpiamos el espacio en RAM ("Garbage Collection")
                    shared_planta->gases_acumulados = 0.0f;
                    
                    // 3. Transformamos (reciclamos) la basura en materia prima útil (alúmina) en la SHM global
                    shared_planta->alumina_enriquecida += (gases_capturados * 0.5f);
                }
            }

            if (gases_capturados > 0)
            {
                lock_guard<mutex> log_lock(candado_log_f4);
                log_file << "[GTC] Capturados " << gases_capturados
                         << "kg de gases. Alúmina Enriquecida generada: "
                         << (gases_capturados * 0.5f) << "kg.\n" << flush;
            }
        }

        log_file << "[GTC] Proceso global finalizado.\n" << flush;
        log_file.close();
    }
}
