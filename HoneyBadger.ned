simple HoneyBadger
{
    parameters:
	int id;
	int numNodes;
        double procTime @unit(s) = default(0.1s);
    gates:
	inout p2p;
}

