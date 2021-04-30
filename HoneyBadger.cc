#include <string.h>
#include <omnetpp.h>
#include "p2p_m.h"

using namespace omnetpp;
using namespace std;

// HoneyBadger implements a simplified version of HoneyBadgerBFT. The protocol runs in
// epochs. In each epoch, every node mines a block. A node advances into the next epoch
// upon receiving all blocks.
class HoneyBadger: public cSimpleModule
{
	private:
		// parameters
		unsigned short id;          // id of the node
		int numNodes;	// total number of nodes

		// internal states
		unsigned int nextBlockSeq;    // sequence of the next epoch 
		cMessage *nextMine;
		cMessage *nextProcBlock;  // event when a block is processed
		cQueue blockProcQueue;   // blocks to be processed
		unordered_map<int, int> epochs;

		// internal methods
		void procBlock(NewBlock *block);
		NewBlock* mineBlock();

		// stats
		cHistogram delayStats;	// records the block delay
		// TODO: also try cPSquare

	public:
		HoneyBadger();
		virtual ~HoneyBadger();

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
};

// Register the module with omnet
Define_Module(HoneyBadger);

HoneyBadger::HoneyBadger()
{
	nextMine = new cMessage("mined");
	nextProcBlock = new cMessage("proced");
	nextBlockSeq = 0;
	blockProcQueue = cQueue("blockProcQueue");
	epochs = unordered_map<int, int>();
	delayStats = cHistogram("blockDelay", 200);
}

HoneyBadger::~HoneyBadger()
{
	cancelAndDelete(nextProcBlock);
	cancelAndDelete(nextMine);
}

void HoneyBadger::initialize()
{
	numNodes = par("numNodes").intValue();
	id = par("id").intValue(); // cannot be placed in the constructor because the parameter was not ready
	WATCH(nextBlockSeq);
	scheduleAt(simTime(), nextMine);	// start the first epoch
}

// Creates a NewBlock message with appropriate miner ID and sequence (epoch) number, and
// increases the block sequence number.
NewBlock* HoneyBadger::mineBlock() {
	NewBlock *newBlock = new NewBlock("block");
	newBlock->setMiner(id);
	newBlock->setSeq(nextBlockSeq);
	newBlock->setTimeMined(simTime());
	nextBlockSeq += 1;
	return newBlock;
}

// Processes a new block. Update the best height, and records the reception of the block.
// Then announces the block to peers.
void HoneyBadger::procBlock(NewBlock *block) {
	delayStats.collect(simTime() - block->getTimeMined());
	int epoch = block->getSeq();
	if (epochs.find(epoch) == epochs.end()) {
		epochs[epoch] = 1;
	}
	else {
		epochs[epoch] += 1;
	}

	if (epochs.find(nextBlockSeq-1) != epochs.end() && epochs[nextBlockSeq-1] >= numNodes) {
		scheduleAt(simTime(), nextMine);
	}
	send(block, "p2p$o");
}

void HoneyBadger::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		NewBlock *newBlock = mineBlock();
		procBlock(newBlock);	// process it locally, does not take time
		return;
	}

	if (msg == nextProcBlock) {
		// block processing event
		// take the next block from the queue
		NewBlock *newBlock = check_and_cast<NewBlock*>(blockProcQueue.pop());
		procBlock(newBlock);
		// if the queue is not empty, schedule the next processing
		if (!blockProcQueue.isEmpty()) {
			scheduleAt(simTime()+par("procTime").doubleValueInUnit("s"), nextProcBlock);
		}
	} else {
		// check message type
		NewBlock *newBlock = dynamic_cast<NewBlock*>(msg);
		if (newBlock != nullptr) {
			// schedule processing if no block is waiting for processing
			if (blockProcQueue.isEmpty()) {
				scheduleAt(simTime()+par("procTime").doubleValueInUnit("s"), nextProcBlock);
			}
			// put it into processing queue
			blockProcQueue.insert(msg);
			return;
		}
	}
}

void HoneyBadger::finish() {
	EV << "Node " << id << " max delay: " << delayStats.getMax() << endl;
	EV << "Node " << id << " min delay: " << delayStats.getMin() << endl;
}
