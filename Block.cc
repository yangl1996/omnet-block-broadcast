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
