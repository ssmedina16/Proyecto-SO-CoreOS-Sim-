#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>   // Requerido para fork() y pid_t
#include <signal.h>   // Requerido para señales del SO (kill, SIGTERM)
#include <sys/wait.h> // Requerido para waitpid()
#include <sys/ipc.h>  // Requerido para IPC de memoria compartida
#include <sys/shm.h>  // Requerido para memoria compartida
#include "../include/industrial/fases_produccion.hpp"

using namespace std;

// Variables de control globales (utilizadas principalmente por el Padre)
volatile sig_atomic_t system_running = 1;

/**
 * Manejador genérico para el proceso padre (Kernel).
 */
void interceptar_apagado_padre(int sig)
{
    if (sig == SIGTERM || sig == SIGINT)
    {
        system_running = 0;
    }
}

int main()
{
    std::cout << ">>> [KERNEL] INICIANDO COREOS-SIM (ARQUITECTURA MULTIPROCESO) <<<\n" << std::endl;

    // Configuración de señal para el padre
    signal(SIGTERM, interceptar_apagado_padre);
    signal(SIGINT, interceptar_apagado_padre);

    // Inicializar segmento de memoria compartida para las celdas
    key_t key_celdas = 12345;
    int shmid = shmget(key_celdas, sizeof(Industrial::EstadoCelda) * 5, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        cerr << "[Kernel - Error] No se pudo crear el segmento de memoria compartida." << endl;
        return 1;
    }
    Industrial::EstadoCelda* celdas_compartidas = (Industrial::EstadoCelda*) shmat(shmid, nullptr, 0);

    // Inicializar las celdas con recursos suficientes para los 15s de simulación
    for (int i = 0; i < 5; ++i)
    {
        celdas_compartidas[i].id_celda = i + 1;
        celdas_compartidas[i].temperatura_bano = 958.0f;
        celdas_compartidas[i].alumina_kg = 2000.0f;       // Evita que se queden sin recursos antes de que actúe el trasiego
        celdas_compartidas[i].anodo_carbon_kg = 500.0f;    // Evita que se queden sin recursos
        celdas_compartidas[i].aluminio_producido = 0.0f;
    }

    // 1. LANZAMIENTO DE LA FASE 1: PLANTA DE CARBÓN (SANTIAGO)
    pid_t pid_plantaCarbon = fork();

    if (pid_plantaCarbon == 0) {
        // PROCESO HIJO: Planta de Carbón
        // El hijo configura sus propios handlers dentro de su .cpp
        Industrial::fase_planta_carbon();
        return 0; 
    }

    // 2. LANZAMIENTO DE LA FASE 3: CELDAS DE REDUCCIÓN (JP)
    pid_t pid_celdasElectroliticas = fork();

    if (pid_celdasElectroliticas == 0) {
        // PROCESO HIJO: Celdas de Reducción
        Industrial::fase_celdas_reduccion(5);
        return 0;
    }

    // 3. LANZAMIENTO DE LA FASE 5: EXTRACCIÓN (TRASIEGO) Y CRISOL (CHRISTIAN)
    pid_t pid_trasiegoCrisol = fork();

    if (pid_trasiegoCrisol == 0) {
        // PROCESO HIJO: Trasiego y Crisol
        // Inicializamos 2 crisoles para vaciar las 5 celdas electrolíticas
        Industrial::fase_trasiego_crisol(2, nullptr, 5);
        return 0;
    }

    // ===============================================
    // CÓDIGO DEL PROCESO PADRE (Kernel Administrador)
    // ===============================================

    // El Kernel espera 15 segundos simulando la jornada industrial
    this_thread::sleep_for(chrono::seconds(15));

    cout << "\n[Kernel] Tiempo de simulación concluido. Enviando señales de apagado...\n";

    // Enviamos SIGTERM a los hijos para que sus handlers internos actúen de forma autónoma
    if (pid_plantaCarbon > 0) kill(pid_plantaCarbon, SIGTERM);
    if (pid_celdasElectroliticas > 0) kill(pid_celdasElectroliticas, SIGTERM);
    if (pid_trasiegoCrisol > 0) kill(pid_trasiegoCrisol, SIGTERM);

    // Limpieza de procesos (evitar zombies)
    waitpid(pid_plantaCarbon, nullptr, 0);
    waitpid(pid_celdasElectroliticas, nullptr, 0);
    waitpid(pid_trasiegoCrisol, nullptr, 0);

    // Separar de la memoria compartida y destruirla en el padre
    shmdt(celdas_compartidas);
    shmctl(shmid, IPC_RMID, nullptr);

    cout << "[Kernel] Todos los procesos industriales detenidos. Sistema finalizado y memoria IPC limpia." << endl;
    return 0;
}
