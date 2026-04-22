/* Copyright (c) 2026 Aaron W. Hsu */

#define FENSTER_API __declspec(dllexport)

#include "fenster.h"
#include <stdlib.h>
#include <string.h>

#define DATA(pp) ((void *)&(pp)->shape[(pp)->rank])

struct pocket {
	long    long length;
	long    long refcount;
	unsigned        int type        : 4;
	unsigned        int rank        : 4;
	unsigned        int eltype      : 4;
	unsigned        int _0          : 13;
	unsigned        int _1          : 16;
	unsigned        int _2          : 16;
	long    long shape[1];
};

struct localp {
	struct pocket *pocket;
	void *i;
};

FENSTER_API int
DyalogGetInterpreterFunctions(void *p)
{
	return 0;
}


FENSTER_API struct fenster *
fenster_open_apl(const char *title, int width, int height, uint16_t *buf)
{
	struct fenster *f;
	void *bufp, *titlep;
	size_t sz, bs;
	
	bs = width*height*sizeof(*f->buf);
	sz = sizeof(*f)+strlen(title)+1+bs;
	
	if ((f = malloc(sz)) == NULL)
		return 0;
	
	bufp = &f[1];
	titlep = ((char *)bufp)+bs;
	strcpy(titlep, title);
	
	struct fenster fv = {titlep, width, height, bufp, 0};
	memcpy(f, &fv, sizeof(*f));
	
	for (int i = 0; i < f->height * f->width; i++) {
		uint32_t p;
		int j;
		
		j = i * 3;
		
		p  = (uint32_t)buf[j + 0] << 16;
		p |= (uint32_t)buf[j + 1] << 8;
		p |= (uint32_t)buf[j + 2];
		
		f->buf[i] = p;
	}

	if (fenster_open(f)) {
		free(f);
		return NULL;
	}
	
	return f;
}

FENSTER_API int
fenster_loop_apl(struct fenster *f, struct localp *lp,
    int keys[256], int *mod, int *x, int *y, int *mouse)
{
	uint16_t *buf = DATA(lp->pocket);
	
	for (int i = 0; i < f->height * f->width; i++) {
		uint32_t p;
		int j;
		
		j = i * 3;
		
		p  = (uint32_t)buf[j + 0] << 16;
		p |= (uint32_t)buf[j + 1] << 8;
		p |= (uint32_t)buf[j + 2];
		
		f->buf[i] = p;
	}
	
	if (fenster_loop(f))
		return 1;
	
	memcpy(keys, f->keys, 256 * sizeof(*keys));
	*mod = f->mod;
	*x = f->x;
	*y = f->y;
	*mouse = f->mouse;
	
	return 0;
}

FENSTER_API void
fenster_close_apl(struct fenster *f)
{
	fenster_close(f);
	free(f);
}
