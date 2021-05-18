#include <string.h>
#include <omnetpp.h>
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

enum BlockState {
	processed,
	downloaded,
	learned
};

const int ANNOUNCED = -1;
const int ACCEPTED = -2;

// NodeP2P implements the P2P event loop of a blockchain node. It loosely follows the
// Ethereum DevP2P protocol for block broadcasting.
class NodeP2P : public cSimpleModule
{
	private:
		// parameters
		cGate* fromNode;
		cGate* toNode;

		// internal states
		unordered_map<Block, BlockState> blocks; // number of chunks received of a block
		unordered_map<Block, int> downloaded; // number of chunks downloaded
		unordered_map<Block, int> requested; // number of chunks requested for a block
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
	blocks = unordered_map<Block, BlockState>();
	downloaded = unordered_map<Block, int>();
	requested = unordered_map<Block, int>();
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
	long id = block->getBlock().id();
	// only announce it if it is not announced before
	if (downloaded.find(b) == downloaded.end() || downloaded[b] != ANNOUNCED) {
		downloaded[b] = ANNOUNCED;
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
			long id = newBlockHash->getBlock().id();
			if (downloaded.find(b) == downloaded.end()) {
				downloaded[b] = 0;	// mark that we have heard the block
			}
			if (requested.find(b) == requested.end()) {
				requested[b] = 0;
			}
			// request the block if not requested before
			if (downloaded[b] >= 0 && requested[b] < totChunks && downloaded[b] < totChunks) {
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get the other half because it's an inout gate
				GetBlockChunk *req = new GetBlockChunk();
				req->setBlock(newBlockHash->getBlock());
				send(req, gate);
				requested[b] += 1;
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
			long id = blockChunk->getBlock().id();
			Block b = blockChunk->getBlock();
			if (downloaded.find(b) == downloaded.end()) {
				downloaded[b] = 0;
			}
			if (downloaded[b] >= 0) {
				downloaded[b] += 1;
				if (downloaded[b] >= totChunks) {
					downloaded[b] = ACCEPTED;
					NewBlock *notification = new NewBlock();
					notification->setBlock(blockChunk->getBlock());
					send(notification, toNode);
				}
				else {
					if (requested.find(b) == requested.end()) {
						requested[b] = 0;
					}
					if (requested[b] < totChunks) {
						cGate *gate = blockChunk->getArrivalGate()->getOtherHalf();
						// get the other half because it's an inout gate
						GetBlockChunk *req = new GetBlockChunk();
						req->setBlock(blockChunk->getBlock());
						send(req, gate);
						requested[b] += 1;
					}
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
