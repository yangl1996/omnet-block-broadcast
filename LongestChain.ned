simple Miner
{
    parameters:
	int id;
        double miningRate = default(0.1);
        double procTime @unit(s) = default(0.1s);
        double roundIntv @unit(s) = default(10s);
	int numFixedMiners = default(0);
    gates:
	inout p2p;
}

