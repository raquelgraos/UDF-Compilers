# UDF-Compilers

UDF compiler (based on CDK)

## HOW TO
In order to compile a program written in the UDF language:

1. Write a example.udf
2. ./udf --target asm example.udf
3. yasm -gdwarf2 -felf32 example.asm
4. ld -m elf_i386 -o example example.o -L$HOME/compiladores/root/usr/lib -lrts (this might require some changing)