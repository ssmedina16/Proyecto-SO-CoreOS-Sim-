#ifndef PCB_HPP
#define PCB_HPP

#include <cstddef>

enum class EstadoProceso { NUEVO, LISTO, EJECUTANDO, BLOQUEADO, TERMINADO };

struct PCB_Venalum {
    int PID;                  // ID único de identificación
    int Tipo_Entidad;         // Clasificación (0 = Celda, 1 = Horno, 2 = Crisol)
    EstadoProceso estado;     // Estado lógicos en el planificador
    int program_counter;      // Registro del contador de programa
    
    size_t base_limit;        // Segmento de memoria virtual aislado
    int cuota_alumina;        // Límite de seguridad de asignación física
    
    // Sincronización e Interrupciones de E/S
    int esperando_por;        // Almacena el ID del Mutex o recurso que causó el BLOQUEADO
};

#endif
