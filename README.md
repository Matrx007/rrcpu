# rrcpu

This repository contains the specification and an emulator for my custom processor which in future will be put onto an FPGA. The processor will have virtual-address space (MMU), software & hardware interrupts, higher privileges section (in my use case, for the kernel) and built-in stack instsructions (may be removed in future).

This processor is designed to run my version of assembly ([Matrx007/rrasm](https://github.com/Matrx007/rrasm)) which also is the compile target of my custom compiler ([Matrx007/rrlang](https://github.com/Matrx007/rrlang));

# Emulator

```
gcc -Wall src/emulator.c -o emulator
```
or
```
clang -Wall src/emulator.c -o emulator
```

To test AST parser, run
```
./emulator tests/std_strings.rr
```

## Roadmap

1. Document micro-instructions: ✅ Done  
2. Implement basic instructions: 🛠 W.I.P  
3. Interrupts: ❌ TO-DO  
4. Display output & user input: ❌ TO-DO
