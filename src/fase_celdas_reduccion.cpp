#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <mutex> // NUEVO: Librería de Exclusión Mutua

#include "../include/industrial/fases_produccion.hpp"

using namespace std;

// Candado global para proteger la consola
mutex candado_consola;

// Traemos la variable global declarada en el main
extern volatile sig_atomic_t system_running;

namespace Industrial
{

    void Hilo_Celda(EstadoCelda &mi_estado)
    {
        while (system_running)
        {
            // VERIFICACIÓN DE RECURSOS (Simulando 1 ciclo = Producción de 0.1 Toneladas)
            // Según Venalum: 1 Tonelada requiere ~2000kg Alúmina y ~450kg Ánodo.
            // Para producir 0.1 Ton (100kg), necesitamos 200kg Alúmina y 45kg Ánodo.
            float alumina_requerida = 200.0f;
            float anodo_requerido = 45.0f;

            if (mi_estado.alumina_kg >= alumina_requerida && mi_estado.anodo_carbon_kg >= anodo_requerido)
            {
                // Hay recursos: Procedemos con la reducción
                mi_estado.alumina_kg -= alumina_requerida;
                mi_estado.anodo_carbon_kg -= anodo_requerido;
                mi_estado.aluminio_producido += 100.0f; // 0.1 Toneladas

                candado_consola.lock();

                cout << "[Celda #" << mi_estado.id_celda
                     << "] REDUCCIÓN OK -> Temp: " << mi_estado.temperatura_bano
                     << "°C | Aluminio acumulado: " << mi_estado.aluminio_producido << "kg\n\n";

                candado_consola.unlock();
            }
            else
            {
                candado_consola.lock();

                // Faltan recursos: Estado de Inanición
                cout << "[Celda #" << mi_estado.id_celda
                     << "] ¡ALERTA! Recursos insuficientes. Esperando logística...\n\n";

                candado_consola.unlock();
            }

            // Retardo de 2.5 segundos
            // para simular el tiempo de cocción
            this_thread::sleep_for(chrono::milliseconds(2500));
        }
    }

    void fase_celdas_reduccion(int cantidad_celdas)
    {
        vector<thread> lista_hilos;
        vector<EstadoCelda> memoria_celdas(cantidad_celdas);

        cout << "\n>>> INICIANDO LÍNEA ELECTROLÍTICA CON " << cantidad_celdas << " CELDAS <<<\n\n";

        for (int i = 0; i < cantidad_celdas; ++i)
        {
            memoria_celdas[i] = {
                i + 1,   // id_celda
                958.0f,  // temperatura_bano
                1000.0f, // alumina_kg
                200.0f,  // anodo_carbon_kg
                0.0f     // aluminio_producido
            };

            lista_hilos.push_back(thread(Hilo_Celda, ref(memoria_celdas[i])));
        }

        for (auto &hilo : lista_hilos)
        {
            if (hilo.joinable())
            {
                hilo.join();
            }
        }
    }
}