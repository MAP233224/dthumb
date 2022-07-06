# dthumb
ARMv5TE and ARMv4T disassembler  

Target architecture: ARMv5TE or ARMv4T  
Target processor: ARM946E-S (Nintendo DS main CPU) or ARM7TDMI (Nintendo DS secondary CPU, Nintendo GBA main CPU)

By default, the disassembly mode is THUMB for ARMv5TE, but you can use:  
- ``/t`` or ``/5`` or ``/t5``: THUMB for ARMv5TE (redundant as it is already the default mode)
- ``/4`` or ``/t4``: THUMB for ARMv4T
- ``/a`` or ``/a5``: ARM for ARMv5TE
- ``/a4``: ARM for ARMv4T  

## Disassemble from a file  
Each file **needs** to have a valid extension (at least one "dot" character in the name).
If no output file is provided, it will print to ``stdout``.  
You can optionally provide a range of addresses to be disassembled (in hexadecimal).  
```
dthumb <filein> {<fileout>} {<start>-<end>} {/<mode>}
```  

## Disassemble a single code  
The code needs to be written in hexadecimal format.  
```
dthumb <code> {/<mode>}
```
