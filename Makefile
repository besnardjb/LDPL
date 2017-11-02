#MPC
#CC= MPC_UNPRIVATIZED_VARS="__compensation_ticks:stdout_stream" mpc_cc -g
#OTHER MPIS
CC= mpicc -g -ldl

TARGETS=libldpl.so

all: $(TARGETS)


gen_files:
	python ./gen.py

FILES=profile.c timer.c redirect.c

libldpl.so : gen_files
	$(CC) -fpic -shared -o $@ ./GEN_pmpi_func.c $(FILES)

clean:
	rm -fr $(TARGETS) ./GEN_*
