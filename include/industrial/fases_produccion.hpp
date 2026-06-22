#ifndef FASES_PRODUCCION_HPP
#define FASES_PRODUCCION_HPP

#include <mutex>
#include <fstream>

namespace Industrial
{
    // Fase 1: Planta de Carbón
    //------------------------------------------------------------
    struct EstadoPlanta
    {
        int pid_proceso;
        float coque_kg;        // Materia prima 1
        float brea_kg;         // Materia prima 2
        int anodos_producidos; // Producto terminado para Fase 3
    };

    void fase_planta_carbon();
    void Hilo_Mezcladora(EstadoPlanta &estado);
    void Hilo_Horno(EstadoPlanta &estado);
    //------------------------------------------------------------


    // Fase 2: Sistema de Logística
    //------------------------------------------------------------
    void fase_logistica_transporte();
    //------------------------------------------------------------

    

    // Fase 3: Celdas de Reducción
    //------------------------------------------------------------
    struct EstadoCelda
    {
        int id_celda;
        float temperatura_bano;   // Debe mantenerse cerca de 958 °C
        float alumina_kg;         // Inventario local de Alúmina
        float anodo_carbon_kg;    // Inventario local de Carbón
        float aluminio_producido; // Aluminio líquido acumulado
    };

    void fase_celdas_reduccion(int cantidad_celdas);
    void Hilo_Celda(EstadoCelda &mi_estado, std::mutex &mtx, std::ofstream &log_file);
    //--------------------------------------------------------------

    // Fase 4: Reciclaje GTC
    //--------------------------------------------------------------
    void fase_reciclaje_gtc(std::mutex &mtx);
    //--------------------------------------------------------------

    /** 
     * @brief Estructura de Memoria Virtual Compartida POSIX (IPC)
     * Unifica los recursos compartidos entre procesos independientes (Fase 1, 2 y 3) en RAM real.
     */
    struct MemoriaCompartidaPlanta {
        float silo_alumina;        // Inventario global gestionado por Fase 2
        float tolvas_celdas[5];    // Tolvas de alúmina para cada celda (Fase 2 -> Fase 3)
        int anodos_producidos;     // Stock de ánodos listos (Fase 1 -> Fase 3)
        
        // --- Integración Fase 4 (Reciclaje GTC y Garbage Collection) ---
        float gases_acumulados;      // Gases emitidos por las celdas (basura a recolectar por Fase 4)
        float alumina_enriquecida;   // Materia prima reciclada lista para usarse en las celdas

        // --- Integración Fase 5 ---
        float aluminio_producido[5]; // Aluminio líquido acumulado en cada celda (Fase 3 -> Fase 5)
    };

    /** 
     * @brief Puntero global a la memoria compartida mapeada.
     * Debe ser inicializado en main.cpp mediante shm_open y mmap.
     */
    extern MemoriaCompartidaPlanta* shared_planta;

    inline const int MAX_CELDAS = 5;

    // Fase 5: Trasiego del Crisol
    //--------------------------------------------------------------
    struct EstadoCrisol
    {
        int id_crisol;
        float capacidad_max = 6000.0f;       // Umbral crítico de succión (6 toneladas lógicas)
        float aluminio_recolectado = 0.0f;   // Registro del material consolidado
        bool en_operacion = false;           // Semáforo interno de ejecución
    };

    // Objeto Mutex global que restringe el acceso al canal físico de extracción
    extern std::mutex mutex_crisol_succion;

    // Proceso Global de Logística
    void fase_trasiego_crisol(int cantidad_crisoles, EstadoCelda* celdas, int cantidad_celdas);
    
    // Unidad Ligera de Ejecución (Hilo del Crisol)
    void Hilo_Crisol(EstadoCrisol &mi_estado, EstadoCelda* celdas, int cantidad_celdas);
    //--------------------------------------------------------------
}

#endif
