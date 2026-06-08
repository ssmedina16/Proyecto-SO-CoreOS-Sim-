#include <iostream>
#include <thread>
#include "../include/industrial/fases_produccion.hpp"

// Variables de control globales
bool system_running = true;

int main() {
    std::cout << "Iniciando CoreOS-Sim..." << std::endl;

    // Lanzar el hilo de la Planta de Carbón (Módulo 1)
    std::thread hilo_carbon(Industrial::fase_planta_carbon);

    // Simular tiempo de ejecución del sistema para validar concurrencia
    std::cout << "[Main] Sistema operando. Esperando 10 segundos..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Desactivar el sistema
    std::cout << "[Main] Apagando sistema..." << std::endl;
    system_running = false;

    // Asegurar el cierre correcto del hilo gestor
    if (hilo_carbon.joinable()) {
        hilo_carbon.join();
    }

    std::cout << "Sistema finalizado." << std::endl;
    return 0;
}