#include <string.h>
#include <omnetpp.h>
#include "NewBlock_m.h"

using namespace omnetpp;

// FullNode is a full node in a blockchain network.
class FullNode : public cSimpleModule
{
	private:
		cMessage *nextBlock;  // pointer to the event object for mining the next block
		int bestLevel;

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
	// ensure the point is null so that destructor will not try to free some random addr
	nextBlock = nullptr;
	bestLevel = 1;
}

FullNode::~FullNode()
{
	cancelAndDelete(nextBlock);
}

void FullNode::initialize()
{
	// schedule the next block to be mined
	nextBlock = new cMessage("mined");
	simtime_t timeToNextBlock = exponential(40);	// PoW on node mines one block every 40 seconds
	scheduleAt(timeToNextBlock, nextBlock);
}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == nextBlock) {
		NewBlock *newBlock = new NewBlock("block");
		newBlock->setHeight(bestLevel+1);
		bestLevel += 1;
		EV << getName() << " now at level " << bestLevel << "\n";
		send(newBlock, "out");
		simtime_t timeToNextBlock = exponential(40);	// PoW on node mines one block every 40 seconds
		scheduleAt(simTime() + timeToNextBlock, nextBlock);
	} else {
		NewBlock *block = (NewBlock*)(msg);
		if (block->getHeight() > bestLevel) {
			bestLevel = block->getHeight();
		}
		delete msg;	// delete the block
	}

}
