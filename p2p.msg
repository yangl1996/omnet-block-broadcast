struct Block
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
};

// Used to pass new blocks received from the P2P layer to the miner, or
// validated blocks from the miner to the P2P layer.
message NewBlock
{
	Block block;
};

// Request block chunk from a peer.
message GetBlockChunk
{
	Block block;
};

// Response to a request for block chunk.
packet BlockChunk
{
	Block block;
	unsigned short chunkId;
};

// Announce the availability of a block.
message NewBlockHash
{
	Block block;
};
