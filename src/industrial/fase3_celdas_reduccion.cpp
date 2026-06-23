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



namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado, mutex &mtx)
    {
        if (shared_planta == nullptr) return;

        float alumina_req = 200.0f;
        bool produccion_ok = false;

        {
            lock_guard<mutex> lock(mtx);
            cout << "[Celda #" << mi_estado.id_celda << "] [SHM Read] Leyendo memoria compartida -> Ánodos listos: " << shared_planta->anodos_producidos << " | Alúmina Eco: " << shared_planta->alumina_enriquecida << " kg\n";

            if (shared_planta->alumina_enriquecida >= alumina_req && shared_planta->anodos_producidos >= 1)
            {
                shared_planta->alumina_enriquecida -= alumina_req;
                shared_planta->anodos_producidos--; 
                produccion_ok = true;

                cout << "[Celda #" << mi_estado.id_celda << "] [RECURSO ECO] Alúmina Enriquecida del GTC detectada en RAM. Consumiendo con prioridad verde.\n";
            }
            else if (shared_planta->tolvas_celdas[mi_estado.id_celda - 1] >= alumina_req && shared_planta->anodos_producidos >= 1)
            {
                shared_planta->tolvas_celdas[mi_estado.id_celda - 1] -= alumina_req; 
                shared_planta->anodos_producidos--; 
                produccion_ok = true;
            }

            if (produccion_ok)
            {
                mi_estado.aluminio_producido += 100.0f;
                shared_planta->gases_acumulados += 50.0f;

                cout << "[Celda #" << mi_estado.id_celda << "] >>> NÚCLEO DE ELECTRÓLISIS ACTIVO <<< | Reducción Exitosa | Ánodo Consumido | Aluminio Líquido Acumulado: " << mi_estado.aluminio_producido << " kg | [SHM Gases Inyectados: +50kg]\n"
                     << "---------------------------------------------------------\n";
            }
            else
            {
                cout << "[Celda #" << mi_estado.id_celda << "] FALTAN RECURSOS.\n";
            }
        }
    }
}


