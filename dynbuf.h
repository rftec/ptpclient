#ifndef __DYNBUF_H__
#define __DYNBUF_H__

#include <stdlib.h>

struct _dynbuf;
typedef struct _dynbuf	dynbuf;

typedef void (*dynbuf_adjust_callback_t)(dynbuf *buf, ssize_t offset, void *context);

struct _dynbuf
{
	void *data;
	size_t size;
	size_t maxsize;
	dynbuf_adjust_callback_t adjust_callback;
	void *context;
};

typedef struct _dynbuf_adjust_context
{
	void *prev_data;
} dynbuf_adjust_context;

#define DYNBUF_OK			0
#define DYNBUF_ERROR_PARAM	-1
#define DYNBUF_ERROR_MEMORY	-2

dynbuf * dynbuf_create(size_t maxsize, dynbuf_adjust_callback_t adjust_callback, void *context);
void dynbuf_free(dynbuf *buf);
void * dynbuf_append(dynbuf *buf, void *data, size_t size);
int dynbuf_clear(dynbuf *buf);
int dynbuf_trunc(dynbuf *buf);
void dynbuf_adjust_begin(dynbuf_adjust_context *context, dynbuf *buf);
void dynbuf_adjust_update(dynbuf_adjust_context *context, dynbuf *buf, void **ptr);

#endif // __DYNBUF_H__
