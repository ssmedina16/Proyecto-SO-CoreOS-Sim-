#include <iostream>
#include <thread>
#include "../include/industrial/fases_produccion.hpp"

// Variables de control globales
bool system_running = true;

int main() {
    std::cout << "Iniciando CoreOS-Sim..." << std::endl;

    // TODO: Inicializar hilos de las fases industriales para el Paso 1
    // (Aquí es van a crear los hilos de la planta)

    // TODO: Asegurar el cierre correcto de los hilos con .join()
    // (Aquí esperaremos que los hilos terminen antes de que muera el main)

    std::cout << "Sistema finalizado." << std::endl;
    return 0;
}