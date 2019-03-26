This folder contains most of the 'middle-end' optimizations.
 - **ValuePromotion** is basically `mem2reg` in LLVM.
 - **Peephole** performs many trivials graph reductions like constant merging.
 - **MemoryLegalize** and **DLXMemoryLegalize** legalize memory nodes into forms that are acceptable in later pipeline.
 - **CSE** perform common subexpression elimination. Note that since we associate memory nodes in a 'memory SSA' fashion, doing CSE on them is pretty easy.