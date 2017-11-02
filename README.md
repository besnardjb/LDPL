DESCRIPTION
===========

LDPL is a differential profiler allowing to compare multiple runs of the same
MPI programs. Its interface aims at describing the time spent in MPI calls,
ranking from the faster to the slowest run.

Moreover, LDPL is able to track program output to compare timings in the
web-interface.

INSTALL
======

Just build the interposition library:

```
make
```

USAGE
=====

Then preload it to your target MPI program (LDPL must be built with the same MPI
implementation)


```
LD_PRELOAD=$PWD/libldpl.so mpirun -np 8 ./a.out
```

This will create a .ldpl file in current directory. File that you can load and
explore using the interface in the render.html file.

Licence
=======

This code is CECILL-C a fully LGPL compatible licence.
