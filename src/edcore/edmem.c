#include "edbase.h"
#include "edmem.h"

typedef struct edmem_small_block edmem_sb;
typedef struct edmem_large_block edmem_lb;

struct edmem_large_block {
	edmem_lb * next;
	void     * ptr;
};	

struct edmem_small_block {
	edmem_sb * next;
	void     * ptr;
	void     * end;
	uint8_t    failed;
};

struct edmem {
	edobject   object;
	size_t     max;
	edmem_lb * large; /* pointer of first large block */
	edmem_sb * small; /* pointer of avail small block */
	edmem_sb   block; /* first small block in edmem */
};

#define EDMEM_PAGE_SIZE			4096
#define EDMEM_ALIGN_SIZE		sizeof(void *)
#define EDMEM_BLOCK_MAX_SIZE	(EDMEM_PAGE_SIZE - 1)

static inline void * edmem_align_ptr(size_t align_size, void * ptr)
{
	return (void *)(((uintptr_t)(ptr) + (uintptr_t)(align_size - 1)) & ~((uintptr_t)(align_size - 1)));
}

static inline void * edmem_memalign_alloc(size_t align_size, size_t size)
{
	void * ptr;

	if (posix_memalign(&ptr, align_size, size) != 0)
		return NULL;

	return ptr;
}

static void * edmem_palloc_block(edmem * mb, size_t size)
{
	edmem_sb * new , * small;
	size_t psize;
	void * ptr;

	psize = (size_t) (mb->block.end - (void *) mb);

	if ((new = edmem_memalign_alloc(EDMEM_ALIGN_SIZE, psize)) == NULL)
		return NULL;

	ptr = new + sizeof(edmem_sb);
	ptr = edmem_align_ptr(EDMEM_ALIGN_SIZE, ptr);

	new->next   = NULL;
	new->ptr    = ptr + size;
	new->end    = new + psize;
	new->failed = 0;

	for (small = mb->small ; small->next ; small = small->next)
	{
		if (small->failed++ > 4)
		{
			mb->small = small->next;
		}
	}

	small->next = new;
	return ptr;
}

static void * edmem_palloc_small(edmem * mb, size_t size)
{
	edmem_sb * small;
	void * ptr;

	for (small = mb->small ; small ; small = small->next)
	{
		ptr = edmem_align_ptr(EDMEM_ALIGN_SIZE, small->ptr);

		if (small->end >= ptr + size)
		{
			small->ptr = ptr + size;
			return ptr;
		}
	}

	return edmem_palloc_block(mb, size);
}

static void * edmem_palloc_large(edmem * mb, size_t size)
{
	edmem_lb * large;
	void * ptr;
	uint8_t n;

	if ((ptr = edmem_memalign_alloc(EDMEM_ALIGN_SIZE, size)) == NULL)
		return NULL;

	/* try to find first 4 large if it could be reuse */
	for (large = mb->large, n = 0 ; large && n < 4 ; large = large->next, n++)
	{
		if (large->ptr == NULL)
		{
			large->ptr = ptr;
			return ptr;
		}
	}

	/* create new large from small block */
	if ((large = edmem_palloc_small(mb, sizeof(edmem_lb))) == NULL)
	{
		free(ptr);
		return NULL;
	}

	large->ptr  = ptr;
	large->next = mb->large;
	mb->large   = large;

	return ptr;
}

void * edmem_pcalloc(edmem * mb, size_t size)
{
	void * ptr;

	if ((ptr = edmem_pmalloc(mb, size)) == NULL)
		return NULL;

	memset(ptr, 0, size);
	return ptr;
}

void * edmem_pmalloc(edmem * mb, size_t size)
{
	return (size <= mb->max) ? 
		edmem_palloc_small(mb, size) : 
		edmem_palloc_large(mb, size) ;
}

void edmem_pfree(edmem * mb, void * ptr)
{
	edmem_lb * large;

	/* find and free the memory that used for large block only */
	for (large = mb->large ; large ; large = large->next)
	{
		if (large->ptr == ptr)
		{
			free(large->ptr);
			large->ptr = NULL;
			return;
		}
	}
}

static void edmem_finalize(edobject * obj)
{
	edmem * mb = container_of(obj, edmem, object);
	edmem_lb * large;
	edmem_sb * block, * next;

	/* free all large block */
	for (large = mb->large ; large ; large = large->next)
	{
		if (large->ptr)
		{
			free(large->ptr);
		}
	}

	/* free all small block except first block in mpool */
	for (block = mb->block.next ; block ; block = next)
	{
		next = block->next;
		free(block);
	}
}

edmem * edmem_new(size_t psize)
{
	edmem * mb;
	size_t size;

	if (psize < sizeof(edmem))
		return NULL;

	if ((mb = edmem_memalign_alloc(EDMEM_ALIGN_SIZE, psize)) == NULL)
		return NULL;

	size = psize - sizeof(edmem);

	memset(mb, 0, sizeof(edmem));
	edobject_init(edmem_to_object(mb));
	edobject_register_finalize(edmem_to_object(mb), edmem_finalize);

	mb->max = (size < EDMEM_BLOCK_MAX_SIZE) ? size : EDMEM_BLOCK_MAX_SIZE ;
	mb->large = NULL;
	mb->small = &mb->block;

	mb->block.ptr    = (uint8_t *) mb + sizeof(edmem);
	mb->block.end    = (uint8_t *) mb + psize;
	mb->block.failed = 0;

	return mb;
}
