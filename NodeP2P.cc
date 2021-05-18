#include <string.h>
#include <bitset>
#include <omnetpp.h>
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

const int NUM_CHUNKS = 30;

enum BlockState {
	processed,
	received,
	learned
};

struct BlockMeta {
	BlockState state;
	bitset<NUM_CHUNKS> downloaded;
	bitset<NUM_CHUNKS> requested;

	BlockMeta() : state(learned), downloaded(bitset<NUM_CHUNKS>()), requested(bitset<NUM_CHUNKS>()) {}
};

// NodeP2P implements the P2P event loop of a blockchain node. It loosely follows the
// Ethereum DevP2P protocol for block broadcasting.
class NodeP2P : public cSimpleModule
{
	private:
		// parameters
		cGate* fromNode;
		cGate* toNode;

		// internal states
		unordered_map<Block, BlockMeta> blocks; // block and its metadata
		int totChunks;	// how many chunks to split the block into

		// helper methods
		void maybeAnnounceNewBlock(NewBlock *block);
		unsigned short nextChunkToRequest(Block block) const;

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
	blocks = unordered_map<Block, BlockMeta>();
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

unsigned short NodeP2P::nextChunkToRequest(Block block) const {
	BlockMeta m = blocks.at(block);
	unsigned short idx;
	for (idx = 0; idx < NUM_CHUNKS; idx++) {
		if (!m.requested[idx]) {
			break;
		}
	}
	return idx;
}

// Handle a block that is just processed by the local node. It announces the block to the peers
// if it has not done so.
void NodeP2P::maybeAnnounceNewBlock(NewBlock *block) {
	Block b = block->getBlock();
	// only announce it if it is not announced before
	if (blocks[b].state != processed) {
		blocks[b].state = processed;
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
			Block b = newBlockHash->getBlock();
			// request the block if not requested before
			if (blocks[b].state == learned &&
					blocks[b].requested.count() < totChunks &&
					blocks[b].downloaded.count() < totChunks) {
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlockChunk *req = new GetBlockChunk();
				req->setBlock(newBlockHash->getBlock());
				unsigned short idx = nextChunkToRequest(newBlockHash->getBlock());
				req->setChunkId(idx);
				send(req, gate);
				blocks[b].requested[idx] = 1;
			}
			delete newBlockHash;	// this is a disposable message
			return;
		}

		GetBlockChunk *getBlock = dynamic_cast<GetBlockChunk*>(msg);
		if (getBlock != nullptr) {
			cGate *gate = getBlock->getArrivalGate()->getOtherHalf();
			BlockChunk *resp = new BlockChunk();
			resp->setBlock(getBlock->getBlock());
			resp->setChunkId(getBlock->getChunkId());
			resp->setByteLength(2000000 / totChunks);
			send(resp, gate);
			delete getBlock;	// this is a disposable message
			return;
		}

		BlockChunk *blockChunk= dynamic_cast<BlockChunk*>(msg);
		if (blockChunk != nullptr) {
			Block b = blockChunk->getBlock();
			blocks[b].downloaded[blockChunk->getChunkId()] = 1;
			if (blocks[b].downloaded.count() >= totChunks) {
				if (blocks[b].state == learned) {
					blocks[b].state = received;
					NewBlock *notification = new NewBlock();
					notification->setBlock(blockChunk->getBlock());
					send(notification, toNode);
				}
			}
			else if (blocks[b].requested.count() < totChunks) {
				cGate *gate = blockChunk->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlockChunk *req = new GetBlockChunk();
				unsigned int idx = nextChunkToRequest(b);
				req->setBlock(blockChunk->getBlock());
				req->setChunkId(idx);
				send(req, gate);
				blocks[b].requested[idx] = 1;
			}
			delete blockChunk;
			return;
		}

		// otherwise just deliver it to the node
		send(msg, toNode);
	}
}

void NodeP2P::finish() {
	// nothing to do here
}
