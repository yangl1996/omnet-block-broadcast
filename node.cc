#include <string.h>
#include <omnetpp.h>
#include "NewBlock_m.h"

using namespace omnetpp;

// FullNode is a full node in a blockchain network.
class FullNode : public cSimpleModule
{
	private:
		cMessage *nextMine;  // event when a block is mined
		cMessage *nextProc;  // event when a block is processed
		cQueue *procQueue;   // blocks to be processed
		int bestLevel;       // the highest block level
		int blocksMined;
		void scheduleNextMine();
		void procBlock(NewBlock *block);

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
	bestLevel = 1;
	blocksMined = 0;
	procQueue = new cQueue("procQueue");
}

FullNode::~FullNode()
{
	cancelAndDelete(nextMine);
	cancelAndDelete(nextProc);
	delete procQueue;
}

void FullNode::scheduleNextMine() {
	simtime_t timeToNextBlock = exponential(par("mineIntv").doubleValueInUnit("s"));
	scheduleAt(simTime()+timeToNextBlock, nextMine);
}

void FullNode::procBlock(NewBlock *block) {
	// update the best height
	if (block->getHeight() > bestLevel) {
		bestLevel = block->getHeight();
	}
}

void FullNode::initialize()
{
	WATCH(bestLevel);
	WATCH(blocksMined);

	scheduleNextMine();
}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		// block mining event
		NewBlock *newBlock = new NewBlock("block");
		newBlock->setHeight(bestLevel+1);
		blocksMined += 1;
		procBlock(newBlock);	// process it locally, does not take time
		send(newBlock, "out");
		scheduleNextMine();
	} else if (msg == nextProc) {
		// block processing event
		// take the next block from the queue
		NewBlock *newBlock = (NewBlock*)procQueue->pop();
		procBlock(newBlock);
		delete newBlock;
		// if the queue is not empty, schedule the next processing
		if (!procQueue->isEmpty()) {
			scheduleAt(simTime()+0.1, nextProc);
		}
	} else {
		// schedule processing if no block is waiting for processing
		if (procQueue->isEmpty()) {
			scheduleAt(simTime()+0.1, nextProc);
		}
		// put it into processing queue
		NewBlock *block = (NewBlock*)(msg);
		procQueue->insert(block);
	}

}
