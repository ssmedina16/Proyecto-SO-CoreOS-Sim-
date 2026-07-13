#include "../../include/kernel/planificador.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

// Flag global de control de apagado heredado del sistema
extern volatile sig_atomic_t system_running;

void MLFQScheduler::encolarProceso(PCB* proceso, int numero_cola) {
    // Actualizamos la prioridad del PCB según la cola donde se inserta
    proceso->prioridad_actual = numero_cola;
    
    // El uso de mutex independientes permite que las operaciones en colas distintas
    // no se bloqueen entre sí, optimizando la concurrencia.
    switch (numero_cola) {
        case 1: {
            std::lock_guard<std::mutex> lock(mutex_cola1);
            cola1.push(proceso);
            break;
        }
        case 2: {
            std::lock_guard<std::mutex> lock(mutex_cola2);
            cola2.push(proceso);
            break;
        }
        case 3: {
            std::lock_guard<std::mutex> lock(mutex_cola3);
            cola3.push(proceso);
            break;
        }
        default:
            std::cerr << "[ERROR Scheduler] Número de cola inválido: " << numero_cola << std::endl;
    }
}

bool MLFQScheduler::desencolarProceso(int numero_cola, PCB*& proceso_out) {
    switch (numero_cola) {
        case 1: {
            std::lock_guard<std::mutex> lock(mutex_cola1);
            if (cola1.empty()) return false;
            proceso_out = cola1.front();
            cola1.pop();
            return true;
        }
        case 2: {
            std::lock_guard<std::mutex> lock(mutex_cola2);
            if (cola2.empty()) return false;
            proceso_out = cola2.top();
            cola2.pop();
            return true;
        }
        case 3: {
            std::lock_guard<std::mutex> lock(mutex_cola3);
            if (cola3.empty()) return false;
            proceso_out = cola3.front();
            cola3.pop();
            return true;
        }
        default:
            return false;
    }
}

bool MLFQScheduler::estaVacia(int numero_cola) {
    switch (numero_cola) {
        case 1: {
            std::lock_guard<std::mutex> lock(mutex_cola1);
            return cola1.empty();
        }
        case 2: {
            std::lock_guard<std::mutex> lock(mutex_cola2);
            return cola2.empty();
        }
        case 3: {
            std::lock_guard<std::mutex> lock(mutex_cola3);
            return cola3.empty();
        }
        default:
            return true;
    }
}

void MLFQScheduler::ejecutarTurno(PCB* proceso, bool& termino_rafaga) {
    // Definición del Quantum estricto para la Cola 1 (Round Robin)
    const double time_slice = 10.0;

    // --- COLA 1: POLÍTICA ROUND ROBIN CON DEGRADACIÓN ---
    if (proceso->prioridad_actual == 1) {
        if (proceso->rafaga_estimada > time_slice) {
            // El proceso consume todo el Quantum sin terminar su trabajo
            proceso->tiempo_ejecutado += time_slice;
            proceso->rafaga_estimada -= time_slice;
            termino_rafaga = false;

            // Degradación: Al superar el time_slice, se penaliza re-encolándolo en la Cola 2 (SJF)
            std::cout << "[MLFQ CPU] Proceso ID " << proceso->id 
                      << " agotó su quantum en Cola 1. Degradando a Cola 2.\n";
            encolarProceso(proceso, 2);
        } 
        else {
            // El proceso termina dentro o justo en el límite del Quantum
            proceso->tiempo_ejecutado += proceso->rafaga_estimada;
            proceso->rafaga_estimada = 0.0;
            termino_rafaga = true;
        }
    }
    // --- COLAS 2 Y 3: POLÍTICAS NO APROPIATIVAS (SJF / FCFS) ---
    else if (proceso->prioridad_actual == 2 || proceso->prioridad_actual == 3) {
        // Al no estar limitadas por rodajas de tiempo, ejecutan toda su ráfaga restante de forma continua
        proceso->tiempo_ejecutado += proceso->rafaga_estimada;
        proceso->rafaga_estimada = 0.0;
        termino_rafaga = true;
    }
}

void MLFQScheduler::forzarRetornoPrioridad() {
    PCB* proceso_aux = nullptr;

    // 1. Vaciar la Cola 2 y pasar todo a la Cola 1
    while (desencolarProceso(2, proceso_aux)) {
        std::cout << "[MLFQ BOOST] Promoviendo Proceso ID " << proceso_aux->id << " de Cola 2 a Cola 1 por envejecimiento.\n";
        encolarProceso(proceso_aux, 1);
    }

    // 2. Vaciar la Cola 3 y pasar todo a la Cola 1
    while (desencolarProceso(3, proceso_aux)) {
        std::cout << "[MLFQ BOOST] Promoviendo Proceso ID " << proceso_aux->id << " de Cola 3 a Cola 1 por envejecimiento.\n";
        encolarProceso(proceso_aux, 1);
    }
}

void MLFQScheduler::runScheduler() {
    std::cout << "[KERNEL - SCHEDULER]: Hilo maestro del planificador MLFQ iniciado.\n";

    while (system_running) {
        PCB* proceso_a_despachar = nullptr;
        bool proceso_encontrado = false;

        // 1. REGLA DE PRIORIDAD ESTRICTA: Escaneo jerárquico de colas
        if (!estaVacia(1)) { // Comprobar Cola 1 (Round Robin)
            if (desencolarProceso(1, proceso_a_despachar)) { //
                proceso_encontrado = true;
            }
        } 
        else if (!estaVacia(2)) { // Comprobar Cola 2 (SJF)
            if (desencolarProceso(2, proceso_a_despachar)) { //
                proceso_encontrado = true;
            }
        } 
        else if (!estaVacia(3)) { // Comprobar Cola 3 (FCFS)
            if (desencolarProceso(3, proceso_a_despachar)) { //
                proceso_encontrado = true;
            }
        }

        // 2. DESPACHO O ESPERA ACTIVA CONTROLADA
        if (proceso_encontrado) {
            // Se envía el proceso al núcleo de procesamiento simulado
            simularCPU(proceso_a_despachar);
        } else {
            // Evita el consumo destructivo del 100% de la CPU real cuando la planta está inactiva
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    std::cout << "[KERNEL - SCHEDULER]: Hilo maestro del planificador detenido.\n";
}

void MLFQScheduler::simularCPU(PCB* proceso) {
    // Transición de estado administrativo a EJECUTANDO
    proceso->cambiarEstado(ProcessState::EJECUTANDO); //

    std::cout << "\n=========================================================\n"
              << "[KERNEL - DESPACHADOR]: Dispaching -> " << proceso->nombre_fase 
              << " [ID: " << proceso->id << "] desde la COLA " << proceso->prioridad_actual << "\n"
              << "=========================================================\n";

    bool termino_rafaga = false;
    
    // Invocar el método de ejecución y degradación temporal previamente programado
    ejecutarTurno(proceso, termino_rafaga); //

    // Evaluación post-ejecución del ciclo de vida
    if (!termino_rafaga) {
        // Si no terminó su ráfaga total, significa que agotó su Quantum en Cola 1
        // y el método 'ejecutarTurno' ya se encargó de degradarlo y re-encolarlo en la Cola 2.
        std::cout << "[KERNEL - DESPACHADOR]: Proceso " << proceso->id 
                  << " requiere más tiempo. Re-encolado en jerarquía inferior.\n";
    } else {
        // El proceso completó de forma exitosa su ciclo de ráfaga estimado
        proceso->cambiarEstado(ProcessState::BLOQUEADO); // Transición de salida a espera de E/S o nuevo ciclo
        std::cout << "[KERNEL - DESPACHADOR]: Proceso " << proceso->nombre_fase 
                  << " [ID: " << proceso->id << "] completó su ráfaga de CPU virtual de forma exitosa.\n";
        
        // Imprimir log de auditoría final del PCB
        proceso->imprimirLog(); //
    }
}
