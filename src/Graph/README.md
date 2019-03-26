## Graph and SubGraph
`Graph` stores all the enclosing `Node`s, it is also the owner of them. `SubGraph` represents subset of nodes within a function, it only store the `End` node of that function, so it can be trivially construcuted and copied.

## Edge
An edge represents a dependency relationship. A 'sink' which depends on a 'source' node will forms a direct edge points from _sink_ to _source_. There are three category of edges in GROSS:
 - **Value** dependencies represent normal data flow. Sink nodes will consume data generated from source nodes.
 - **Control** dependencies represent the movement path of virtual **control tokens** for execution model in [Cliff's paper](https://dl.acm.org/citation.cfm?id=201061), Appendix 4.1.
 - **Effect** dependencies impose the ordering between two nodes. For example, a `MemLoad` node will effect depends on a `MemStore` if the former one is required to executed after the latter one.

There might be some confusions between control and effect dependencies. An informal and _theoritic-computer-scientist-unfriendly_ explain will be: Control dependencies connect control nodes, which will tell you **how to move the tape** in a Turing Machine.

Effect dependencies connect nodes that need to execute in certain order because they will make (permanant) changes on their environment(in most cases, the **memory**), just like punching holes on the tape of a Turing Machine. On the other hand, value edges only tells you **how values floating around the air**: how values tossed through the air from one operation to the other. Since they always stay in the air, they won't even _touch_ their environment and thus pose less restrictions.

## Node
A node can mean lots of thing: an operation, an extrenal data(e.g. `Argument`), a control step(i.e. all the control nodes), or a virtual placeholder for special purposes(e.g. `EffectMerge`). Nodes can have all kinds of dependencies, namely, the _input_.