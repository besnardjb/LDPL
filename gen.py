#!/usr/bin/env python 
import json
import sys
import os
import string

scriptpath = os.path.dirname(os.path.realpath(sys.argv[0]))

# ALL MPI function footprints
with open(scriptpath + '/mpi_interface.json') as data_file:    
        mpi_interface = json.load(data_file)


#GEN FUNCTION LIST

LIST=""

for f in mpi_interface:
    LIST += "FUNC(" + f + ")\n"


 
f = open("GEN_mpi_func_list.h", "w" )
 
f.write( LIST );
 
f.close()


FNAMES="#ifndef LDPL_META_H\n"
FNAMES+="#define LDPL_META_H\n"
FNAMES+="#include <stdlib.h>\n"
FNAMES+="""
#define FUNC(a) LDPL_FUNC_ ## a,\n
typedef enum{\n
#include \"GEN_mpi_func_list.h\"\n
LDPL_FUNC_COUNT
}LDPL_func;\n"""

def ldpl_name(fname):
    return "LDPL_FUNC_"+fname



FNAMES+="""
#undef FUNC
#define FUNC(a) #a,\n
static const char * const LDPL_func_names[LDPL_FUNC_COUNT + 1] = {\n
#include \"GEN_mpi_func_list.h\"\n
NULL
};\n
#undef FUNC\n"""

 
FNAMES+="#endif\n"

f = open("GEN_ldpl_meta.h", "w" )
 
f.write( FNAMES );
 
f.close()


#GEN PMPI IFACE

IFACE="#define const\n"
IFACE+="#include <mpi.h>\n"
IFACE+="#include \"profile.h\"\n"



IFACE+="\nvoid ENTER( LDPL_func func );\n"
IFACE+="void LEAVE( LDPL_func func );\n"

for f in mpi_interface:
    fd = mpi_interface[f]
    rtype=""
    if(f=="MPI_Wtime"):
        rtype = "double"
    else:
        rtype = "int"
    IFACE+= "\n\n" + rtype + " " + f + "("
    for i in range(0, len(fd)):
        arg=fd[i]
        name = arg[1];
        ctype = arg[0];
        #BOOL
        if ctype == "bool":
            ctype = "int*"
        #ARRAY CASE
        array=""
        try:
            idx=ctype.index("[")
        except ValueError:
            idx=-1
        if idx!=-1:
            array=ctype[idx:]
            ctype=ctype[0:idx]
        IFACE+=" " + ctype + " " + name + array
        if i < (len(fd) - 1):
            IFACE += ","
        
    IFACE+= ")\n{\n"
    if (f != "MPI_Init") and (f != "MPI_Init_thread"):
        IFACE+= "\tENTER(" + ldpl_name(f) + ");\n"
    if (f == "MPI_Finalize"):
        IFACE+= "\tLEAVE(" + ldpl_name(f) + ");\n"
        IFACE+="\tRELEASE();\n"
    IFACE+= "\t" + rtype + " ret = P" + f + "("
    for i in range(0, len(fd)):
        arg=fd[i]
        name = arg[1];
        IFACE+=name
        if i < (len(fd) - 1):
            IFACE += ","
    IFACE += ");\n"
    if (f == "MPI_Init") or (f == "MPI_Init_thread"):
        IFACE+="\tINIT();"
        IFACE+= "\tENTER(" + ldpl_name(f) + ");\n"
        IFACE+= "\tLEAVE(" + ldpl_name(f) + ");\n"
    else:
        if (f != "MPI_Finalize"):
            IFACE+= "\tLEAVE(" + ldpl_name(f) + ");\n"
    IFACE+= "\n\treturn ret;\n"
    IFACE+= "}\n"


 
f = open("GEN_pmpi_func.c", "w" )
 
f.write( IFACE );
 
f.close()


