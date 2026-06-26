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
        // !!! bucle while(system_running) ELIMINADO !!!
        ofstream log_file("logs/fase4_gtc.log", ios::out | ios::app);
        if (!log_file.is_open()) {
            cerr << "Error crítico: No se pudo abrir el archivo de log de la Fase 4." << endl;
            return;
        }

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
        }

        log_file.close();
    }
}
