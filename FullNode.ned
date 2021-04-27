module FullNode
{
	gates:
		inout peer[];
	submodules:
		p2p: NodeP2P;
		rl: NodeRateLimiter;
		miner: Miner {
			id = ancestorIndex(1);
		}
	connections:
		p2p.node <--> miner.p2p;
		peer[i] <--> rl.outer++ for i=0..sizeof(peer)-1;
		p2p.peer++ <--> rl.inner++ for i=0..sizeof(peer)-1;
}
