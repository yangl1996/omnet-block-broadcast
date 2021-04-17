simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
        int id;
    gates:
        inout link[];
}

network TwoNodes
{
    submodules:
        node_1: FullNode {
		parameters:
			id = 1;
	}
        node_2: FullNode {
		parameters:
			id = 2;
	}
    connections:
        node_1.link++ <--> {  delay = 100ms; } <--> node_2.link++;
}
