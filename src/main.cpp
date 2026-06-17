#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>   // Requerido para fork() y pid_t
#include <signal.h>   // Requerido para señales del SO (kill, SIGTERM)
#include <sys/wait.h> // Requerido para waitpid()
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/industrial/fases_produccion.hpp"

using namespace std;

Industrial::MemoriaCompartidaPlanta* Industrial::shared_planta = nullptr;

// Variables de control globales (utilizadas principalmente por el Padre)
volatile sig_atomic_t system_running = 1;

// Inicialización de IPC para silo y tolvas
// init_ipc_silo_tolvas removed per request; no shared silo/tolvas in main

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
    std::cout << ">>> [KERNEL] INICIANDO COREOS-SIM (ARQUITECTURA MULTIPROCESO) <<<\n"
              << std::endl;

    // Configuración de señal para el padre
    signal(SIGTERM, interceptar_apagado_padre);
    signal(SIGINT, interceptar_apagado_padre);

    /**
     * INICIALIZACIÓN DEL GESTOR DE MEMORIA VIRTUAL COMPARTIDA (SHM)
     * -----------------------------------------------------------
     * 1. shm_open: Crea o abre un objeto de memoria compartida POSIX.
     * 2. ftruncate: Define el tamaño físico en bytes (sizeof de la estructura).
     * 3. mmap: Mapea el objeto en el espacio de direcciones del proceso.
     * 4. shared_planta: Puntero que ahora apunta directamente a RAM compartida.
     */
    int shm_fd = shm_open("/shm_venalum", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(Industrial::MemoriaCompartidaPlanta)) == -1) {
        perror("ftruncate");
        return 1;
    }

    Industrial::shared_planta = (Industrial::MemoriaCompartidaPlanta*)mmap(NULL, 
        sizeof(Industrial::MemoriaCompartidaPlanta), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (Industrial::shared_planta == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Inicializar datos iniciales directamente en la RAM compartida
    Industrial::shared_planta->silo_alumina = 50000.0f;
    for (int i = 0; i < 5; ++i) {
        Industrial::shared_planta->tolvas_celdas[i] = 400.0f;
    }
    Industrial::shared_planta->anodos_producidos = 0;

    // 1. LANZAMIENTO DE LA FASE 1: PLANTA DE CARBÓN (SANTIAGO)
    pid_t pid_plantaCarbon = fork();

    if (pid_plantaCarbon == 0)
    {
        // PROCESO HIJO: Planta de Carbón
        // El hijo configura sus propios handlers dentro de su .cpp
        Industrial::fase_planta_carbon();
        return 0;
    }

    // 2. LANZAMIENTO DE LA FASE 2: LOGÍSTICA Y TRANSPORTE
    pid_t pid_logistica = fork();
    if (pid_logistica == 0) {
        // PROCESO HIJO: Logística y Transporte
        Industrial::fase_logistica_transporte();
        return 0;
    }

    // 3. LANZAMIENTO DE LA FASE 3: CELDAS DE REDUCCIÓN (JP)
    pid_t pid_celdasElectroliticas = fork();

    if (pid_celdasElectroliticas == 0)
    {
        // PROCESO HIJO: Celdas de Reducción
        Industrial::fase_celdas_reduccion(5);
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
    if (pid_logistica > 0) kill(pid_logistica, SIGTERM);
    if (pid_celdasElectroliticas > 0) kill(pid_celdasElectroliticas, SIGTERM);

    // Limpieza de procesos (evitar zombies)
    waitpid(pid_plantaCarbon, nullptr, 0);
    waitpid(pid_logistica, nullptr, 0);
    waitpid(pid_celdasElectroliticas, nullptr, 0);

    /**
     * LIMPIEZA DE RECURSOS IPC
     * ------------------------
     * munmap: Desmapea la memoria del espacio del proceso.
     * shm_unlink: Elimina el objeto de memoria compartida del sistema Linux.
     */
    if (Industrial::shared_planta != nullptr && Industrial::shared_planta != MAP_FAILED) {
        munmap(Industrial::shared_planta, sizeof(Industrial::MemoriaCompartidaPlanta));
        shm_unlink("/shm_venalum");
    }

    cout << "[Kernel] Todos los procesos industriales detenidos. Sistema finalizado." << endl;
    return 0;
}
