#include <string.h>
#include <omnetpp.h>
#include "NewBlock_m.h"
#include "GetBlock_m.h"
#include "NewBlockHash_m.h"

using namespace omnetpp;
using namespace std;

// Packs the miner ID and block sequence number into a long int
long packBlockId(unsigned short miner, unsigned int seq) {
	unsigned long m = (unsigned long)miner;
	unsigned long s = (unsigned long)seq;
	return (long)(m << sizeof(unsigned int)) + s;
}

// FullNode is a full node in a blockchain network.
class FullNode : public cSimpleModule
{
	private:
		unsigned short id;          // id of the node
		unsigned int nextBlockSeq;    // sequence of the block; combined with id identifies a block
		cMessage *nextMine;  // event when a block is mined
		cMessage *nextProcBlock;  // event when a block is processed
		cQueue blockProcQueue;   // blocks to be processed
		unsigned int bestLevel;       // the highest block level
		unordered_set<long> heardBlocks; // set of blocks that are heard of
		unordered_set<long> rcvdBlocks;  // set of blocks that have been received

		void scheduleNextMine();
		void procBlock(NewBlock *block);
		NewBlock* mineBlock();

	public:
		FullNode();
		virtual ~FullNode();

	protected:
		virtual void initialize() override;
		virtual void handleMessage(cMessage *msg) override;
};

// Register the module with omnet
Define_Module(FullNode);

FullNode::FullNode()
{
	nextMine = new cMessage("mined");
	nextProcBlock = new cMessage("proced");
	bestLevel = 0;
	nextBlockSeq = 0;
	blockProcQueue = cQueue("blockProcQueue");
	heardBlocks = unordered_set<long>();
	rcvdBlocks = unordered_set<long>();
}

FullNode::~FullNode()
{
	cancelAndDelete(nextMine);
	cancelAndDelete(nextProcBlock);
}

void FullNode::initialize()
{
	id = par("id").intValue(); // cannot be placed in the constructor because the parameter was not ready
	WATCH(bestLevel);
	WATCH(nextBlockSeq);

	scheduleNextMine();
}

// Randomly samples an exponential delay and schedules the nextMine
// event to happen after that time.
void FullNode::scheduleNextMine() {
	simtime_t timeToNextBlock = exponential(par("mineIntv").doubleValueInUnit("s"));
	scheduleAt(simTime()+timeToNextBlock, nextMine);
}

// Creates a NewBlock message with appropriate height, miner ID, and sequence number, and
// increases the block sequence number.
NewBlock* FullNode::mineBlock() {
	NewBlock *newBlock = new NewBlock("block");
	newBlock->setHeight(bestLevel+1);
	newBlock->setMiner(id);
	newBlock->setSeq(nextBlockSeq);
	nextBlockSeq += 1;
	return newBlock;
}

// Processes a new block. Update the best height, and records the reception of the block.
// Then announces the block to peers.
void FullNode::procBlock(NewBlock *block) {
	long id = packBlockId(block->getMiner(), block->getSeq());
	// only process it if it has not been heard
	if (rcvdBlocks.find(id) == rcvdBlocks.end()) {
		rcvdBlocks.insert(id);
		heardBlocks.insert(id);
		if (block->getHeight() > bestLevel) {
			bestLevel = block->getHeight();
		}
		int n = gateSize("link");
		// broadcast the message
		for (int i = 0; i < n; i++) {
			NewBlockHash* m = new NewBlockHash();
			m->setHeight(block->getHeight());
			m->setMiner(block->getMiner());
			m->setSeq(block->getSeq());
			send(m, "link$o", i);
		}
	}

}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		// block mining event
		NewBlock *newBlock = mineBlock();
		procBlock(newBlock);	// process it locally, does not take time
		delete newBlock;
		scheduleNextMine();
	} else if (msg == nextProcBlock) {
		// block processing event
		// take the next block from the queue
		NewBlock *newBlock = check_and_cast<NewBlock*>(blockProcQueue.pop());
		procBlock(newBlock);
		delete newBlock;
		// if the queue is not empty, schedule the next processing
		if (!blockProcQueue.isEmpty()) {
			scheduleAt(simTime()+0.1, nextProcBlock);
		}
	} else {
		// check message type
		NewBlock *newBlock = dynamic_cast<NewBlock*>(msg);
		if (newBlock != nullptr) {
			// schedule processing if no block is waiting for processing
			if (blockProcQueue.isEmpty()) {
				scheduleAt(simTime()+0.1, nextProcBlock);
			}
			// put it into processing queue
			blockProcQueue.insert(msg);
		}

		NewBlockHash *newBlockHash = dynamic_cast<NewBlockHash*>(msg);
		if (newBlockHash != nullptr) {
			long id = packBlockId(newBlockHash->getMiner(), newBlockHash->getSeq());
			// request the block if not requested before
			if (heardBlocks.find(id) == heardBlocks.end()) {
				heardBlocks.insert(id);
				cGate *gate = newBlockHash->getArrivalGate()->getOtherHalf();
				// get other half because it's an inout gate
				GetBlock *req = new GetBlock();
				req->setHeight(newBlockHash->getHeight());
				req->setMiner(newBlockHash->getMiner());
				req->setSeq(newBlockHash->getSeq());
				send(req, gate);
			}
			delete newBlockHash;	// this is a disposable message
		}

		GetBlock *getBlock = dynamic_cast<GetBlock*>(msg);
		if (getBlock != nullptr) {
			cGate *gate = getBlock->getArrivalGate()->getOtherHalf();
			NewBlock *resp = new NewBlock();
			resp->setHeight(getBlock->getHeight());
			resp->setMiner(getBlock->getMiner());
			resp->setSeq(getBlock->getSeq());
			send(resp, gate);
			//delete getBlock;	// this is a disposable message
		}
	}
}
