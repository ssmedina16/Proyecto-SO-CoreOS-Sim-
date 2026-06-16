#ifndef FASES_PRODUCCION_HPP
#define FASES_PRODUCCION_HPP

#include <mutex>
#include <fstream>

namespace Industrial
{
    // Inventario para comunicación Fase 3 - Fase 4
    struct InventarioAmbiental
    {
        float gases_acumulados = 0.0f;
        float alumina_enriquecida = 0.0f;
    };

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
    void Hilo_Celda(EstadoCelda &mi_estado, InventarioAmbiental &env, std::mutex &mtx, std::ofstream &log_file);
    //--------------------------------------------------------------

    // Fase 4: Reciclaje GTC
    //--------------------------------------------------------------
    void fase_reciclaje_gtc(InventarioAmbiental &env, std::mutex &mtx);
    //--------------------------------------------------------------

    // Interconexión Fase 2 -> Fase 3 --- VARIABLES DE CONEXIÓN ENTRE PROCESOS ---
    inline float SILO_ALUMINA_GLOBAL = 50000.0f;
    inline const int MAX_CELDAS = 5;
    // Las tolvas de las 5 celdas empezarán vacías (0.0f) para probar la conexión
    inline float TOLVAS_CELDAS_GLOBAL[MAX_CELDAS] = {400.0f, 400.0f, 400.0f, 400.0f, 400.0f};

    // Fase 5: Trasiego del Crisol
    // void fase_trasiego_crisol();
}

#endif
