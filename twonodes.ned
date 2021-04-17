simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
        int id;
    gates:
        input in;
        output out;
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
        node_1.out --> {  delay = 100ms; } --> node_2.in;
        node_1.in <-- {  delay = 100ms; } <-- node_2.out;
}
