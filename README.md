# GROSS - GRaph Optimizer Solely for cS241

## Language / Target Architecture Specs
 - [Programming Language(PL241)](doc/lang-spec.pdf)
 - [Target Architecture](doc/DLX.pdf)

## Design
As you may know, Graph IR is the core of this compiler. Inspired by TurboFan JIT compiler in [V8 javascript engine](https://v8.dev),
this project use IR format that has lots of freedom among vertices. Namely, _"Sea of Nodes"_ IR graph.
 - [C Click, KD Cooper."Combining analyses, combining optimizations"](https://dl.acm.org/citation.cfm?id=201061)
   - The original "Sea of Nodes" paper. (Jump to Chap6,7 for TL;DR)
 - [TurboFan TechTalk presentation](https://docs.google.com/presentation/d/1sOEF4MlF7LeO7uq-uThJSulJlTh--wgLeaVibsbb3tc/edit#slide=id.g54ccc405e_2102)
 - [TurboFan IR](https://docs.google.com/presentation/d/1Z9iIHojKDrXvZ27gRX51UxHD-bKf1QcPzSijntpMJBM/edit#slide=id.g19134d40cb_0_193)
