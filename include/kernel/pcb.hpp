#ifndef PCB_HPP
#define PCB_HPP

#include <cstddef>

enum class EstadoProceso { NUEVO, LISTO, EJECUTANDO, BLOQUEADO, TERMINADO };

struct PCB_Venalum {
    int PID;
    EstadoProceso estado;
    int program_counter;
    
    size_t base_limit;     // Gestión de Memoria Virtual
    int cuota_alumina;     // Límite de materiales
    
    // Otros campos pueden añadirse en el Paso 2
};

#endif
