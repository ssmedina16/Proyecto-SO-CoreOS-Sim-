#ifndef PLANIFICADOR_HPP
#define PLANIFICADOR_HPP

#include "pcb.hpp"
#include <queue>

struct MLFQ {
    std::queue<PCB_Venalum*> cola_1; // Prioridad Alta (Round Robin)
    std::queue<PCB_Venalum*> cola_2; // Prioridad Media (SJF)
    std::queue<PCB_Venalum*> cola_3; // Prioridad Baja (FCFS)
};

// Firmas para el control de colas
void inicializar_planificador();
void planificar_siguiente();

#endif
