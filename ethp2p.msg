message GetBlock
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
};

packet NewBlock
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
};

message NewBlockHash
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;
};
