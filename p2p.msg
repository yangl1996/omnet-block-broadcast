struct Block
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
};

message GetBlock
{
	Block block;
};

packet NewBlock
{
	Block block;
};

packet BlockChunk
{
	Block block;
	unsigned short chunkId;
};

message NewBlockHash
{
	Block block;
};
