struct Block
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
}

message GetBlock
{
	Block block;
};

packet NewBlock
{
	Block block;
};

message NewBlockHash
{
	Block block;
};
