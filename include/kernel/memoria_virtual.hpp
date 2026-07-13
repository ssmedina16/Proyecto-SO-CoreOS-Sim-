#ifndef MEMORIA_VIRTUAL_HPP
#define MEMORIA_VIRTUAL_HPP

#include "../industrial/fases_produccion.hpp"

namespace Industrial {
    /**
     * @brief Inicializa el objeto de memoria compartida POSIX y lo mapea en el espacio de direcciones.
     * @return void* Puntero a la memoria mapeada o MAP_FAILED en caso de error.
     */
    void* inicializar_memoria_virtual();

    /**
     * @brief Libera la memoria compartida (munmap) y elimina el objeto del sistema (shm_unlink).
     */
    void liberar_memoria_virtual();
}

#endif // MEMORIA_VIRTUAL_HPP
