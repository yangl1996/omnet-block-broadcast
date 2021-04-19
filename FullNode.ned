simple FullNode
{
    parameters:
        double roundTime @unit(s) = default(1s);
        double miningRate = default(0.1);
        double procTime @unit(s) = default(0.1s);
    gates:
        inout link[];
}

