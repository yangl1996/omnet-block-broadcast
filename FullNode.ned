simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
        double procTime @unit(s) = default(0.1s);
    gates:
        inout link[];
}

