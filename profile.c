#include "profile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "redirect.h"

int profile_entry_reduce( struct profile_entry * pe, struct profile_entry * out )
{

	if( !out )
		out = pe;

	PMPI_Reduce( &pe->call_count, &out->call_count , 1, MPI_LONG_LONG_INT, MPI_SUM,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->first_call_ts, &out->first_call_ts , 1, MPI_DOUBLE, MPI_MIN,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->last_call_ts, &out->last_call_ts , 1, MPI_DOUBLE, MPI_MAX,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->total_time, &out->total_time , 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->min_time, &out->min_time , 1, MPI_DOUBLE, MPI_MIN,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->max_time, &out->max_time , 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->total_size, &out->total_size , 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->min_size, &out->min_size , 1, MPI_DOUBLE, MPI_MIN,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->max_size, &out->max_size , 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->max_burst, &out->max_burst , 1, MPI_DOUBLE, MPI_MAX,  0, MPI_COMM_WORLD );	
	PMPI_Reduce( &pe->min_burst, &out->min_burst , 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );	

	return 0;
}

#define BURST_WINDOW 3e9

char *profile_entry_render( struct profile_entry * pe )
{
	char * ret = malloc( 1024 );

	if( !ret )
	{
		perror("malloc");
		abort();
	}

	ret[0] = '\0';

	snprintf( ret, 1024, "\t{\n\t\"call\" : %lld ,\n \t\"first_call\" : %g ,\n \t\"last_call\" : %g ,\n \t\"total_time\" : %g,\n \t\"min_time\" : %g ,\n \t\"max_time\" : %g ,\n \t\"total_size\" : %g,\n \t\"min_size\" : %g,\n \t\"max_size\" : %g,\n\t\"min_burst\" : %g,\n\t\"max_burst\" : %g\n \t}", pe->call_count, pe->first_call_ts / ticks_per_sec(), pe->last_call_ts / ticks_per_sec(), pe->total_time / ticks_per_sec(), pe->min_time / ticks_per_sec(), pe->max_time / ticks_per_sec() , pe->total_size, pe->min_size, pe->max_size, pe->min_burst * ticks_per_sec() / BURST_WINDOW, pe->max_burst* ticks_per_sec() / BURST_WINDOW);

	return ret;
}

struct profile_array * profile_array_init()
{
	struct profile_array * ret = malloc( sizeof(struct profile_array));

	if( ret == NULL)
	{
		perror("malloc");
		return NULL;
	}

	memset(ret, 0, sizeof(struct profile_array));

	int i;
	for( i = 0 ; i < LDPL_FUNC_COUNT ; i++ )
	{
		ret->funcs[i].last_hit = -1;
	}

	pthread_spin_init(&ret->lock, 0);

	/* Get TS for latter TPS callibration */
	ticks_per_sec_start_cal();

	PMPI_Comm_rank( MPI_COMM_WORLD, &ret->my_rank );

	return ret;
}



int profile_array_reduce( struct profile_array * in , struct profile_array * out )
{
	int i;
	for (i = 0; i < LDPL_FUNC_COUNT ; ++i)
	{
		profile_entry_reduce( &in->funcs[i], &out->funcs[i] );
	}

	return 0;
}

char * profile_array_render_json( struct profile_array * pa )
{
	char * ret = malloc( 5 * 1024 * 1024 );

	if( !ret )
	{
		perror("malloc");
		abort();
	}

	ret[0] = '\0';

	strcat( ret, "{\n");

	int i;
	for (i = 0; i < LDPL_FUNC_COUNT ; ++i)
	{
		{
			char * ent = profile_entry_render( &pa->funcs[i] );
			strcat( ret , "   \"" );	
			strcat( ret , LDPL_func_names[i] );	
			strcat( ret ,"\" :\n" );	
			strcat( ret , ent );	
			strcat( ret ,",\n" );	
			free( ent );	
		}	
	}




	strcat( ret, "   \"NULL\" : {}\n}\n");


	return ret;
}


char * profile_array_render( struct profile_array * pa )
{
	if( !pa )
		return NULL;


	/* Root node */
	struct profile_array * root = profile_array_init();

	profile_array_reduce( pa , root );


	if(pa->my_rank == 0)
	{

		char * rpa = profile_array_render_json( root );
		return rpa;

	}

	profile_array_release( &root );

	return NULL;
}

int profile_array_release( struct profile_array ** pa )
{
	if( !pa )
		return 1;

	if( !*pa )
	{
		return 1;
	}


	pthread_spin_destroy(&(*pa)->lock);

	free( *pa );

	*pa = NULL;

	return 0;
}

int profile_array_hit_time( struct profile_array * pa , 
		LDPL_func func ,
		ticks duration )
{
	if( LDPL_FUNC_COUNT <= func )
		return 1;

	struct profile_entry * pe = &(pa->funcs[func]);

	pe->call_count++;

	ticks now = ldpl_getticks();

	if(pe->last_hit < 0 )
	{
		pe->last_hit = now;
	}
	else
	{
		if( BURST_WINDOW < (now - pe->last_hit))
		{
			/* Over one second report burst */
			if( (pe->min_burst == 0) || (pe->hits_count < pe->min_burst) )
			{
				pe->min_burst = pe->hits_count;
			}

			if( (pe->max_burst == 0) || ( pe->max_burst < pe->hits_count ) )
			{
				pe->max_burst = pe->hits_count;
			}


			pe->last_hit = now;
			pe->hits_count = 0;
		}
		else
		{
			/* Less than 1 second accumulate burst */
			pe->hits_count++;
		}
	}

	if( pe->first_call_ts == 0 )
	{
		pe->first_call_ts = now;
	}

	pe->last_call_ts = now;

	if( (pe->min_time == 0) || (pe->min_time < duration) )
	{
		pe->min_time = duration;
	}

	if( (pe->max_time == 0) || (pe->max_time < duration) )
	{
		pe->max_time = duration;
	}

	pe->total_time += duration;

	return 0;
}


int profile_array_hit_size( struct profile_array * pa , 
		LDPL_func func ,
		double size )
{
	if( LDPL_FUNC_COUNT <= func )
		return 1;

	struct profile_entry * pe = &(pa->funcs[func]);

	if( (pe->min_size == 0) || (pe->min_size < size) )
	{
		pe->min_size = size;
	}

	if( (pe->max_size == 0) || (pe->max_size < size) )
	{
		pe->max_size = size;
	}

	pe->total_size += size;

	return 0;
}


struct profile_array * __profile = NULL;


void INIT()
{
	__profile = profile_array_init();
	stdout_intercept();

}

void render()
{
	int rank;
	PMPI_Comm_rank( MPI_COMM_WORLD, &rank );
	
	char * prof = profile_array_render( __profile );
	char * std = stdout_render();
	
	if( rank == 0 )
	{
		char fname[100];

		char * param = getenv("LDPL_OUT");


		if( param )
		{
			snprintf( fname, 100 , "%s" , param );
		}
		else
		{
			char * pcvs = getenv("PCVS_TESTCASE");

			if( pcvs )
			{
			
				snprintf( fname, 100 , "%s.ldpl", pcvs );
			}
			else
			{
				snprintf( fname, 100 , "log.ldpl" );
			}

		}


		FILE * out = fopen( fname, "w");
		fprintf(out, "{\"profile\" : %s ,\n \"stdout\" : %s }\n", prof, std );
		fclose( out );
	}
	
	free( prof );
	free( std );
	
	
}

void RELEASE()
{
	
	/* Compute TPS */
	ticks_per_sec_end_cal();

	/* Flush the output */
	stdout_flush();
	

	render();


	profile_array_release( &__profile );
	stdout_relax();
}

void ENTER(LDPL_func f)
{
	if( !__profile )
		return;

	/* ENTER FUNC */
	//pthread_spin_lock(&__profile->lock);

	__profile->last_ts = ldpl_getticks();

	//printf("E %s\n", LDPL_func_names[f]);
}

void LEAVE(LDPL_func f)
{
	if( !__profile )
		return;
	//printf("L %s\n", LDPL_func_names[f]);


	ticks now = ldpl_getticks();
	ticks duration = now - __profile->last_ts;

	profile_array_hit_time( __profile , f, duration );

	/* DONE */
	//pthread_spin_unlock(&__profile->lock);
}

