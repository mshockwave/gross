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