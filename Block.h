#include <omnetpp.h>
using namespace std;
using namespace omnetpp;

struct Block {
	unsigned int height;
	unsigned short miner;
	unsigned int seq;
	simtime_t timeMined;

	bool operator==(const Block& other) const;
	long id() const;
};

namespace std {
	template <> struct hash<Block> {
		std::size_t operator()(const Block& k) const {
			return hash<long>()(k.id());
		}
	};

}
