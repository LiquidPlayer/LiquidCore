// Polyfill for pthread_barrier_* function implementations
// NDK 28 has the pthread_barrier_t type defined but no function implementations
// We provide compatible implementations using mutex and condition variables

#include <pthread.h>
#include <errno.h>

#ifndef PTHREAD_BARRIER_SERIAL_THREAD
#define PTHREAD_BARRIER_SERIAL_THREAD 1
#endif

// Use an internal structure that mirrors what we need
// We'll cast pthread_barrier_t to this internally
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int count;
    unsigned int current;
    unsigned int cycle;
} barrier_impl_t;

int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count) {
    barrier_impl_t *impl = (barrier_impl_t *)barrier;
    int ret;

    if (count == 0) {
        return EINVAL;
    }

    if (sizeof(barrier_impl_t) > sizeof(pthread_barrier_t)) {
        // Our implementation is too large for the system-defined type
        return EINVAL;
    }

    if ((ret = pthread_mutex_init(&impl->mutex, NULL)) != 0) {
        return ret;
    }

    if ((ret = pthread_cond_init(&impl->cond, NULL)) != 0) {
        pthread_mutex_destroy(&impl->mutex);
        return ret;
    }

    impl->count = count;
    impl->current = 0;
    impl->cycle = 0;

    return 0;
}

int pthread_barrier_wait(pthread_barrier_t *barrier) {
    barrier_impl_t *impl = (barrier_impl_t *)barrier;
    int cycle;
    int ret = 0;

    pthread_mutex_lock(&impl->mutex);

    cycle = impl->cycle;

    if (++impl->current >= impl->count) {
        impl->cycle++;
        impl->current = 0;
        pthread_cond_broadcast(&impl->cond);
        ret = PTHREAD_BARRIER_SERIAL_THREAD;
    } else {
        while (cycle == impl->cycle) {
            pthread_cond_wait(&impl->cond, &impl->mutex);
        }
    }

    pthread_mutex_unlock(&impl->mutex);

    return ret;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
    barrier_impl_t *impl = (barrier_impl_t *)barrier;
    int ret1, ret2;

    ret1 = pthread_cond_destroy(&impl->cond);
    ret2 = pthread_mutex_destroy(&impl->mutex);

    return (ret1 != 0) ? ret1 : ret2;
}
