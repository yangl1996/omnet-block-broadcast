#include <string.h>
#include <omnetpp.h>
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

const int ANNOUNCED = -1;
const int ACCEPTED = -2;

// Packs the miner ID and block sequence number into a long int
long packBlockId(const Block block) {
	unsigned long m = (unsigned long)block.miner;
	unsigned long s = (unsigned long)block.seq;
	return (long)(m << (sizeof(unsigned int)* 8)) + s;
}

// NodeP2P implements the P2P event loop of a blockchain node. It loosely follows the
// Ethereum DevP2P protocol for block broadcasting.
class NodeP2P : public cSimpleModule
{
	private:
		// parameters
		cGate* fromNode;
		cGate* toNode;

		// internal states
		unordered_map<long, int> blocks; // number of chunks received of a block
		unordered_map<long, int> requested; // number of chunks requested for a block
		void maybeAnnounceNewBlock(NewBlock *block);
		int totChunks;	// how many chunks to split the block into

	public:
		NodeP2P();
		virtual ~NodeP2P();

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
};

// Register the module with omnet
Define_Module(NodeP2P);

NodeP2P::NodeP2P()
{
	blocks = unordered_map<long, int>();
	requested = unordered_map<long, int>();
}

NodeP2P::~NodeP2P()
{
	// nothing to clean up
}

void NodeP2P::initialize()
{
	fromNode = gate("node$i");
	toNode = gate("node$o");
	totChunks = par("totalChunks").intValue();
}

// Handle a block that is just processed by the local node. It announces the block to the peers
// if it has not done so.
void NodeP2P::maybeAnnounceNewBlock(NewBlock *block) {
	long id = packBlockId(block->getBlock());
	// only announce it if it is not announced before
	if (blocks.find(id) == blocks.end() || blocks[id] != ANNOUNCED) {
		blocks[id] = ANNOUNCED;
		int n = gateSize("peer");
		// broadcast the message
		for (int i = 0; i < n; i++) {
			NewBlockHash* m = new NewBlockHash();
			m->setBlock(block->getBlock());
			send(m, "peer$o", i);
		}
	}
}

void NodeP2P::handleMessage(cMessage *msg)
{
	// check if it is from the node
	if (msg->getArrivalGate() == fromNode) {
		// check message type
		NewBlock *newBlock = dynamic_cast<NewBlock*>(msg);
		if (newBlock != nullptr) {
			maybeAnnounceNewBlock(newBlock);
			delete newBlock;
		}
		else {
			int n = gateSize("peer");
			// broadcast the message
			for (int i = 0; i < n; i++) {
				send(msg->dup(), "peer$o", i);
			}
			delete msg;
		}
		return;
	}
	else {
		// check message type
		NewBlockHash *newBlockHash = dynamic_cast<NewBlockHash*>(msg);
		if (newBlockHash != nullptr) {
			long id = packBlockId(newBlockHash->getBlock());
			if (blocks.find(id) == blocks.end()) {
				blocks[id] = 0;	// mark that we have heard the block
			}
			if (requested.find(id) == requested.end()) {
				requested[id] = 0;
			}
			// request the block if not requested before
			if (blocks[id] >= 0 && requested[id] < totChunks && blocks[id] < totChunks) {
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlock *req = new GetBlock();
				req->setBlock(newBlockHash->getBlock());
				send(req, gate);
				requested[id] += 1;
			}
			delete newBlockHash;	// this is a disposable message
			return;
		}

		GetBlock *getBlock = dynamic_cast<GetBlock*>(msg);
		if (getBlock != nullptr) {
			long id = packBlockId(getBlock->getBlock());
			cGate *gate = getBlock->getArrivalGate()->getOtherHalf();
			NewBlock *resp = new NewBlock();
			resp->setBlock(getBlock->getBlock());
			resp->setByteLength(2000000 / totChunks);
			send(resp, gate);
			delete getBlock;	// this is a disposable message
			return;
		}

		NewBlock *newBlock = dynamic_cast<NewBlock*>(msg);
		if (newBlock != nullptr) {
			long id = packBlockId(newBlock->getBlock());
			if (blocks.find(id) == blocks.end()) {
				blocks[id] = 0;
			}
			if (blocks[id] >= 0) {
				blocks[id] += 1;
				if (blocks[id] >= totChunks) {
					blocks[id] = ACCEPTED;
					send(newBlock, toNode);
				}
				else {
					if (requested.find(id) == requested.end()) {
						requested[id] = 0;
					}
					if (requested[id] < totChunks) {
						cGate *gate = newBlock->getArrivalGate()->getOtherHalf();
						// get the other half because it's an inout gate
						GetBlock *req = new GetBlock();
						req->setBlock(newBlock->getBlock());
						send(req, gate);
						requested[id] += 1;
					}
					delete newBlock;
				}
			}
			else {
				delete newBlock;
			}
			return;
		}

		// otherwise just deliver it to the node
		send(msg, toNode);
	}
}

void NodeP2P::finish() {
	// nothing to do here
}
