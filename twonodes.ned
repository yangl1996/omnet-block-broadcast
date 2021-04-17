network TwoNodes
{
    submodules:
        node[10]: FullNode;
    connections:
        node[0].link++ <--> {  delay = 100ms; } <--> node[1].link++;
        node[1].link++ <--> {  delay = 100ms; } <--> node[2].link++;
        node[2].link++ <--> {  delay = 100ms; } <--> node[3].link++;
        node[3].link++ <--> {  delay = 100ms; } <--> node[4].link++;
        node[4].link++ <--> {  delay = 100ms; } <--> node[5].link++;
        node[5].link++ <--> {  delay = 100ms; } <--> node[6].link++;
        node[6].link++ <--> {  delay = 100ms; } <--> node[7].link++;
        node[7].link++ <--> {  delay = 100ms; } <--> node[8].link++;
        node[8].link++ <--> {  delay = 100ms; } <--> node[9].link++;
}
