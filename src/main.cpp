#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>   // Requerido para fork() y pid_t
#include <signal.h>   // Requerido para señales del SO (kill, SIGTERM)
#include <sys/wait.h> // Requerido para waitpid()
#include "../include/industrial/fases_produccion.hpp"
#include "../include/kernel/memoria_virtual.hpp"
#include "../include/kernel/planificador.hpp"

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

// ============================================================================
// ADAPTACIÓN DE FASES INDUSTRIALES PARA TRABAJAR CON EL PLANIFICADOR (HILOS)
// ============================================================================

void wrapper_fase1_carbon(MLFQScheduler& scheduler) {
    // Crear el bloque administrativo (PCB) para la Fase 1
    PCB pcb_fase1 = { 1, "Planta de Carbon", ProcessState::LISTO, 1, 0.0, 25.0 }; // Ráfaga estimada de 25 unidades
    
    // Inyectar el proceso en la Cola 1 (Prioridad Alta - Round Robin inicial)
    scheduler.encolarProceso(pcb_fase1, 1);

    while (system_running) {
        // Simulación: La planta opera y genera solicitudes de procesamiento cíclicas
        this_thread::sleep_for(chrono::seconds(4));
        if (!system_running) break;
        
        // Si el proceso ya culminó su turno de CPU virtual previo, genera un nuevo lote de trabajo
        if (pcb_fase1.estado == ProcessState::BLOQUEADO) { //
            pcb_fase1.rafaga_estimada = 15.0; // Próxima ráfaga térmica
            pcb_fase1.cambiarEstado(ProcessState::LISTO); //
            std::cout << "[Fase 1 - Carbón]: Generando nuevo lote de Ánodos. Solicitando CPU virtual...\n";
            scheduler.encolarProceso(pcb_fase1, 1); // Volver a encolar en Cola de Entrada
        }
    }
}

void wrapper_fase2_logistica(MLFQScheduler& scheduler) {
    // Registrar el PCB de la Fase 2
    PCB pcb_fase2 = { 2, "Logistica y Transporte", ProcessState::LISTO, 1, 0.0, 8.0 }; // Trabajo corto (8 unidades)
    scheduler.encolarProceso(pcb_fase2, 1); //

    while (system_running) {
        this_thread::sleep_for(chrono::seconds(3));
        if (!system_running) break;

        if (pcb_fase2.estado == ProcessState::BLOQUEADO) { //
            pcb_fase2.rafaga_estimada = 5.0; // Escaneo rápido de tolvas
            pcb_fase2.cambiarEstado(ProcessState::LISTO); //
            std::cout << "[Fase 2 - Logística]: Escaneo periódico de tolvas requerido. Encolando...\n";
            scheduler.encolarProceso(pcb_fase2, 1); //
        }
    }
}

void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler) {
    // Registrar el PCB de la Fase 3/4 unificada
    PCB pcb_fase3 = { 3, "Celdas Reduccion y GTC", ProcessState::LISTO, 1, 0.0, 30.0 }; // Proceso pesado
    scheduler.encolarProceso(pcb_fase3, 1); //

    while (system_running) {
        this_thread::sleep_for(chrono::seconds(5));
        if (!system_running) break;

        if (pcb_fase3.estado == ProcessState::BLOQUEADO) { //
            pcb_fase3.rafaga_estimada = 35.0; // Reacción química de electrólisis pesada
            pcb_fase3.cambiarEstado(ProcessState::LISTO); //
            std::cout << "[Fase 3 - Celdas]: Reacción de reducción masiva lista. Solicitando planificación...\n";
            scheduler.encolarProceso(pcb_fase3, 1); //
        }
    }
}

// ============================================================================
// FUNCIÓN PRINCIPAL (MAIN KERNEL CON ENTORNO MULTIHILO)
// ============================================================================
int main() {
    std::cout << ">>> [KERNEL] INICIANDO COREOS-SIM (ARQUITECTURA MULTIHILO CON MLFQ) <<<\n" << std::endl;

    // Configuración de señales del SO
    signal(SIGTERM, interceptar_apagado_padre); //
    signal(SIGINT, interceptar_apagado_padre);  //

    // Inicializar el gestor de memoria virtual compartida (RAM compartida POSIX mapeada)
    Industrial::shared_planta = (Industrial::MemoriaCompartidaPlanta*)Industrial::inicializar_memoria_virtual(); //

    if (Industrial::shared_planta == nullptr || Industrial::shared_planta == (void*)-1) { //
        std::cerr << "Error crítico: No se pudo inicializar la memoria compartida." << std::endl; //
        return 1; //
    }

    // 1. Instanciar el objeto global del Planificador MLFQ
    MLFQScheduler planificador;

    // 2. Lanzar el hilo independiente del planificador (CPU Virtual Maestro)
    std::thread thread_kernel(&MLFQScheduler::runScheduler, &planificador);

    // 3. Lanzar los hilos de las fases industriales (Fusión de Fases)
    std::vector<std::thread> hilos_sistema;
    
    hilos_sistema.push_back(std::thread(wrapper_fase1_carbon, std::ref(planificador)));
    hilos_sistema.push_back(std::thread(wrapper_fase2_logistica, std::ref(planificador)));
    hilos_sistema.push_back(std::thread(wrapper_fase3_fase4_celdas, std::ref(planificador)));

    // 4. Hilo de control de Inanición (Priority Boosting)
    // Se ejecuta de forma concurrente para limpiar las colas inferiores periódicamente
    std::thread thread_boost([&planificador]() {
        while (system_running) {
            std::this_thread::sleep_for(std::chrono::seconds(10)); // Alarma cada 10 segundos virtuales
            if (!system_running) break;
            std::cout << "\n>>> [KERNEL]: Alarma de envejecimiento activada. Forzando Priority Boost... <<<\n";
            planificador.forzarRetornoPrioridad(); // Evita la inanición de ráfagas largas
        }
    });

    // El Kernel duerme el hilo principal durante el tiempo de simulación global establecido
    std::this_thread::sleep_for(std::chrono::seconds(25));
    
    std::cout << "\n[Kernel]: Tiempo de simulación concluido. Apagando componentes y liberando contextos...\n";
    system_running = 0; // Provoca la ruptura de los loops de producción y de planificación

    // Unir todos los hilos activos de forma segura para evitar fugas o punteros colgantes
    if (thread_kernel.joinable()) thread_kernel.join();
    if (thread_boost.joinable()) thread_boost.join();
    
    for (auto& th : hilos_sistema) {
        if (th.joinable()) th.join();
    }

    // Liberación del objeto de memoria compartida del SO mediante munmap/shm_unlink
    Industrial::liberar_memoria_virtual(); //

    std::cout << "[Kernel]: Todos los hilos industriales detenidos. Simulación MLFQ finalizada con éxito." << std::endl;
    return 0;
}