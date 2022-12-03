# harn

harn is a project implementing an interactive toy linker, capable of loading and linking one or more elf object files, and generating library bindings to Linux DSOs for linkage.  Unlike normal linkers which stop at creating an executable file, harn keeps linked code and data in-memory along with public symbols, allowing further interaction.

The goal is to have a REPL-like system that allows 
* re-linking of ELF object components;
* finer-grain re-linking of individual functions and data
* a REPL-like environment for debugging and manipulating code and data

It is a non-goal to create a full-scale linker capable of doing the job of ld, lld, or gold.  harn is built to operate on a small subset of code generated in a very specific way.

pushd o; make all; popd
make
# a simple hello world
harn o/test2.o
# a two-object cross-dependent example
harn o/twoA.o o/twoB.o

Current state:
2022-12-02 linking object files, resolving dependencies to libc and any internal dependenices including cross-object-file dependencies, relocating.  Can find any public symbol and execute it (currently running 'bar').
