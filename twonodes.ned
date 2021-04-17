simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
    gates:
        inout link[];
}

network TwoNodes
{
    submodules:
        node[3]: FullNode;
    connections:
        node[0].link++ <--> {  delay = 100ms; } <--> node[1].link++;
        node[1].link++ <--> {  delay = 100ms; } <--> node[2].link++;
}
