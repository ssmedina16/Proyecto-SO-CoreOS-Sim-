#ifndef PCB_HPP
#define PCB_HPP

#include <string>
#include <iostream>
#include <iomanip>

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

/**Enumeración fuerte para los estados del proceso.*/
enum class ProcessState { 
    LISTO, 
    EJECUTANDO, 
    BLOQUEADO 

};

/**Bloque de Control de Proceso (PCB)
 *Contiene la información administrativa de cada fase del simulador industrial.
 */
struct PCB {
    int id;                         ///< Identificador único del proceso
    std::string nombre_fase;         ///< Nombre de la fase de la planta
    ProcessState estado;            ///< Estado actual del proceso
    int prioridad_actual;           ///< Nivel de cola actual (1, 2 o 3)
    double tiempo_ejecutado;        ///< Tiempo total acumulado de CPU
    double rafaga_estimada;         ///< Duración estimada de la ráfaga
    int esperando_por = 0;          ///< ID del mutex/semáforo que bloquea este PCB (0 = libre)
    ProcessState* estado_compartido = nullptr; ///< Puntero al estado original en el wrapper

    /**Cambia el estado del proceso de forma segura.
     * nuevo_estado El nuevo estado a asignar.
     */
    void cambiarEstado(ProcessState nuevo_estado) {
        this->estado = nuevo_estado;
        if (estado_compartido != nullptr) {
            *estado_compartido = nuevo_estado;
        }
    }

    /**Imprime en consola los datos del PCB de manera formateada para auditoría.
     */
    void imprimirLog() const {
        std::string str_estado;
        switch (estado) {
            case ProcessState::LISTO:     str_estado = "LISTO"; break;
            case ProcessState::EJECUTANDO: str_estado = "EJECUTANDO"; break;
            case ProcessState::BLOQUEADO:  str_estado = "BLOQUEADO"; break;
        }

        std::cout << "[LOG PCB] ID: " << id 
                  << " | Fase: " << nombre_fase 
                  << " | Estado: " << str_estado 
                  << " | Cola: " << prioridad_actual 
                  << " | Tiempo: " << std::fixed << std::setprecision(2) << tiempo_ejecutado 
                  << " | Ráfaga: " << rafaga_estimada << std::endl;
    }
};

#endif // PCB_HPP

