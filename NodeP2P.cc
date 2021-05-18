#include <string.h>
#include <omnetpp.h>
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

enum BlockState {
	processed,
	received,
	learned
};

struct BlockMeta {
	BlockState state;
	unsigned int downloaded;
	unsigned int requested;

	BlockMeta() : state(learned), downloaded(0), requested(0) {}
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
					blocks[b].requested < totChunks &&
					blocks[b].downloaded < totChunks) {
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlockChunk *req = new GetBlockChunk();
				req->setBlock(newBlockHash->getBlock());
				send(req, gate);
				blocks[b].requested += 1;
			}
			delete newBlockHash;	// this is a disposable message
			return;
		}

		GetBlockChunk *getBlock = dynamic_cast<GetBlockChunk*>(msg);
		if (getBlock != nullptr) {
			cGate *gate = getBlock->getArrivalGate()->getOtherHalf();
			BlockChunk *resp = new BlockChunk();
			resp->setBlock(getBlock->getBlock());
			resp->setByteLength(2000000 / totChunks);
			send(resp, gate);
			delete getBlock;	// this is a disposable message
			return;
		}

		BlockChunk *blockChunk= dynamic_cast<BlockChunk*>(msg);
		if (blockChunk != nullptr) {
			Block b = blockChunk->getBlock();
			if (blocks[b].downloaded < totChunks) {
				blocks[b].downloaded += 1;
				if (blocks[b].downloaded >= totChunks) {
					if (blocks[b].state == learned) {
						blocks[b].state = received;
					}
					NewBlock *notification = new NewBlock();
					notification->setBlock(blockChunk->getBlock());
					send(notification, toNode);
				}
				else if (blocks[b].requested < totChunks) {
					cGate *gate = blockChunk->getArrivalGate()->getOtherHalf();
					// get the other half because it's an inout gate
					GetBlockChunk *req = new GetBlockChunk();
					req->setBlock(blockChunk->getBlock());
					send(req, gate);
					blocks[b].requested += 1;
				}
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
