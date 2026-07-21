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

// Parámetros operativos (simulación interna)
static const int MAX_SOLICITUDES = 8;
static const int SLEEP_MS = 1500;

// Candado para logs dentro del proceso de logística
static mutex candado_logistica;

// Recursos y funciones de registro CSV para Fase 2
static ofstream csv_f2;
static mutex csv_mutex_f2;
static auto start_time_f2 = chrono::steady_clock::now();

static void log_csv_f2(const string &event, int celda_id, float cantidad, float silo_restante) {
    lock_guard<mutex> lock(csv_mutex_f2);
    if (!csv_f2.is_open()) {
        bool is_empty = false;
        {
            ifstream check("logs/fase2_logistica.csv");
            if (!check || check.peek() == ifstream::traits_type::eof()) {
                is_empty = true;
            }
        }
        csv_f2.open("logs/fase2_logistica.csv", ios::out | ios::app);
        if (is_empty) {
            csv_f2 << "timestamp_ms,evento,celda_id,cantidad_enviada,silo_alumina_restante\n" << flush;
        }
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start_time_f2).count();
    csv_f2 << elapsed << "," << event << "," << celda_id << "," << cantidad << "," << silo_restante << "\n" << flush;
}
namespace Industrial {

    void fase_logistica_transporte() {
        // !!! bucle while(system_running) ELIMINADO !!!
        // !!! exit(0) REMOVIDO COMPLETAMENTE !!!

        // Cambiado a ios::app para acumular las transferencias a lo largo de la simulación
        std::ofstream log_file("logs/fase2_logistica.log", std::ios::out | std::ios::app);
        if (!log_file.is_open()) {
            std::cerr << "Error Crítico: No se pudo abrir el archivo de log de Logística." << std::endl;
            return;
        }
        float silo_val = shared_planta ? shared_planta->silo_alumina : 0.0f;
        log_csv_f2("INICIADO", 0, 0.0f, silo_val);

        {
            lock_guard<mutex> lg(candado_logistica);
            cout << "[Logística] [DMA Scan] Escaneando mapa de tolvas en memoria compartida...\n";
        }

        if (shared_planta != nullptr) {
            // Auto-reabastecimiento del silo central de alúmina (Capacidad nominal: 50,000 kg; Umbral 20%: 10,000 kg)
            if (shared_planta->silo_alumina < 10000.0f) {
                float cantidad_reabastecida = 50000.0f - shared_planta->silo_alumina;
                shared_planta->silo_alumina = 50000.0f;
                {
                    lock_guard<mutex> lg(candado_logistica);
                    log_file << "[Logística] [AUTO-REABASTECIMIENTO] Nivel de silo crítico (< 20%). Inyectados "
                             << cantidad_reabastecida << " kg. [Silo SHM: 50000 kg]\n"
                             << "---------------------------------------------------------\n" << std::flush;
                    cout << "[Logística] [AUTO-REABASTECIMIENTO] Silo de alúmina reabastecido al 100% (50000 kg).\n";
                    log_csv_f2("REABASTECIMIENTO", 0, cantidad_reabastecida, shared_planta->silo_alumina);
                }
            }

            for (int i = 0; i < MAX_CELDAS; ++i) {
                if (shared_planta->tolvas_celdas[i] < 500.0f) {
                    float nivel_actual = shared_planta->tolvas_celdas[i];
                    float cantidad_a_enviar = 1000.0f - nivel_actual;

                    if (shared_planta->silo_alumina >= cantidad_a_enviar) {
                        shared_planta->silo_alumina -= cantidad_a_enviar;
                        shared_planta->tolvas_celdas[i] += cantidad_a_enviar;

                        {
                            lock_guard<mutex> lg(candado_logistica);
                            log_file << "[Logística] [ALERTA] Celda #" << (i + 1) << " requiere reabastecimiento. Nivel actual: " << nivel_actual << " kg (Umbral < 500kg).\n"
                                     << "[Logística] [IPC] Retirando recursos del Silo Central. [Silo SHM Restante: " << shared_planta->silo_alumina << " kg]\n"
                                     << "[Logística] [TRANSFERENCIA OK] Inyectando recursos en la dirección de memoria de la Celda #" << (i + 1) << ". [Tolva SHM: " << shared_planta->tolvas_celdas[i] << " kg]\n"
                                     << "---------------------------------------------------------\n" << std::flush;
                            
                            cout << "[Logística] [ALERTA] Inyectando recursos neumáticos en Celda #" << (i + 1) << "\n";
                            log_csv_f2("RELLENANDO", i + 1, cantidad_a_enviar, shared_planta ? shared_planta->silo_alumina : 0.0f);
                        }

                        // Simula el retardo atómico de la transferencia por este turno
                        this_thread::sleep_for(chrono::milliseconds(200));
                    }
                }
            }
        }

        log_file << "[Logística] Finalizando proceso de logística.\n" << std::flush;
        log_csv_f2("FINALIZADO", 0, 0.0f, shared_planta ? shared_planta->silo_alumina : 0.0f);
        log_file.close();
    }

} 
