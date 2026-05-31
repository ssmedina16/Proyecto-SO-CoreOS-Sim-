#ifndef SINCRONIZACION_HPP
#define SINCRONIZACION_HPP

class KMutex {
public:
    KMutex();
    void lock();
    void unlock();
};

class KSemaphore {
public:
    KSemaphore(int initial_count);
    void wait();
    void signal();
};

#endif
