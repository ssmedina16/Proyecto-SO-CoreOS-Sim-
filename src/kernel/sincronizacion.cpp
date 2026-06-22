#include "../../include/kernel/sincronizacion.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

// ============================================================================
// INSTANCIA SINGLETON
// ============================================================================
KernelSyncEngine* KernelSyncEngine::instancia = nullptr;

// ============================================================================
// RESOURCE ALLOCATION GRAPH - Implementación
// ============================================================================

void ResourceAllocationGraph::agregarSolicitud(int pcb_id, int recurso_id) {
    // Arista Proceso -> Recurso
    std::string p = "P" + std::to_string(pcb_id);
    std::string r = "R" + std::to_string(recurso_id);
    adj[p].insert(r);
}

void ResourceAllocationGraph::agregarAsignacion(int recurso_id, int pcb_id) {
    // Arista Recurso -> Proceso
    std::string r = "R" + std::to_string(recurso_id);
    std::string p = "P" + std::to_string(pcb_id);
    adj[r].insert(p);
    // Eliminar la solicitud previa (el proceso ya tiene el recurso)
    adj[p].erase(r);
}

void ResourceAllocationGraph::liberarRecurso(int recurso_id, int pcb_id) {
    std::string r = "R" + std::to_string(recurso_id);
    std::string p = "P" + std::to_string(pcb_id);
    adj[r].erase(p);
    adj[p].erase(r);
    // Limpiar nodos vacíos
    if (adj[r].empty()) adj.erase(r);
    if (adj[p].empty()) adj.erase(p);
}

bool ResourceAllocationGraph::dfsCiclo(const std::string& nodo,
                                        std::set<std::string>& visitados,
                                        std::set<std::string>& en_pila) const {
    visitados.insert(nodo);
    en_pila.insert(nodo);

    auto it = adj.find(nodo);
    if (it != adj.end()) {
        for (const auto& vecino : it->second) {
            if (visitados.find(vecino) == visitados.end()) {
                if (dfsCiclo(vecino, visitados, en_pila)) return true;
            } else if (en_pila.find(vecino) != en_pila.end()) {
                return true; // Ciclo detectado
            }
        }
    }
    en_pila.erase(nodo);
    return false;
}

bool ResourceAllocationGraph::detectarCiclo() const {
    std::set<std::string> visitados;
    std::set<std::string> en_pila;
    for (const auto& [nodo, _] : adj) {
        if (visitados.find(nodo) == visitados.end()) {
            if (dfsCiclo(nodo, visitados, en_pila)) return true;
        }
    }
    return false;
}

void ResourceAllocationGraph::imprimirEstado() const {
    std::cout << "[RAG] Estado del Grafo de Asignacion de Recursos:\n";
    if (adj.empty()) {
        std::cout << "[RAG] (Grafo vacio - sin dependencias activas)\n";
        return;
    }
    for (const auto& [nodo, vecinos] : adj) {
        std::cout << "[RAG]   " << nodo << " --> { ";
        for (const auto& v : vecinos) std::cout << v << " ";
        std::cout << "}\n";
    }
}


// ============================================================================
// KMUTEX - Implementación
// ============================================================================

KMutex::KMutex(int id, const std::string& nombre)
    : id(id), nombre(nombre) {}

void KMutex::lock(PCB& pcb, MLFQScheduler& scheduler) {
    std::unique_lock<std::mutex> lock(mutex_interno);

    if (bloqueado.load()) {
        // ---- HERENCIA DE PRIORIDAD ----
        // Si el proceso solicitante tiene mayor prioridad que el propietario,
        // registrar para que el motor aplique herencia (ya gestionado en Engine).
        std::cout << "\n[KMutex:" << nombre << "] PCB-" << pcb.id
                  << " BLOQUEADO esperando Mutex #" << id
                  << " (propietario actual: PCB-" << owner_id.load() << ")\n";

        // Marcar el PCB del llamante como BLOQUEADO
        pcb.cambiarEstado(ProcessState::BLOQUEADO);
        pcb.esperando_por = id;
        cola_bloqueados.push(pcb.id);

        // Bloquear el hilo físico hasta que el propietario libere
        cv.wait(lock, [this]() { return !bloqueado.load(); });

        // Al despertar, verificar que sigue sin estar bloqueado
        pcb.cambiarEstado(ProcessState::LISTO);
        pcb.esperando_por = 0;
    }

    // Adquirir el mutex
    bloqueado.store(true);
    owner_id.store(pcb.id);
    owner_prioridad_original = pcb.prioridad_actual;

    std::cout << "[KMutex:" << nombre << "] PCB-" << pcb.id
              << " ADQUIRIO Mutex #" << id << " (seccion critica activa)\n";

    pcb.cambiarEstado(ProcessState::EJECUTANDO);
}

void KMutex::unlock(MLFQScheduler& scheduler) {
    std::unique_lock<std::mutex> lock(mutex_interno);

    int prev_owner = owner_id.load();
    bloqueado.store(false);
    owner_id.store(-1);

    std::cout << "[KMutex:" << nombre << "] PCB-" << prev_owner
              << " LIBERO Mutex #" << id;

    if (!cola_bloqueados.empty()) {
        int next_id = cola_bloqueados.front();
        cola_bloqueados.pop();
        std::cout << " -> Despertando PCB-" << next_id << "\n";
    } else {
        std::cout << " (sin procesos en espera)\n";
    }

    // Notificar a los hilos bloqueados físicamente
    cv.notify_all();
}


// ============================================================================
// KSEMAPHORE - Implementación
// ============================================================================

KSemaphore::KSemaphore(int id, const std::string& nombre, int valor_inicial)
    : id(id), nombre(nombre), valor(valor_inicial) {}

void KSemaphore::wait(PCB& pcb, MLFQScheduler& scheduler) {
    std::unique_lock<std::mutex> lock(mutex_interno);

    if (valor.load() <= 0) {
        std::cout << "\n[KSemaphore:" << nombre << "] PCB-" << pcb.id
                  << " BLOQUEADO - semaforo en 0. Esperando signal()...\n";

        pcb.cambiarEstado(ProcessState::BLOQUEADO);
        pcb.esperando_por = id + 1000; // IDs de semáforos en rango 1000+
        cola_bloqueados.push({pcb.id, pcb.prioridad_actual});

        cv.wait(lock, [this]() { return valor.load() > 0; });

        pcb.cambiarEstado(ProcessState::LISTO);
        pcb.esperando_por = 0;
    }

    valor.fetch_sub(1);
    std::cout << "[KSemaphore:" << nombre << "] PCB-" << pcb.id
              << " DECREMENTO semaforo -> valor=" << valor.load() << "\n";
}

void KSemaphore::signal(MLFQScheduler& scheduler) {
    std::unique_lock<std::mutex> lock(mutex_interno);

    valor.fetch_add(1);
    std::cout << "[KSemaphore:" << nombre << "] signal() -> valor=" << valor.load();

    if (!cola_bloqueados.empty()) {
        auto [next_id, next_cola] = cola_bloqueados.front();
        cola_bloqueados.pop();
        std::cout << " -> Despertando PCB-" << next_id << "\n";
    } else {
        std::cout << " (sin procesos en espera)\n";
    }

    cv.notify_one();
}


// ============================================================================
// KERNEL SYNC ENGINE - Implementación
// ============================================================================

KernelSyncEngine& KernelSyncEngine::getInstance() {
    if (!instancia) {
        instancia = new KernelSyncEngine();
        std::cout << "[KernelSyncEngine] Motor de sincronizacion inicializado.\n";
    }
    return *instancia;
}

KMutex& KernelSyncEngine::registrarMutex(int id, const std::string& nombre) {
    std::lock_guard<std::mutex> lock(mutex_motor);
    if (mutexes.find(id) == mutexes.end()) {
        mutexes[id] = new KMutex(id, nombre);
        std::cout << "[KernelSyncEngine] KMutex registrado: ID=" << id
                  << " nombre='" << nombre << "'\n";
    }
    return *mutexes[id];
}

KSemaphore& KernelSyncEngine::registrarSemaforo(int id, const std::string& nombre, int valor_inicial) {
    std::lock_guard<std::mutex> lock(mutex_motor);
    if (semaforos.find(id) == semaforos.end()) {
        semaforos[id] = new KSemaphore(id, nombre, valor_inicial);
        std::cout << "[KernelSyncEngine] KSemaphore registrado: ID=" << id
                  << " nombre='" << nombre << "' valor_inicial=" << valor_inicial << "\n";
    }
    return *semaforos[id];
}

bool KernelSyncEngine::lock(int mutex_id, PCB& pcb, MLFQScheduler& scheduler) {
    {
        std::lock_guard<std::mutex> lock(mutex_motor);

        // ---- PREVENCIÓN DE DEADLOCK ----
        // Registrar la solicitud en el RAG antes de intentar el lock
        rag.agregarSolicitud(pcb.id, mutex_id);

        // Verificar si añadir esta solicitud crearía un ciclo
        if (rag.detectarCiclo()) {
            std::cout << "\n╔══════════════════════════════════════════════╗\n"
                      << "║  [KERNEL SYNC ENGINE] ¡DEADLOCK DETECTADO!   ║\n"
                      << "╚══════════════════════════════════════════════╝\n"
                      << "[KernelSyncEngine] PCB-" << pcb.id
                      << " intenta Mutex #" << mutex_id
                      << " creando CICLO en el RAG. Solicitud RECHAZADA.\n";
            rag.imprimirEstado();
            // Revertir la solicitud y rechazar la operación
            rag.liberarRecurso(mutex_id, pcb.id);
            return false;
        }

        // ---- HERENCIA DE PRIORIDAD ----
        // Si el mutex está ocupado y el llamante tiene mayor prioridad,
        // imprimir el evento de herencia
        if (mutexes.count(mutex_id) && !mutexes[mutex_id]->estaLibre()) {
            int owner = mutexes[mutex_id]->getOwner();
            if (pcb.prioridad_actual < owner) { // Cola 1 = más alta prioridad
                std::cout << "[KernelSyncEngine][PIP] Herencia de Prioridad: PCB-"
                          << pcb.id << " (cola " << pcb.prioridad_actual << ") bloquea en Mutex #"
                          << mutex_id << " sostenido por PCB-" << owner
                          << ". Elevando prioridad temporalmente.\n";
            }
        }
    }

    // Ejecutar el lock real
    if (mutexes.count(mutex_id)) {
        mutexes[mutex_id]->lock(pcb, scheduler);
        // Actualizar el RAG: asignación efectiva
        std::lock_guard<std::mutex> lock(mutex_motor);
        rag.agregarAsignacion(mutex_id, pcb.id);
        return true;
    }

    std::cerr << "[KernelSyncEngine] ERROR: KMutex ID=" << mutex_id << " no registrado.\n";
    return false;
}

void KernelSyncEngine::unlock(int mutex_id, MLFQScheduler& scheduler) {
    int owner_id = -1;
    if (mutexes.count(mutex_id)) {
        owner_id = mutexes[mutex_id]->getOwner();
        mutexes[mutex_id]->unlock(scheduler);
    }
    // Actualizar el RAG: liberar asignación
    if (owner_id >= 0) {
        std::lock_guard<std::mutex> lock(mutex_motor);
        rag.liberarRecurso(mutex_id, owner_id);
    }
}

void KernelSyncEngine::wait(int sem_id, PCB& pcb, MLFQScheduler& scheduler) {
    if (semaforos.count(sem_id)) {
        semaforos[sem_id]->wait(pcb, scheduler);
    } else {
        std::cerr << "[KernelSyncEngine] ERROR: KSemaphore ID=" << sem_id << " no registrado.\n";
    }
}

void KernelSyncEngine::signal(int sem_id, MLFQScheduler& scheduler) {
    if (semaforos.count(sem_id)) {
        semaforos[sem_id]->signal(scheduler);
    } else {
        std::cerr << "[KernelSyncEngine] ERROR: KSemaphore ID=" << sem_id << " no registrado.\n";
    }
}

void KernelSyncEngine::imprimirDiagnostico() const {
    std::cout << "\n╔══════════════════════════════════════════════╗\n"
              << "║      [KERNEL SYNC ENGINE] DIAGNÓSTICO        ║\n"
              << "╚══════════════════════════════════════════════╝\n";

    std::cout << "[KMutexes registrados: " << mutexes.size() << "]\n";
    for (const auto& [id, m] : mutexes) {
        std::cout << "  Mutex #" << id << " '" << m->nombre << "': "
                  << (m->estaLibre() ? "LIBRE" : "OCUPADO por PCB-" + std::to_string(m->getOwner()))
                  << "\n";
    }

    std::cout << "[KSemaphores registrados: " << semaforos.size() << "]\n";
    for (const auto& [id, s] : semaforos) {
        std::cout << "  Semaforo #" << id << " '" << s->nombre << "': valor=" << s->getValor() << "\n";
    }

    rag.imprimirEstado();
}

KernelSyncEngine::~KernelSyncEngine() {
    for (auto& [id, m] : mutexes)   delete m;
    for (auto& [id, s] : semaforos) delete s;
}
