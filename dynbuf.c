#include "dynbuf.h"
#include <string.h>
#include <stdint.h>

dynbuf * dynbuf_create(size_t maxsize, dynbuf_adjust_callback_t adjust_callback, void *context)
{
	dynbuf *buf;
	
	buf = malloc(sizeof(dynbuf));
	
	if (!buf)
	{
		return NULL;
	}
	
	buf->data = malloc(maxsize);
	
	if (!(buf->data))
	{
		free(buf);
		return NULL;
	}
	
	buf->size = 0;
	buf->maxsize = maxsize;
	buf->adjust_callback = adjust_callback;
	buf->context = context;
	
	return buf;
}

void dynbuf_free(dynbuf *buf)
{
	if (buf)
	{
		free(buf->data);
		free(buf);
	}
}

void * dynbuf_append(dynbuf *buf, void *data, size_t size)
{
	size_t rem;
	ssize_t offset = 0;
	void *dst;
	
	if (size == 0)
	{
		return ((uint8_t *)buf->data) + buf->size;
	}
	
	rem = buf->maxsize - buf->size;
	
	if (rem < size)
	{
		size_t new_size = buf->size + size + 1024;
		void *new_data, *prev_data;
		
		prev_data = buf->data;
		new_data = realloc(buf->data, new_size);
		
		if (!new_data)
		{
			return NULL;
		}
		
		buf->data = new_data;
		buf->maxsize = new_size;
		
		offset = new_data - prev_data;
	}
	
	dst = (uint8_t *)(buf->data) + buf->size;
	
	if (data)
	{
		memcpy(dst, data, size);
	}
	else
	{
		memset(dst, 0, size);
	}
	
	buf->size += size;
	
	if (offset != 0 && buf->adjust_callback)
	{
		buf->adjust_callback(buf, offset, buf->context);
	}
	
	return dst;
}

int dynbuf_clear(dynbuf *buf)
{
	if (!buf)
	{
		return DYNBUF_ERROR_PARAM;
	}
	
	buf->size = 0;
	return DYNBUF_OK;
}

int dynbuf_trunc(dynbuf *buf)
{
	if (!buf)
	{
		return DYNBUF_ERROR_PARAM;
	}
	
	if (buf->size < buf->maxsize)
	{
		void *prev_data = buf->data;
		void *new_data = realloc(buf->data, buf->size);
		
		if (!new_data)
		{
			return DYNBUF_ERROR_MEMORY;
		}
		
		buf->data = new_data;
		buf->maxsize = buf->size;
		
		if (buf->adjust_callback && new_data != prev_data)
		{
			buf->adjust_callback(buf, new_data - prev_data, buf->context);
		}
	}
	
	return DYNBUF_OK;
}

void dynbuf_adjust_begin(dynbuf_adjust_context *context, dynbuf *buf)
{
	context->prev_data = buf->data;
}

void dynbuf_adjust_update(dynbuf_adjust_context *context, dynbuf *buf, void **ptr)
{
	if (context->prev_data != buf->data && *ptr != NULL)
	{
		*ptr = (((uint8_t *)(*ptr)) + (buf->data - context->prev_data));
		context->prev_data = buf->data;
	}
}
