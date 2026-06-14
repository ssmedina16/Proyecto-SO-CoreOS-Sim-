#include "../../include/industrial/fases_produccion.hpp"
#include "../../include/kernel/pcb.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

using namespace std;

// Comunicación con el Kernel
extern volatile sig_atomic_t system_running;

// Candado para la consola en este proceso
static std::mutex candado_consola;

namespace Industrial
{
    // Definición e inicialización del mutex de control industrial
    std::mutex mutex_crisol_succion;

    static void recibir_signal_apagado_f5(int sig)
    {
        if (sig == SIGTERM)
        {
            system_running = 0;
        }
    }

    void fase_trasiego_crisol(int cantidad_crisoles, EstadoCelda* celdas_param, int cantidad_celdas)
    {
        signal(SIGTERM, recibir_signal_apagado_f5);

        candado_consola.lock();
        std::cout << "\n>>> [PROCESO] INICIANDO FASE 5: LOGÍSTICA Y TRASIEGO (CRISOL) - PID: " << getpid() << " <<<\n\n";
        candado_consola.unlock();

        EstadoCelda* celdas = celdas_param;
        int shmid = -1;
        
        // Si no se pasó un puntero directo, adjuntar a memoria compartida
        if (celdas == nullptr)
        {
            key_t key = 12345;
            shmid = shmget(key, sizeof(EstadoCelda) * cantidad_celdas, 0666);
            if (shmid < 0)
            {
                candado_consola.lock();
                std::cerr << "[Fase 5 - Error] No se pudo obtener el segmento de memoria compartida para las celdas." << std::endl;
                candado_consola.unlock();
                exit(1);
            }
            celdas = (EstadoCelda*) shmat(shmid, nullptr, 0);
        }

        // Creación dinámica de hilos y sus estructuras de control
        std::vector<EstadoCrisol> lista_crisoles(cantidad_crisoles);
        std::vector<std::thread> hilos_crisol;

        for (int i = 0; i < cantidad_crisoles; ++i)
        {
            lista_crisoles[i].id_crisol = i + 1;
            // Para la simulación de 15 segundos, ajustamos la capacidad crítica a 300.0f
            // de modo que podamos ver el vaciado lógico en funcionamiento.
            lista_crisoles[i].capacidad_max = 300.0f;
            lista_crisoles[i].aluminio_recolectado = 0.0f;
            lista_crisoles[i].en_operacion = false;
            
            hilos_crisol.push_back(std::thread(Hilo_Crisol, std::ref(lista_crisoles[i]), celdas, cantidad_celdas));
        }

        // Esperar a que terminen los hilos (cuando system_running sea 0)
        for (auto &th : hilos_crisol)
        {
            if (th.joinable()) th.join();
        }

        candado_consola.lock();
        std::cout << "[Logística y Trasiego] Resumen de recolección de Crisoles:\n";
        for (int i = 0; i < cantidad_crisoles; ++i)
        {
            std::cout << "  * Crisol #" << lista_crisoles[i].id_crisol 
                      << ": " << lista_crisoles[i].aluminio_recolectado << " kg recolectados.\n";
        }
        std::cout << "[Logística y Trasiego] Proceso finalizado.\n";
        candado_consola.unlock();

        if (shmid >= 0)
        {
            shmdt(celdas);
        }
        exit(0);
    }

    void Hilo_Crisol(EstadoCrisol &mi_estado, EstadoCelda* celdas, int cantidad_celdas)
    {
        // Inicializar estructura PCB simulada para el Crisol
        PCB_Venalum pcb_crisol;
        pcb_crisol.PID = mi_estado.id_crisol + 200; // Rango de PID de simulación para crisoles
        pcb_crisol.Tipo_Entidad = 2;               // 2 = Crisol
        pcb_crisol.estado = EstadoProceso::EJECUTANDO;
        pcb_crisol.program_counter = 0;
        pcb_crisol.base_limit = sizeof(EstadoCelda) * cantidad_celdas;
        pcb_crisol.cuota_alumina = (int)mi_estado.capacidad_max;
        pcb_crisol.esperando_por = 0;

        while (system_running)
        {
            // Retardo de sistema controlado (Equivalente asíncrono a 24-48 horas)
            // Hacemos sleeps pequeños de 500ms en bucle para responder rápido al apagado
            for (int s = 0; s < 10; ++s)
            {
                if (!system_running) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            if (!system_running) break;

            mi_estado.en_operacion = true;

            // Recorrido de las celdas en busca de exceso
            for (int i = 0; i < cantidad_celdas; ++i)
            {
                if (!system_running) break;

                // Verificación del umbral lógico de succión (300.0f kg para simulación)
                if (celdas[i].aluminio_producido >= mi_estado.capacidad_max)
                {
                    candado_consola.lock();
                    std::cout << "[Aviso] Hilo_Crisol #" << mi_estado.id_crisol 
                              << " detecta exceso en Celda #" << celdas[i].id_celda 
                              << " (" << celdas[i].aluminio_producido << " kg)" << std::endl;
                    
                    // Simulación en PCB: Cambio de contexto a BLOQUEADO
                    pcb_crisol.estado = EstadoProceso::BLOQUEADO;
                    pcb_crisol.esperando_por = 1; // 1 = ID de mutex_crisol_succion
                    std::cout << "[Kernel OS] PCB PID " << pcb_crisol.PID 
                              << " cambiado a BLOQUEADO esperando mutex_crisol_succion (ID: 1)" << std::endl;
                    candado_consola.unlock();

                    // Exclusión Mutua para resguardar la sección crítica
                    std::unique_lock<std::mutex> lock(mutex_crisol_succion);

                    // Simulación en PCB: Cambio de contexto a EJECUTANDO tras adquirir el mutex
                    pcb_crisol.estado = EstadoProceso::EJECUTANDO;
                    pcb_crisol.esperando_por = 0;

                    candado_consola.lock();
                    std::cout << "[Mutex] Hilo_Crisol #" << mi_estado.id_crisol 
                              << " adquiere exclusión de vaciado. PCB PID " << pcb_crisol.PID 
                              << " cambiado a EJECUTANDO." << std::endl;

                    // Operación crítica sobre variables compartidas (Vaciado lógico)
                    float material_transferido = celdas[i].aluminio_producido;
                    celdas[i].aluminio_producido = 0.0f; // Vaciado lógico
                    mi_estado.aluminio_recolectado += material_transferido;

                    std::cout << "[Éxito] Succión finalizada: " << material_transferido 
                              << " kg retirados de la Celda #" << celdas[i].id_celda << std::endl;
                    candado_consola.unlock();
                    
                    break; // Libera el lock de forma segura y espera el siguiente ciclo
                }
            }
            
            mi_estado.en_operacion = false;
        }
    }
}
