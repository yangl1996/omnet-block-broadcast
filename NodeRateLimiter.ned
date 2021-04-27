simple NodeRateLimiter
{
    parameters:
        double incomingRate @unit(bps) = default(0bps);
        double outgoingRate @unit(bps) = default(0bps);
    gates:
        inout inner[];
	inout outer[];
}

