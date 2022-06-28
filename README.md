# dthumb
ARMv5TE disassembler  

Target architecture: ARMv5TE  
Target processor: ARM946E-S (Nintendo DS main CPU)  

Each file **needs** to have a valid extension (at least one "dot" character in the name).
If no output file is provided, it will print to ``stdout``.  
You can optionally provide a range of addresses to be disassembled (in hexadecimal).
By default, the disassembly mode is THUMB, but you can use ``/a`` as an optional argument to disassemble in ARM mode.
```
dthumb <filein> {<fileout>} {<start>-<end>} {/a}
```
