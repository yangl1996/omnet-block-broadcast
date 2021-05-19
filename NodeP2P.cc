#include <string.h>
#include <omnetpp.h>
#include "NodeRateLimiter.h"
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

const int NUM_CHUNKS = 30;

// BlockState is the processing state of a block.
enum BlockState {
	processed,	// processed locally and announced to peeers
	received,	// downloaded locally, but not processed or announced
	learned		// learned the existence of the block, but not downloaded
};

// BlockMeta gathers the p2p layer information of a block.
struct BlockMeta {
       BlockState state;		// local processing state
       ChunkMap downloaded;		// chunks that we have finished downloading
       unordered_map<unsigned short, ChunkMap> peerAvail;	// peers' possession of chunks

       BlockMeta(): state(learned),
	downloaded(ChunkMap()),
	peerAvail(unordered_map<unsigned short, ChunkMap>()) {}
	
};

// NodeP2P implements the P2P layer of a blockchain node.
class NodeP2P : public cSimpleModule
{
	private:
		// special gates to and from the node (consensus logic)
		cGate* fromNode;
		cGate* toNode;
		
		// network layer
		NodeRateLimiter* rateLimiter;	// per-node rate limiter (to query queue length)

		// protocol states
		unordered_map<Block, BlockMeta> blocks; // per-block protocol state

		// helper methods
		void maybeAnnounceNewBlock(NewBlock *block);
		void notifyPeersOfAvailability(Block block);

	public:
		NodeP2P();
		virtual ~NodeP2P();

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
};

Define_Module(NodeP2P);	// register the module with OMNET++

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
	rateLimiter = dynamic_cast<NodeRateLimiter*>(getParentModule()->getSubmodule("rl"));
}

// Handle a block that is just processed by the local node. It announces the block to the peers
// if it has not done so.
void NodeP2P::maybeAnnounceNewBlock(NewBlock *block) {
	Block b = block->getBlock();
	// only announce it if it is not announced before
	if (blocks[b].state != processed) {
		blocks[b].state = processed;
		blocks[b].downloaded.set();
		notifyPeersOfAvailability(b);
	}
}

void NodeP2P::notifyPeersOfAvailability(Block block) {
	int n = gateSize("peer");
	// broadcast the message
	for (int i = 0; i < n; i++) {
		BlockAvailability* m = new BlockAvailability();
		m->setBlock(block);
		m->setChunks(blocks[block].downloaded);
		send(m, "peer$o", i);
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
		BlockAvailability *blockAvail = dynamic_cast<BlockAvailability*>(msg);
		if (blockAvail != nullptr) {
			Block b = blockAvail->getBlock();
			unsigned short peerIdx = blockAvail->getArrivalGate()->getIndex();
			// TODO: do something here now that peer has updates its availability
			delete blockAvail;	// this is a disposable message
			return;
		}

		BlockChunk *blockChunk= dynamic_cast<BlockChunk*>(msg);
		if (blockChunk != nullptr) {
			Block b = blockChunk->getBlock();
			blocks[b].downloaded[blockChunk->getChunkId()] = 1;
			if (blocks[b].downloaded.count() >= NUM_CHUNKS) {
				if (blocks[b].state == learned) {
					blocks[b].state = received;
					NewBlock *notification = new NewBlock();
					notification->setBlock(blockChunk->getBlock());
					send(notification, toNode);
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
}
