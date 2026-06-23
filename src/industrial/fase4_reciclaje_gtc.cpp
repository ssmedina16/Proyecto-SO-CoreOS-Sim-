#include "../../include/industrial/fases_produccion.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <signal.h>
#include <mutex>

using namespace std;
using namespace Industrial;



namespace Industrial
{
    void simular_reciclaje_gtc(mutex &mtx)
    {
        if (shared_planta == nullptr) return;

        cout << "[GTC - Garbage Collector] Escaneando el registro de gases residuales globales en la memoria...\n";

        float gases_capturados = 0.0f;

        {
            lock_guard<mutex> lock(mtx);
            
            if (shared_planta->gases_acumulados >= 100.0f)
            {
                gases_capturados = shared_planta->gases_acumulados;
                shared_planta->gases_acumulados = 0.0f;
                shared_planta->alumina_enriquecida += (gases_capturados * 0.5f);
            }
        }

        if (gases_capturados > 0)
        {
            cout << "[GTC] [ALERTA] Nivel crítico de emisiones detectado (" << gases_capturados << " kg). Iniciando ciclo atómico de lavado químico...\n"
                 << "[GTC] [MEMORIA] Registro 'gases_acumulados' purgado a 0.0f en la RAM real. Espacio virtual liberado.\n"
                 << "[GTC] [RECICLAJE] Materia prima regenerada: +" << (gases_capturados * 0.5f) << " kg de Alúmina Enriquecida escrita en RAM global. [Total SHM Eco: " << shared_planta->alumina_enriquecida << "]\n"
                 << "---------------------------------------------------------\n";
        }
    }
}
