#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>
#include <pthread.h>

#include "timer.h"

#include "GEN_ldpl_meta.h"

void INIT();
void RELEASE();

void ENTER(LDPL_func f);
void LEAVE(LDPL_func f);


struct profile_entry
{
	uint64_t call_count;

	double first_call_ts;
	double last_call_ts;

	double total_time;
	double max_time;
	double min_time;

	double total_size;
	double max_size;
	double min_size;

	double max_burst;
	double min_burst;

	double last_hit;
	size_t hits_count;
};


int profile_entry_reduce( struct profile_entry * pe, struct profile_entry * out );
char *profile_entry_render( struct profile_entry * pe );




struct profile_array
{
	int my_rank;
	ticks last_ts;
	pthread_spinlock_t lock;
	struct profile_entry funcs[LDPL_FUNC_COUNT];
};

struct profile_array * profile_array_init();

int profile_array_reduce( struct profile_array * pa, struct profile_array *out );

char * profile_array_render_json( struct profile_array * pa );

char * profile_array_render( struct profile_array * pa );

int profile_array_release( struct profile_array ** pa );

int profile_array_hit_time( struct profile_array * pa , LDPL_func func , ticks duration );

int profile_array_hit_size( struct profile_array * pa , LDPL_func func , double size );

#endif /* end of include guard: PROFILE_H */
