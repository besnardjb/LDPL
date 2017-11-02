#ifndef REDIRECT_H
#define REDIRECT_H

#include <mpi.h>
#include <pthread.h>

#include "timer.h"

/* INTERCEPTION */

struct output_stream_interceptor
{
	pthread_t rthread;
	int ref_fd;
};

int output_stream_interceptor_init(struct output_stream_interceptor *oi,  int target_fd,  void (*handler)(char * line, void * hctx ), void * hctx  );
int output_stream_interceptor_release(struct output_stream_interceptor *oi);

/* STORAGE */

struct output_line
{
	ticks timestamp;
	int rank;
	char * line;
	struct output_line * prev;
};


struct output_line * output_line_new( char * line, int rank, ticks compensation );
int output_line_free( struct output_line * ol );
char * output_line_render_json( struct output_line * ol );



/* USAGE FOR STDOUT */


int stdout_intercept();
int stdout_flush();
int stdout_relax();
char * stdout_render();

#endif /* REDIRECT_H */
