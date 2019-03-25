# CodeGen Pipeline
The biggest different with conventional codegen pipeline is that we don't
have standalone instruction selection phase. Instead, we separate it into two
individual phases: Pre and PostMachineLowering. Primary reason to do so is because our language is simple, such that we can select native instructions for most of the operations before linearlizing the graph.

1. **PreMachineLowering** phase lowers operations unrelated to control flow into native instructions.
2. **GraphScheduling** phase 'linearlizes' the graph into straight-line code. That is, conventional BasicBlocks and CFG. This phase have several sub-phases:
   1. **CFGBuilder** assigns a BB for each control nodes(e.g. Loop, IfTrue/False, Merge). And place some fixed nodes(e.g. PHI) in correct places. Finally, it connects BBs into CFG.
   2. **PostOrderNodePlacement** places nodes from the bottom-up. By adopting bottom-up strategy, we can have better register live ranges. The drawbacks is that we can't doing some optimizations that requires input nodes scheduled (e.g. loop invariant node placement) at the same time.
   3. **RPONodePlacement** _(TBD)_ solves the problems in previous phase. Since all the inputs of a given node have been scheduled.
3. **PostMachineLowering** phase lowers rest of the control-flow-sensitive nodes. For example: jumps, function calls and function prologue/epilogues. Also, this phase removes all the PHIs that only have effect inputs/output(i.e. EffectPhi)
4. **RegisterAllocator** phase assign physical registers to instructions. Currently we adopt linear scan register allocation.
5. **PostRALowering** continue lowering some nodes that is previously required by RA. Also do some house cleaning.

# ABI
The origin DLX architecture doesn't give a concrete ABI definition. It only specified the following special registers:
 - **R0** is always zero value.
 - **R28** is used as frame pointer.
 - **R29** is used as stack pointer.
 - **R30** is used to access global variables.
 - **R31** is the link register.

I add another two special registers: **R26** and **R27** as 'scratch' registers. They're usually used in register-spilling code (By reserving independent registers, register allocator will be easier to implement). They can not be used as general-purpose value registers.

Also, I define my own calling convention in the following section.

## Calling Convention
I use a classic RISC-style calling convention design. Where paramters and 
result are passed by registers. Here is the detail procedure calling flow:
1. Save any caller-saved registers. In this case, they're `R28`(frame pointer), `R31`(link register), `R2`,`R3`,`R4`, `R5`(parameter registers), and all the other general-purpose registers except `R6`, `R7`, `R8`, and `R9`.
2. Parameters are stored in `R2`,`R3`,`R4`, and `R5`. If there are more parameters, push the rest to the stack.
3. Execute `BSR`, which will save return address to `R31` and jump to target procedure.
4. Save the current stack position to frame pointer(`R28`).
5. Reserve stack slots for local variables and/or register spilling slots.
6. Procedure need to save value of `R6`, `R7`, `R8`, and `R9` (i.e. callee-saved registers).
7. Store return value in `R1`.
8. Before exiting:
   1. Restore all the callee-saved registers.
   2. Restore frame pointer(`R28`) value back to stack pointer(`R29`).
9.  Execute `RET R31` to go back to caller procedure.
10. Restore caller-saved registers.

Here is the summary of all register usages:

|  Register |                            Description                           |
|:---------:|:----------------------------------------------------------------:|
|     R0    | Zero.                                                            |
|     R1    | Return value.                                                    |
|  R2 ~ R5  | First four parameters.                                           |
|  R6 ~ R9  | Callee-saved general-purpose registers.                          |
| R10 ~ R25 | Caller-saved general-purpose registers.                          |
|  R26, R27 | Scratch registers, can not be used as general-purpose registers. |
|    R28    | Frame pointer.                                                   |
|    R29    | Stack pointer.                                                   |
|    R30    | Global variables pointer.                                        |
|    R31    | Link register.                                                   |

Here is the memory layout (address range from high to low):

|             Stack Slots                |
|----------------------------------------|
|  Parameter N (N > 4)                   | 
|  Parameter N + 1 (N > 4)               | 
|------------ FP Points Here ------------| 
|  Local Var1                            | 
|  Local Var2                            | 
|  Register Spill Slot1                  | 
|  Register Spill Slot2                  | 
|  Callee-Saved Register1                | 
|  Callee-Saved Register2                | 
|  ....General Purpose Stack Space...    |
|------------ SP Points Here ------------| 
|  Caller-Saved Registers1               | 
|  Caller-Saved Registers2               | 
|  Parameter M (M > 4)                   | 
|  Parameter M + 1 (M > 4)               | 
|----- Start of another stack frame -----| 
