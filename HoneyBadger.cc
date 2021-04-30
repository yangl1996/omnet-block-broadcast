#include <string.h>
#include <omnetpp.h>
#include "p2p_m.h"
#include "honeybadger_m.h"

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
		simtime_t lastEpochFinish;

		// internal methods
		void procBlock(NewBlock *block);
		NewBlock* mineBlock();
		void confirmReception(unsigned int epoch);

		// stats
		cHistogram roundIntvStats;	// records the block delay
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
	roundIntvStats = cHistogram("roundInterval", 200);
	lastEpochFinish = 0;
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
	Block blk;
	blk.miner = id;
	blk.seq = nextBlockSeq;
	blk.timeMined = simTime();
	newBlock->setBlock(blk);
	nextBlockSeq += 1;
	return newBlock;
}

void HoneyBadger::procBlock(NewBlock *block) {
	Block blk = block->getBlock();
	int epoch = blk.seq;
	confirmReception(epoch);
	send(block, "p2p$o");
	GotBlock *msg = new GotBlock();
	msg->setEpoch(epoch);
	msg->setNode(id);
	send(msg, "p2p$o");
}

void HoneyBadger::confirmReception(unsigned int epoch) {
	if (epochs.find(epoch) == epochs.end()) {
		epochs[epoch] = 1;
	}
	else {
		epochs[epoch] += 1;
	}

	// note that we want to only check if epoch==nextBlockSeq-1. otherwise we risk scheduling multiple nextMine events.
	if (epoch == nextBlockSeq-1 && epochs.find(nextBlockSeq-1) != epochs.end() && epochs[nextBlockSeq-1] >= numNodes*numNodes) {
		scheduleAt(simTime(), nextMine);
		roundIntvStats.collect(simTime() - lastEpochFinish);
		lastEpochFinish = simTime();
	}
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

		GotBlock *gotBlock = dynamic_cast<GotBlock*>(msg);
		if (gotBlock != nullptr) {
			confirmReception(gotBlock->getEpoch());
			delete gotBlock;
			return;
		}
	}
}

void HoneyBadger::finish() {
	EV << "Node " << id << " max intv: " << roundIntvStats.getMax() << endl;
	EV << "Node " << id << " min intv: " << roundIntvStats.getMin() << endl;
}
