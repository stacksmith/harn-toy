# harn-toy

harn-toy implements an interactive toy linker, capable of loading and linking one or more elf object files, and generating library bindings to Linux DSOs for linkage.  Unlike normal linkers which stop at creating an executable file, harn keeps linked code and data in-memory along with public symbols, allowing further interaction.

Harn-toy has achieved its purpose of loading, linking and relocating a provided set of two ELF objects, cross-dependent and libc-dependent.

It is a non-goal to create a full-scale linker capable of doing the job of ld, lld, or gold.  harn is built to operate on a small subset of code generated in a very specific way.
```
pushd o; make all; popd
make
# a simple hello world
harn o/test2.o
# a two-object cross-dependent example
harn o/twoA.o o/twoB.o
```
