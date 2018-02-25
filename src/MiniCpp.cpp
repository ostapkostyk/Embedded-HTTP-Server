/*
 * MiniCpp.cpp
 *
 */

// Minimal embedded C++ support, without exception handling, without RTTI

#include <stdlib.h>                     //  for prototypes of malloc() and free()

//============================================================================
extern "C" void *malloc(size_t)
{
    return (void *)0;
}
//============================================================================
extern "C" void free(void *)
{

}

//============================================================================
void *operator new(size_t size) throw()
{
    return malloc(size);
}

//============================================================================
void operator delete(void *p) throw()
{
    free(p);
}

//============================================================================
extern "C" int __aeabi_atexit(void *object,
                              void (*destructor)(void *),
                              void *dso_handle)
{

    return 0;
}

extern "C"{
    void __cxa_atexit() {
    }
}

void __register_exitproc() {
    }

extern "C" void __cxa_pure_virtual() { while(1); }
