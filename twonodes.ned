network TwoNodes
{
    submodules:
        node[2]: FullNode;
    connections:
        node[0].link++ <--> {  delay = 100ms; } <--> node[1].link++;
}
