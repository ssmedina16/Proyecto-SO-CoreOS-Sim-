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

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;



namespace Industrial {

    void simular_escaneo_tolvas() {
        if (shared_planta == nullptr) return;

        cout << "[Logística] [DMA Scan] Escaneando mapa de tolvas en memoria compartida...\n";
        
        for (int i = 0; i < MAX_CELDAS; ++i) {
            if (shared_planta->tolvas_celdas[i] < 500.0f) {
                float cantidad_a_enviar = 1000.0f - shared_planta->tolvas_celdas[i];

                if (shared_planta->silo_alumina >= cantidad_a_enviar) {
                    float nivel_actual = shared_planta->tolvas_celdas[i];
                    shared_planta->silo_alumina -= cantidad_a_enviar;
                    shared_planta->tolvas_celdas[i] += cantidad_a_enviar;

                    cout << "[Logística] [ALERTA] Celda #" << (i + 1) << " requiere reabastecimiento. Nivel actual: " << nivel_actual << " kg (Umbral < 500kg).\n"
                         << "[Logística] [IPC] Retirando recursos del Silo Central. [Silo SHM Restante: " << shared_planta->silo_alumina << " kg]\n"
                         << "[Logística] [TRANSFERENCIA OK] Inyectando recursos en la dirección de memoria de la Celda #" << (i + 1) << ". [Tolva SHM: " << shared_planta->tolvas_celdas[i] << " kg]\n"
                         << "---------------------------------------------------------\n";

                    // simular tiempo de transferencia neumática
                    this_thread::sleep_for(chrono::milliseconds(800));
                }
            }
        }
    }

} 
