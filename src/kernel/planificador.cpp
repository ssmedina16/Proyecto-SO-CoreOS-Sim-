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
