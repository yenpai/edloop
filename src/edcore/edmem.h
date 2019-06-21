#ifndef _EDMEM_H
#define _EDMEM_H

#include "edobject.h"

typedef struct edmem edmem;

_EDOBJECT_EXTEND_METHOD_MACRO_(edmem, edmem);
void *  edmem_pcalloc(edmem * mb, size_t size);
void *  edmem_pmalloc(edmem * mb, size_t size);
void    edmem_pfree(edmem * mb, void * ptr);
edmem * edmem_new(size_t size);

#define EDMEM_DEFAULT_SIZE		(16 * 1024)
#endif
