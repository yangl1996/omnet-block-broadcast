#include <string.h>
#include <omnetpp.h>
#include "NewBlock_m.h"

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
		cMessage *nextProc;  // event when a block is processed
		cQueue procQueue;   // blocks to be processed
		unsigned int bestLevel;       // the highest block level
		void scheduleNextMine();
		void procBlock(NewBlock *block);
		void announceBlock(NewBlock *block);
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
	nextProc = new cMessage("proced");
	bestLevel = 0;
	nextBlockSeq = 0;
	procQueue = cQueue("procQueue");
}

FullNode::~FullNode()
{
	cancelAndDelete(nextMine);
	cancelAndDelete(nextProc);
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

// Processes a new block. Currently it simply updates the best height.
void FullNode::procBlock(NewBlock *block) {
	// update the best height
	if (block->getHeight() > bestLevel) {
		bestLevel = block->getHeight();
	}
}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		// block mining event
		NewBlock *newBlock = mineBlock();
		procBlock(newBlock);	// process it locally, does not take time
		send(newBlock, "out");
		scheduleNextMine();
	} else if (msg == nextProc) {
		// block processing event
		// take the next block from the queue
		NewBlock *newBlock = (NewBlock*)procQueue.pop();
		procBlock(newBlock);
		delete newBlock;
		// if the queue is not empty, schedule the next processing
		if (!procQueue.isEmpty()) {
			scheduleAt(simTime()+0.1, nextProc);
		}
	} else {
		// schedule processing if no block is waiting for processing
		if (procQueue.isEmpty()) {
			scheduleAt(simTime()+0.1, nextProc);
		}
		// put it into processing queue
		NewBlock *block = (NewBlock*)(msg);
		procQueue.insert(block);
	}
}
