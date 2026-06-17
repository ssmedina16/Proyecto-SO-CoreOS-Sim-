#include "../../include/kernel/planificador.hpp"
#include <iostream>

void MLFQScheduler::encolarProceso(PCB proceso, int numero_cola) {
    // Actualizamos la prioridad del PCB según la cola donde se inserta
    proceso.prioridad_actual = numero_cola;
    
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

bool MLFQScheduler::desencolarProceso(int numero_cola, PCB& proceso_out) {
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

void MLFQScheduler::ejecutarTurno(PCB& proceso, bool& termino_rafaga) {
    // Definición del Quantum estricto para la Cola 1 (Round Robin)
    const double time_slice = 10.0;

    // --- COLA 1: POLÍTICA ROUND ROBIN CON DEGRADACIÓN ---
    if (proceso.prioridad_actual == 1) {
        if (proceso.rafaga_estimada > time_slice) {
            // El proceso consume todo el Quantum sin terminar su trabajo
            proceso.tiempo_ejecutado += time_slice;
            proceso.rafaga_estimada -= time_slice;
            termino_rafaga = false;

            // Degradación: Al superar el time_slice, se penaliza re-encolándolo en la Cola 2 (SJF)
            std::cout << "[MLFQ CPU] Proceso ID " << proceso.id 
                      << " agotó su quantum en Cola 1. Degradando a Cola 2.\n";
            encolarProceso(proceso, 2);
        } 
        else {
            // El proceso termina dentro o justo en el límite del Quantum
            proceso.tiempo_ejecutado += proceso.rafaga_estimada;
            proceso.rafaga_estimada = 0.0;
            termino_rafaga = true;
        }
    }
    // --- COLAS 2 Y 3: POLÍTICAS NO APROPIATIVAS (SJF / FCFS) ---
    else if (proceso.prioridad_actual == 2 || proceso.prioridad_actual == 3) {
        // Al no estar limitadas por rodajas de tiempo, ejecutan toda su ráfaga restante de forma continua
        proceso.tiempo_ejecutado += proceso.rafaga_estimada;
        proceso.rafaga_estimada = 0.0;
        termino_rafaga = true;
    }
}

void MLFQScheduler::forzarRetornoPrioridad() {
    PCB proceso_aux;

    // 1. Vaciar la Cola 2 y pasar todo a la Cola 1
    while (desencolarProceso(2, proceso_aux)) {
        std::cout << "[MLFQ BOOST] Promoviendo Proceso ID " << proceso_aux.id << " de Cola 2 a Cola 1 por envejecimiento.\n";
        encolarProceso(proceso_aux, 1);
    }

    // 2. Vaciar la Cola 3 y pasar todo a la Cola 1
    while (desencolarProceso(3, proceso_aux)) {
        std::cout << "[MLFQ BOOST] Promoviendo Proceso ID " << proceso_aux.id << " de Cola 3 a Cola 1 por envejecimiento.\n";
        encolarProceso(proceso_aux, 1);
    }
}
