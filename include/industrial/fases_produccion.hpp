#ifndef FASES_PRODUCCION_HPP
#define FASES_PRODUCCION_HPP

namespace Industrial
{
    // Fase 1: Planta de Carbón
    void fase_planta_carbon();

    // Fase 2: Sistema de Logística
    void fase_logistica_transporte();

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
    void fase_reciclaje_gtc();

    // Fase 5: Trasiego del Crisol
    void fase_trasiego_crisol();
}

#endif
