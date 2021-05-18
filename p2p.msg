cplusplus {{
#include "Block.h"
#include <bitset>
typedef std::bitset<30> ChunkMap;
}}

struct Block {
	@existingClass;
}

class ChunkMap {
	@existingClass;
}

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
	unsigned short chunkId;
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

// Exchange the local availability of a block.
message BlockAvailability
{
	Block block;
	ChunkMap chunks;
	@descriptor(false);	// c++ compiler complains
}
