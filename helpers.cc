#include "helpers.h"

// Packs the miner ID and block sequence number into a long int
long packBlockId(const Block block) {
	unsigned long m = (unsigned long)block.miner;
	unsigned long s = (unsigned long)block.seq;
	return (long)(m << (sizeof(unsigned int)* 8)) + s;
}
