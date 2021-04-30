simple NodeP2P
{
    parameters:
	int totalChunks = default(1);
    gates:
        inout peer[];
	inout node;
}

