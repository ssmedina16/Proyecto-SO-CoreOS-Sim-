#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>   // Requerido para fork() y pid_t
#include <signal.h>   // Requerido para señales del SO (kill, SIGTERM)
#include <sys/wait.h> // Requerido para waitpid()
#include "../include/industrial/fases_produccion.hpp"

using namespace std;

// Variables de control globales
volatile sig_atomic_t system_running = 1;

// --- MANEJADOR DE SEÑALES ---
// Esta función atrapa la orden de apagado del Kernel
void interceptar_apagado(int sig)
{
    if (sig == SIGTERM)
    {
        cout << "\n[Línea Electrolítica] Señal SIGTERM recibida. Deteniendo celdas...\n";
        system_running = 0; // Cambiamos la variable de control para que los hilos de las celdas terminen su ejecución
    }
}

int main()
{
    std::cout << "Iniciando CoreOS-Sim..." << std::endl;

    // Clonamos el programa creando un nuevo Proceso
    pid_t pid_celdasElectroliticas = fork();

    if (pid_celdasElectroliticas < 0)
    {
        cerr << "Error crítico: falló la llamada a fork()." << endl;
        return 1;
    }

    else if (pid_celdasElectroliticas == 0)
    {
        // ==========================================
        // CÓDIGO DEL PROCESO HIJO (Linea Electrolítica)
        // ==========================================

        // Le decimos al SO que si el padre nos envía un SIGTERM, ejecute la función 'interceptar_apagado'
        signal(SIGTERM, interceptar_apagado);

        Industrial::fase_celdas_reduccion(5);

        return 0; // Cuando la línea termine, el proceso hijo muere limpiamente.
    }
    else
    {
        // ==========================================
        // CÓDIGO DEL PROCESO PADRE (Kernel Administrador)
        // ==========================================

        // TODO: Inicializar hilos de las fases industriales para el Paso 1
        // (Aquí es van a crear los hilos de la planta)

        // TODO: Asegurar el cierre correcto de los hilos con .join()
        // (Aquí esperaremos que los hilos terminen antes de que muera el main)

        // Dejamos que el simulador corra por 10 segundos
        this_thread::sleep_for(chrono::seconds(15));

        cout << "\n[Kernel] Tiempo de simulación concluido. Apagando PID " << pid_celdasElectroliticas << "...\n";

        // Enviamos la orden de apagado al proceso hijo
        kill(pid_celdasElectroliticas, SIGTERM);

        // Esperamos a que el sistema operativo limpie los recursos del hijo (evita "Procesos Zombie")
        waitpid(pid_celdasElectroliticas, nullptr, 0);

        cout << "[Kernel] Sistema finalizado de manera segura." << endl;
        return 0;
    }
}