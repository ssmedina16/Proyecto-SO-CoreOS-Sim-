#ifndef PLANIFICADOR_HPP
#define PLANIFICADOR_HPP

#include "pcb.hpp"
#include <queue>
#include <vector>
#include <mutex>

/**Comparador para la cola SJF (Shortest Job First).
 * La ráfaga estimada más corta tiene mayor prioridad en la priority_queue.
 */
struct ComparadorSJF {
    bool operator()(const PCB* a, const PCB* b) {
        return a->rafaga_estimada > b->rafaga_estimada;
    }
};

/**Clase MLFQScheduler (Multi-Level Feedback Queue Scheduler).
 * Gestiona tres colas de procesos con diferentes políticas de planificación de forma thread-safe.
 */
class MLFQScheduler {
private:
    // Cola 1: Alta Prioridad - Round Robin
    std::queue<PCB*> cola1;
    std::mutex mutex_cola1;

    // Cola 2: Prioridad Media - Shortest Job First
    std::priority_queue<PCB*, std::vector<PCB*>, ComparadorSJF> cola2;
    std::mutex mutex_cola2;

    // Cola 3: Baja Prioridad - First Come First Served
    std::queue<PCB*> cola3;
    std::mutex mutex_cola3;

public:
    MLFQScheduler() = default;

    /**Inserta un proceso en la cola especificada de forma segura.
        proceso El PCB del proceso a encolar.
        numero_cola La cola destino (1, 2 o 3).
     */
    void encolarProceso(PCB* proceso, int numero_cola);

    /**
     *Intenta extraer el siguiente proceso de la cola especificada.
     * numero_cola La cola de la cual extraer (1, 2 o 3).
     * proceso_out Referencia donde se asignará el proceso extraído.
     * true si se extrajo con éxito, false si la cola estaba vacía.
     */
    bool desencolarProceso(int numero_cola, PCB*& proceso_out);

    /**
     *Verifica si una cola específica está vacía de forma thread-safe.
     *numero_cola La cola a verificar (1, 2 o 3).
     *true si está vacía, false en caso contrario.
     */
    bool estaVacia(int numero_cola);

    void ejecutarTurno(PCB* proceso, bool& termino_rafaga);

    void forzarRetornoPrioridad();

    /**
     * @brief Loop central del planificador. Actúa como el hilo maestro de la CPU virtual.
     */
    void runScheduler();

    /**
     * @brief Simula el despacho y paso de un proceso por la CPU virtual.
     * @param proceso Referencia al PCB extraído de las colas.
     */
    void simularCPU(PCB* proceso);
};

#endif // PLANIFICADOR_HPP


