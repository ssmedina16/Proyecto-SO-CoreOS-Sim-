#include "../../include/industrial/fases_produccion.hpp"
#include <thread>
#include <chrono>
#include <signal.h>

using namespace std;

extern volatile sig_atomic_t system_running;

namespace Industrial
{
    void fase_reciclaje_gtc(InventarioAmbiental &env, mutex &mtx)
    {
        while (system_running)
        {
            this_thread::sleep_for(chrono::seconds(5));

            {
                lock_guard<mutex> lock(mtx);
                if (env.gases_acumulados >= 100.0f)
                {
                    float gases_capturados = env.gases_acumulados;
                    env.gases_acumulados = 0.0f;
                    env.alumina_enriquecida += (gases_capturados * 0.5f);
                }
            }
        }
    }
}
