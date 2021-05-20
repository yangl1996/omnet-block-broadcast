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
       ChunkMap requested;		// chunks for which we have broadcast our request
       unordered_map<unsigned short, ChunkMap> peerAvail;	// peers' possession of chunks
       unordered_map<unsigned short, ChunkMap> peerReq;		// peers' request for chunks

       BlockMeta(): state(learned),
	downloaded(ChunkMap()),
	requested(ChunkMap()),
	peerAvail(unordered_map<unsigned short, ChunkMap>()),
	peerReq(unordered_map<unsigned short, ChunkMap>())
	{}
	
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
			Block b = newBlock->getBlock();
			blocks[b].state = processed;
			blocks[b].downloaded.set();
			blocks[b].requested.set();
			notifyPeersOfAvailability(b);
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
		if (dynamic_cast<BlockAvailability*>(msg)) {
			BlockAvailability *blockAvail = dynamic_cast<BlockAvailability*>(msg);
			Block b = blockAvail->getBlock();
			unsigned short peerIdx = blockAvail->getArrivalGate()->getIndex();
			blocks[b].peerAvail[peerIdx] |= blockAvail->getChunks();
			// TODO: for now we assume everyone wants the full block
			// broadcast our request for the whole block if not done so
			if (blocks[b].requested.count() == 0) {
				int n = gateSize("peer");
				// broadcast the message
				for (int i = 0; i < n; i++) {
					GetBlockChunks* m = new GetBlockChunks();
					m->setBlock(b);
					m->getChunksForUpdate().set();	// set all bits
					send(m, "peer$o", i);
				}
				blocks[b].requested.set();
			}
			delete blockAvail;	// this is a disposable message
		}
		else if (dynamic_cast<BlockChunk*>(msg)) {
			BlockChunk *blockChunk= dynamic_cast<BlockChunk*>(msg);
			Block b = blockChunk->getBlock();
			if (blocks[b].downloaded[blockChunk->getChunkId()] == false) {
				blocks[b].downloaded[blockChunk->getChunkId()] = 1;
				notifyPeersOfAvailability(b);
				if (blocks[b].downloaded.count() >= NUM_CHUNKS) {
					if (blocks[b].state == learned) {
						blocks[b].state = received;
						NewBlock *notification = new NewBlock();
						notification->setBlock(blockChunk->getBlock());
						send(notification, toNode);
					}
				}
			}
			delete blockChunk;
		}
		else if (dynamic_cast<GetBlockChunks*>(msg)) {
			GetBlockChunks *getChunks = dynamic_cast<GetBlockChunks*>(msg);
			Block b = getChunks->getBlock();
			unsigned short peerIdx = getChunks->getArrivalGate()->getIndex();
			blocks[b].peerReq[peerIdx] |= getChunks->getChunks();
			delete getChunks;
		}
		else {
			// otherwise just deliver it to the node
			send(msg, toNode);
			return;
		}
		// this branch will only be taken if the message is for p2p not for node
		// try to fill the queue
		unsigned int qLength = rateLimiter->outQueueLength();
		int npeers = gateSize("peer");
		// put more work into the queue
		// we do not need to worry about ordering between blocks, because before the
		// next epoch kicks off, all blocks have been downloaded by all nodes
		// also, we do not need to worry about requesting on others' behalf, because
		// all nodes request all blocks for now (HB)
		// we try to prioritize nodes with higher idx (larger out bw)
		for (int pidx = npeers-1; pidx >= 0; pidx--) {
			for (auto it: blocks) {
				// see if this peer has any outstanding request that we can fulfill
				ChunkMap mask = it.second.peerReq[pidx] &
					(~it.second.peerAvail[pidx]) & 
					it.second.downloaded;
				for (int cidx = 0; cidx < NUM_CHUNKS; cidx++) {
					if (mask[cidx]) {
						BlockChunk *resp = new BlockChunk();
						resp->setBlock(it.first);
						resp->setChunkId(cidx);
						resp->setByteLength(2000000 / NUM_CHUNKS);
						send(resp, "peer$o", pidx);
						qLength++;
						// important: locally mark that the peer has received the chunk
						blocks[it.first].peerAvail[pidx][cidx] = true;
						if (qLength >= 5000) {
							return;
						}
					}
				}
			}
		}
	}
}

void NodeP2P::finish() {
}
