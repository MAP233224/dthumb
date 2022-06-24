# dthumb
ARMv5TE disassembler  

Target architecture: ARMv5TE  
Target processor: ARM946E-S (Nintendo DS main CPU)  

Each file **needs** to have a valid extension (at least one "dot" character in the name).
If no output file is provided, it will print to ``stdout``.  
You can optionally provide a range of addresses to be disassembled (in hexadecimal).
```
dthumb <filein> {<fileout>} {<start>-<end>}
```
