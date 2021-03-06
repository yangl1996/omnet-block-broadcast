#include <string.h>
#include <omnetpp.h>
#include "ethp2p_m.h"

using namespace omnetpp;
using namespace std;

const int ANNOUNCED = -1;
const int ACCEPTED = -2;
const int TOTCHUNKS = 100;

// Packs the miner ID and block sequence number into a long int
long packBlockId(unsigned short miner, unsigned int seq) {
	unsigned long m = (unsigned long)miner;
	unsigned long s = (unsigned long)seq;
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
		void processedNewBlock(NewBlock *block);

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
}

NodeP2P::~NodeP2P()
{
	// nothing to clean up
}

void NodeP2P::initialize()
{
	fromNode = gate("node$i");
	toNode = gate("node$o");
}

// Handle a block that is just processed by the local node. It announces the block to the peers
// if it has not done so.
void NodeP2P::processedNewBlock(NewBlock *block) {
	long id = packBlockId(block->getMiner(), block->getSeq());
	// only announce it if it is not announced before
	if (blocks.find(id) == blocks.end() || blocks[id] != ANNOUNCED) {
		blocks[id] = ANNOUNCED;
		int n = gateSize("peer");
		// broadcast the message
		for (int i = 0; i < n; i++) {
			NewBlockHash* m = new NewBlockHash();
			m->setHeight(block->getHeight());
			m->setMiner(block->getMiner());
			m->setSeq(block->getSeq());
			m->setTimeMined(block->getTimeMined());
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
			processedNewBlock(newBlock);
			delete newBlock;
		}
		return;
	}
	else {
		// check message type
		NewBlockHash *newBlockHash = dynamic_cast<NewBlockHash*>(msg);
		if (newBlockHash != nullptr) {
			long id = packBlockId(newBlockHash->getMiner(), newBlockHash->getSeq());
			if (blocks.find(id) == blocks.end()) {
				blocks[id] = 0;	// mark that we have heard the block
			}
			// request the block if not requested before
			if (blocks[id] >= 0 && blocks[id] < TOTCHUNKS) {
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlock *req = new GetBlock();
				req->setHeight(newBlockHash->getHeight());
				req->setMiner(newBlockHash->getMiner());
				req->setSeq(newBlockHash->getSeq());
				req->setTimeMined(newBlockHash->getTimeMined());
				send(req, gate);
			}
			delete newBlockHash;	// this is a disposable message
			return;
		}

		GetBlock *getBlock = dynamic_cast<GetBlock*>(msg);
		if (getBlock != nullptr) {
			long id = packBlockId(getBlock->getMiner(), getBlock->getSeq());
			cGate *gate = getBlock->getArrivalGate()->getOtherHalf();
			NewBlock *resp = new NewBlock();
			resp->setHeight(getBlock->getHeight());
			resp->setMiner(getBlock->getMiner());
			resp->setSeq(getBlock->getSeq());
			resp->setTimeMined(getBlock->getTimeMined());
			resp->setByteLength(2000000 / TOTCHUNKS);
			send(resp, gate);
			delete getBlock;	// this is a disposable message
			return;
		}

		NewBlock *newBlock = dynamic_cast<NewBlock*>(msg);
		if (newBlock != nullptr) {
			long id = packBlockId(newBlock->getMiner(), newBlock->getSeq());
			if (blocks.find(id) == blocks.end()) {
				blocks[id] = 0;
			}
			if (blocks[id] >= 0) {
				blocks[id] += 1;
				if (blocks[id] >= TOTCHUNKS) {
					blocks[id] = ACCEPTED;
					send(newBlock, toNode);
				}
				else {
					cGate *gate = newBlock->getArrivalGate()->getOtherHalf();
					// get the other half because it's an inout gate
					GetBlock *req = new GetBlock();
					req->setHeight(newBlock->getHeight());
					req->setMiner(newBlock->getMiner());
					req->setSeq(newBlock->getSeq());
					req->setTimeMined(newBlock->getTimeMined());
					send(req, gate);
					delete newBlock;
				}
			}
			else {
				delete newBlock;
			}
			return;
		}
	}
}

void NodeP2P::finish() {
	// nothing to do here
}
