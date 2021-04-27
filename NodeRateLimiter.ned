//
// NodeRateLimiter enforces the rate data can go into and out of a module. It has
// two arraies of ports: inner, and outer. Each inner port at index i, inner[i],
// is connected to the corresponding outer port at the same index, outer[i].
//
simple NodeRateLimiter
{
    parameters:
        double incomingRate @unit(bps) = default(0bps);	// Max data rate from outer to inner ports.
        double outgoingRate @unit(bps) = default(0bps); // Max data rate from inner to outer ports.
    gates:
        inout inner[];
	inout outer[];
}
