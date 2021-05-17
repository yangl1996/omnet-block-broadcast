#include "Block.h"

bool Block::operator==(const Block& other) const {
	if (this->height == other.height &&
			this->miner == other.miner &&
			this->seq == other.seq &&
			this->timeMined == other.timeMined) {
		return true;
	}
	else {
		return false;
	}
}

long Block::id() const {
	unsigned long m = (unsigned long)this->miner;
	unsigned long s = (unsigned long)this->seq;
	return (long)(m << (sizeof(unsigned int)* 8)) + s;
}
