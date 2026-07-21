#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <csignal>
#include "../include/industrial/fases_produccion.hpp"
#include "../include/kernel/memoria_virtual.hpp"
#include "../include/kernel/planificador.hpp"
#include "../include/kernel/wrappers.hpp"
#include "../include/kernel/sincronizacion.hpp"

using namespace std;

// Puntero global mapeado a la memoria compartida de la planta
Industrial::MemoriaCompartidaPlanta* Industrial::shared_planta = nullptr;

// Control atómico global del ciclo de vida del Kernel
volatile sig_atomic_t system_running = 1;

/**
 * Capturador de señales del SO (SIGINT/SIGTERM) para apagado controlado
 */
void interceptar_apagado_padre(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        system_running = 0;
    }
}

int main() {
    std::cout << ">>> [KERNEL] INICIANDO COREOS-SIM (ARQUITECTURA ULTRA-MODULAR CON MLFQ) <<<\n" << std::endl;

    // Configuración de señales
    signal(SIGTERM, interceptar_apagado_padre);
    signal(SIGINT, interceptar_apagado_padre);

    // Inicialización física de la memoria virtual en RAM
    Industrial::shared_planta = (Industrial::MemoriaCompartidaPlanta*)Industrial::inicializar_memoria_virtual();
    if (Industrial::shared_planta == nullptr || Industrial::shared_planta == (void*)-1) {
        std::cerr << "Error crítico: No se pudo inicializar la memoria compartida virtual." << std::endl;
        return 1;
    }

    // 1. Instanciación única del Planificador jerárquico
    MLFQScheduler planificador;

    // 2. Lanzamiento del hilo maestro de la CPU Virtual
    std::thread thread_kernel(&MLFQScheduler::runScheduler, &planificador);

    // 3. Lanzamiento limpio de hilos industriales llamando a los Wrappers externos
    std::vector<std::thread> hilos_sistema;
    hilos_sistema.push_back(std::thread(Industrial::wrapper_fase1_carbon, std::ref(planificador)));
    hilos_sistema.push_back(std::thread(Industrial::wrapper_fase2_logistica, std::ref(planificador)));
    hilos_sistema.push_back(std::thread(Industrial::wrapper_fase3_fase4_celdas, std::ref(planificador)));
    hilos_sistema.push_back(std::thread(Industrial::wrapper_fase5_trasiego, std::ref(planificador)));

    // 4. Lanzamiento del hilo supervisor de Inanición (Priority Boosting)
    std::thread thread_boost([&planificador]() {
        while (system_running) {
            // Duerme 10 veces 1 segundo
            for (int i = 0; i < 10; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!system_running) return; // Sale inmediatamente si el Kernel ordena apagar
            }
            
            std::cout << "\n >>> [KERNEL]: Alarma de envejecimiento activada. Forzando Priority Boost global... <<<\n";
            planificador.forzarRetornoPrioridad();
        }
    });

    // Bucle infinito del simulador hasta recibir orden de detención manual (SIGINT/SIGTERM)
    while (system_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\n[Kernel]: Señal de detención recibida. Solicitando apagado general de hilos...\n";
    system_running = 0;

    // Unir todos los hilos de manera segura para evitar fugas de contexto
    if (thread_kernel.joinable()) thread_kernel.join();
    if (thread_boost.joinable()) thread_boost.join();
    for (auto& th : hilos_sistema) {
        if (th.joinable()) th.join();
    }

    // Diagnóstico del motor de sincronización antes del cierre
    KernelSyncEngine::getInstance().imprimirDiagnostico();

    // Desmapear memoria virtual y limpiar enlace del SO
    Industrial::liberar_memoria_virtual();

    std::cout << "[Kernel]: Simulación finalizada correctamente. Memoria liberada." << std::endl;
    return 0;
}