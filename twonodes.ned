simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
    gates:
        input in;
        output out;
}

network TwoNodes
{
    submodules:
        node_1: FullNode;
        node_2: FullNode;
    connections:
        node_1.out --> {  delay = 100ms; } --> node_2.in;
        node_1.in <-- {  delay = 100ms; } <-- node_2.out;
}
