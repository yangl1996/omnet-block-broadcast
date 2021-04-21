message GetBlock
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	double timeMined;
};
message NewBlock
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	double timeMined;
};
message NewBlockHash
{
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	double timeMined;
};
