#include "redirect.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

int (*LDPL_realpthread_create)( pthread_t * tg, void *conf , void *(*f)(void *), void *arg);
int (*LDPL_realpthread_join)( pthread_t  tg, void *conf );


struct interceptor_ctx
{
	int fd;
	void (*handler)( char * line, void * hctx );
	void * hctx;
};


void * out_reader( void * pctx )
{

	struct interceptor_ctx * ctx = (struct interceptor_ctx *)pctx;

	int rfd = ctx->fd;
	void (*h)(char *, void * ) = ctx->handler;
	void * hctx = ctx->hctx;

	free( ctx );

	FILE * out = fdopen( rfd , "r" );

	char * ret = NULL;
	char buff[2048];
	
	while( ret = fgets(buff, 2048, out)  )
	{
		(h)(buff, hctx );
	}


	fclose( out );

	return NULL;
}



int redirect_output_stream( int target_fd )
{

	int intercepted_out[2];

	if( pipe(intercepted_out) < 0 )
	{
		perror("pipe");
		abort();
	}


	if( dup2( intercepted_out[1],  fileno( stdout )) < 0 )
	{
		perror("dup2");
		abort();
	}

	close( intercepted_out[1] );

	return intercepted_out[0];
}



int output_stream_interceptor_init(struct output_stream_interceptor *oi,  int target_fd,  void (*handler)(char * line, void * hctx ), void * hctx  )
{
	int rfd = redirect_output_stream( target_fd );

	if( rfd < 0 )
	{
		return 1;
	}

	oi->ref_fd = target_fd;

	struct interceptor_ctx * ctx = malloc( sizeof(struct interceptor_ctx ) );

	if( !ctx )
	{
		perror("malloc");
		return 1;
	}

	ctx->handler = handler;
	ctx->fd = rfd;
	ctx->hctx = hctx;

	void * lh = dlopen("/lib64/libpthread.so.0", RTLD_LAZY );

	if( lh == NULL )
	{
		perror("dlopen");
	}

	LDPL_realpthread_create = (int (*)(pthread_t *, void *, void *(*)(void*), void *))dlsym( lh, "pthread_create" );
	LDPL_realpthread_join = (int(*)(pthread_t, void *))dlsym( lh, "pthread_join" );

	if( lh )
	{
		dlclose( lh );
	}

	if( !LDPL_realpthread_create )
	{
		LDPL_realpthread_create = (int (*)(pthread_t *, void *, void *(*)(void*), void *))pthread_create;
	}

	if( !LDPL_realpthread_join )
	{
		LDPL_realpthread_join = (int(*)(pthread_t, void *))pthread_join;
	}

	if( (LDPL_realpthread_create)(&oi->rthread	, NULL, out_reader, (void *)ctx) < 0 )
	{
		return 1;
	}

	return 0;
}



int output_stream_interceptor_release(struct output_stream_interceptor *oi)
{
	if( !oi )
	{
		return 1;
	}
	
	close(oi->ref_fd);

	(LDPL_realpthread_join)( oi->rthread, NULL	);

	return 0;
}


/* STORAGE */

struct output_line * output_line_new( char * line, int rank, ticks compensation )
{
	struct output_line * ret = malloc( sizeof(struct output_line) );
	
	if( !ret )
	{
		perror("malloc");
		abort();
	}
	
	ret->timestamp = getticks() - compensation;

#ifndef MPC_PRIVATIZED
	PMPI_Comm_rank( MPI_COMM_WORLD, &ret->rank );
#else
	ret->rank = -1;
#endif


	ret->line = strdup( line );
	
	return ret;
}

int output_line_free( struct output_line * ol )
{
	if( !ol )
	{
		return 1;
	}
	
	free( ol->line );
	ol->line = NULL;
	
	free( ol );
	
	return 0;
}

char * output_line_render_json( struct output_line * ol )
{
	char * ret = malloc( 1024 * 1024 );
	
	ret[0] = '\0';
	
	char buff[2048];
	
	strcat( ret, "{\n");
	snprintf(buff, 2048, "\"time\" : %g,\n", (double)ol->timestamp / ticks_per_sec());
	strcat( ret, buff);
	snprintf(buff, 2048, "\"rank\" : %d,\n", ol->rank);
	strcat( ret, buff);
	strcat( ret, "\"line\" : \"");
	ol->line[ strlen(ol->line) - 1] = '\0';
	strcat( ret, ol->line );
	strcat( ret, "\"\n");

	strcat( ret, "\n}\n");
	
	return ret;
}


char * output_line_render( struct output_line * ol )
{
	char * ret = malloc( 20 * 1024 * 1024 );
	
	ret[0] = '\0';
	
	strcat( ret, "[\n");
	
	struct output_line * cur = ol;
	
	while( cur )
	{
		char *ser = output_line_render_json( cur );
		
		strcat( ret, ser );
		
		free( ser );
	
		if( cur->prev )
			strcat( ret, ",\n");
	
		struct output_line * to_free = cur;
		cur = cur->prev;
		//output_line_free( to_free );
	}
	
	strcat( ret, "\n]\n");
	
	return ret;
}


/* This is the STDOUT HANDLING */

static int intercept_stdout_done = -1;
static struct output_stream_interceptor intercept_stdout;
static struct output_line * stdout_stream = NULL;

struct out_hctx
{
	pthread_spinlock_t stream_lock;
	struct output_line ** stream;
	int rank;
	ticks compensation;
};

struct out_hctx * out_hctx_init( void ** stream )
{
	struct out_hctx * ret = malloc( sizeof(struct out_hctx));

	if( !ret )
	{
		perror("malloc");
		abort();
	}

	ret->stream = (struct output_line **)stream;
	pthread_spin_init( &ret->stream_lock, 0);

	PMPI_Comm_rank(MPI_COMM_WORLD, &ret->rank );

	ret->compensation = ticks_comp();

	return ret;
}



void stdout_handler(char * line, void * phctx )
{
	struct out_hctx * hctx = (struct out_hctx *)phctx;
	
	struct output_line * l = output_line_new( line, hctx->rank, hctx->compensation );
	
	l->prev = *hctx->stream;
	*hctx->stream = l;
	
	fprintf(stderr, ":%s", line );
}


int stdout_intercept()
{
	if( getenv("LDPL_NOREDIRECT") )
		return 0;

	if( intercept_stdout_done == -1 )
	{
		PMPI_Comm_rank( MPI_COMM_WORLD , &intercept_stdout_done );
		
		struct out_hctx *hctx = out_hctx_init( (void **)&stdout_stream  );

		output_stream_interceptor_init( &intercept_stdout, fileno(stdout),  stdout_handler, hctx );
	}
	
	return 0;
}

int stdout_flush()
{
	fflush(stdout);
}

int stdout_relax()
{
	fflush(stdout);

	int r = -1;
	PMPI_Comm_rank( MPI_COMM_WORLD , &r );

	if( intercept_stdout_done == r )
	{
		intercept_stdout_done = 0;
		output_stream_interceptor_release( &intercept_stdout );
	}

	return 0;
}


char * stdout_render()
{
	PMPI_Barrier(MPI_COMM_WORLD);
	return output_line_render( stdout_stream );
}



