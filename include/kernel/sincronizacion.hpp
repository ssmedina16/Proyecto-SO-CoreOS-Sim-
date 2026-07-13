#ifndef SINCRONIZACION_HPP
#define SINCRONIZACION_HPP

#include "pcb.hpp"
#include "planificador.hpp"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <atomic>

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class KernelSyncEngine;

// ============================================================================
// KMUTEX - Mutex administrado por el Kernel Virtual
// ============================================================================
/**
 * @brief Mutex a nivel de Kernel que integra el planificador MLFQ.
 *
 * A diferencia de std::mutex, KMutex transfiere el control directamente al
 * planificador cuando un proceso intenta adquirir un recurso ya ocupado.
 * El PCB del proceso bloqueado es marcado como BLOQUEADO y re-encolado cuando
 * el propietario libera el recurso (simula el comportamiento del SO real).
 */
class KMutex {
public:
    int id;                         ///< Identificador único del mutex en el sistema
    std::string nombre;             ///< Nombre descriptivo para logs del kernel

private:
    std::atomic<bool> bloqueado{false};   ///< Estado atómico del mutex
    std::atomic<int>  owner_id{-1};       ///< PID del proceso propietario (-1 = libre)
    int owner_prioridad_original{-1};     ///< Prioridad original antes de herencia

    std::mutex mutex_interno;             ///< Candado físico subyacente
    std::condition_variable cv;           ///< Variable de condición para bloqueo
    std::queue<int> cola_bloqueados;      ///< IDs de los PCBs en espera

public:
    KMutex() = default;
    KMutex(int id, const std::string& nombre);

    /**
     * @brief Intenta adquirir el mutex de forma bloqueante.
     *
     * Si el mutex está ocupado:
     *   1. Marca el PCB del llamante como BLOQUEADO.
     *   2. Registra la espera en el sistema de detección de deadlocks.
     *   3. Bloquea el hilo del wrapper hasta que el propietario libere el recurso.
     *   4. Aplica Herencia de Prioridad si el llamante tiene mayor prioridad.
     *
     * @param pcb       Referencia al PCB del proceso solicitante.
     * @param scheduler Referencia al planificador MLFQ para re-encolar.
     */
    void lock(PCB& pcb, MLFQScheduler& scheduler);

    /**
     * @brief Libera el mutex y despierta el siguiente proceso en cola.
     * Revierte la Herencia de Prioridad si fue aplicada.
     *
     * @param scheduler Referencia al planificador MLFQ para re-encolar al siguiente.
     */
    void unlock(MLFQScheduler& scheduler);

    bool estaLibre() const { return !bloqueado.load(); }
    int  getOwner()  const { return owner_id.load(); }
    std::queue<int> getColaBloqueados() const { return cola_bloqueados; }
};


// ============================================================================
// KSEMAPHORE - Semáforo contable administrado por el Kernel Virtual
// ============================================================================
/**
 * @brief Semáforo contable (counting semaphore) a nivel de Kernel.
 *
 * Permite controlar acceso concurrente a N unidades de un mismo recurso.
 * Integra el planificador: los procesos que no obtienen el recurso se marcan
 * BLOQUEADO y son despertados en orden FIFO cuando otro proceso llama signal().
 */
class KSemaphore {
public:
    int id;
    std::string nombre;

private:
    std::atomic<int> valor;               ///< Contador interno del semáforo
    std::mutex mutex_interno;
    std::condition_variable cv;
    std::queue<std::pair<int,int>> cola_bloqueados; ///< {pcb_id, cola_mlfq}

public:
    KSemaphore() = default;
    KSemaphore(int id, const std::string& nombre, int valor_inicial);

    /**
     * @brief Operación P (wait/down): decrementa el semáforo.
     * Si el valor es 0, bloquea el PCB llamante hasta que haya disponibilidad.
     *
     * @param pcb       Referencia al PCB del proceso solicitante.
     * @param scheduler Referencia al planificador MLFQ.
     */
    void wait(PCB& pcb, MLFQScheduler& scheduler);

    /**
     * @brief Operación V (signal/up): incrementa el semáforo y despierta un proceso.
     *
     * @param scheduler Referencia al planificador MLFQ para re-encolar.
     */
    void signal(MLFQScheduler& scheduler);

    int getValor() const { return valor.load(); }
};


// ============================================================================
// RESOURCE ALLOCATION GRAPH (RAG) - Grafo de asignación de recursos
// ============================================================================
/**
 * @brief Grafo dirigido para detección de deadlocks por ciclos.
 *
 * Almacena dos tipos de aristas:
 *   - Proceso -> Recurso: solicitud de un recurso (proceso espera por él)
 *   - Recurso -> Proceso: asignación de un recurso a un proceso
 *
 * Un ciclo en este grafo implica un deadlock.
 */
class ResourceAllocationGraph {
private:
    // adj[X] = conjunto de nodos a los que X tiene arista
    std::map<std::string, std::set<std::string>> adj;

    bool dfsCiclo(const std::string& nodo,
                  std::set<std::string>& visitados,
                  std::set<std::string>& en_pila) const;
public:
    /**
     * @brief Agrega la arista Proceso -> Recurso (solicitud).
     * Se agrega antes de intentar el lock, para poder detectar ciclos.
     */
    void agregarSolicitud(int pcb_id, int recurso_id);

    /**
     * @brief Agrega la arista Recurso -> Proceso (asignación).
     * Se agrega cuando el proceso adquiere efectivamente el recurso.
     */
    void agregarAsignacion(int recurso_id, int pcb_id);

    /** @brief Elimina las aristas de un proceso cuando libera su recurso. */
    void liberarRecurso(int recurso_id, int pcb_id);

    /**
     * @brief Verifica si el grafo actual contiene un ciclo (deadlock).
     * @return true si se detecta un ciclo.
     */
    bool detectarCiclo() const;

    void imprimirEstado() const;
};


// ============================================================================
// KERNEL SYNC ENGINE - Motor central de sincronización
// ============================================================================
/**
 * @brief Motor singleton de sincronización del Kernel Virtual.
 *
 * Centraliza el registro y gestión de todos los KMutex y KSemaphore del sistema.
 * Es el punto de entrada único para:
 *   - Creación de primitivas de sincronización con ID único.
 *   - Prevención de deadlocks mediante el RAG.
 *   - Aplicación del Protocolo de Herencia de Prioridad (PIP).
 *   - Diagnóstico y log del estado global de sincronización.
 */
class KernelSyncEngine {
private:
    std::map<int, KMutex*>     mutexes;
    std::map<int, KSemaphore*> semaforos;
    ResourceAllocationGraph    rag;
    std::mutex                 mutex_motor;   ///< Protege el acceso al motor

    static KernelSyncEngine* instancia;       ///< Instancia singleton

    KernelSyncEngine() = default;

public:
    // Patrón Singleton
    static KernelSyncEngine& getInstance();

    /**
     * @brief Registra un KMutex en el motor y lo devuelve para su uso.
     */
    KMutex& registrarMutex(int id, const std::string& nombre);

    /**
     * @brief Registra un KSemaphore en el motor y lo devuelve para su uso.
     */
    KSemaphore& registrarSemaforo(int id, const std::string& nombre, int valor_inicial);

    /**
     * @brief Delega lock de un KMutex con previa verificación del RAG.
     * Si la adquisición generaría un ciclo en el RAG, se emite una alerta
     * de Deadlock y la operación es rechazada/registrada.
     *
     * @return true si el lock fue adquirido, false si fue rechazado por deadlock.
     */
    bool lock(int mutex_id, PCB& pcb, MLFQScheduler& scheduler);

    /**
     * @brief Delega unlock de un KMutex y actualiza el RAG.
     */
    void unlock(int mutex_id, MLFQScheduler& scheduler);

    /**
     * @brief Delega wait de un KSemaphore.
     */
    void wait(int sem_id, PCB& pcb, MLFQScheduler& scheduler);

    /**
     * @brief Delega signal de un KSemaphore.
     */
    void signal(int sem_id, MLFQScheduler& scheduler);

    /**
     * @brief Imprime el estado diagnóstico de todos los recursos.
     */
    void imprimirDiagnostico() const;

    // -- Acceso al RAG para uso interno de KMutex --
    ResourceAllocationGraph& getRAG() { return rag; }

    ~KernelSyncEngine();
};

#endif // SINCRONIZACION_HPP
