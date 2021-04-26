module FullNode
{
	gates:
		inout peer[];
	submodules:
		p2p: NodeP2P;
		miner: Miner;
	connections:
		p2p.node <--> miner.p2p;
		peer[i] <--> p2p.peer++ for i=0..sizeof(peer)-1;
		
}
