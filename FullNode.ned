simple FullNode
{
    parameters:
        double mineIntv @unit(s) = default(40s);
    gates:
        inout link[];
}

