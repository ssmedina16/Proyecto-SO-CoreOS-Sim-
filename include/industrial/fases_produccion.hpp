#ifndef FASES_PRODUCCION_HPP
#define FASES_PRODUCCION_HPP

#include <mutex>

namespace Industrial
{
    // Fase 1: Planta de Carbón
    //------------------------------------------------------------
    struct EstadoPlanta
    {
        int pid_proceso;
        float coque_kg;           // Materia prima 1
        float brea_kg;            // Materia prima 2
        int anodos_producidos;    // Producto terminado para Fase 3
    };

    void fase_planta_carbon();
    void Hilo_Mezcladora(EstadoPlanta &estado);
    void Hilo_Horno(EstadoPlanta &estado);
    //------------------------------------------------------------

    // Fase 2: Sistema de Logística
   // void fase_logistica_transporte();

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
    void Hilo_Celda(EstadoCelda &mi_estado);
    //--------------------------------------------------------------

    // Fase 4: Reciclaje GTC
    //void fase_reciclaje_gtc();

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
