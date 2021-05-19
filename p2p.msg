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
// validated blocks from the miner to the P2P layer. Not for use between
// peers.
message NewBlock
{
	Block block;
};

// Exchange the local availability of a block.
message BlockAvailability
{
	Block block;
	ChunkMap chunks;
	@descriptor(false);	// c++ compiler complains otherwise
}

// Request block chunks from a peer.
message GetBlockChunks
{
	Block block;
	ChunkMap chunks;
	@descriptor(false);	// c++ compiler complains otherwise
};

// Response to a request for block chunk.
packet BlockChunk
{
	Block block;
	unsigned short chunkId;
};

