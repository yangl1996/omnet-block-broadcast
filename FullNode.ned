simple FullNode
{
    parameters:
        double miningRate = default(0.1);
        double procTime @unit(s) = default(0.1s);
        double roundIntv @unit(s) = default(10s);
    gates:
        inout link[];
}

