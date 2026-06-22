#ifndef WRAPPERS_HPP
#define WRAPPERS_HPP

#include "planificador.hpp"

namespace Industrial {
    /**
     * @brief Wrapper para la Fase 1: Planta de Carbón (Horneado y Mezclado).
     * Administra el PCB en la Cola 3 (FCFS) y ejecuta los logs industriales originales.
     */
    void wrapper_fase1_carbon(MLFQScheduler& scheduler);

    /**
     * @brief Wrapper para la Fase 2: Sistema de Logística y Transporte Neumático.
     * Administra el PCB en la Cola 2 (SJF) y procesa los escaneos de tolvas.
     */
    void wrapper_fase2_logistica(MLFQScheduler& scheduler);

    /**
     * @brief Wrapper unificado para la Fase 3 y 4: Celdas de Reducción y GTC.
     * Administra el PCB en la Cola 1 (Round Robin) para control de misión crítica y reciclaje.
     */
    void wrapper_fase3_fase4_celdas(MLFQScheduler& scheduler);
}

#endif // WRAPPERS_HPP