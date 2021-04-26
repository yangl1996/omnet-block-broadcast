#include <string.h>
#include <omnetpp.h>
#include "ethp2p_m.h"

using namespace omnetpp;
using namespace std;

// Miner implements a blockchain miner.
// It supports three mining modes, each mimics a type of permissionless consensus protocol:
//   Continuous mode, where blocks are mined as in PoW. Set ronudIntv and numFixedMiners
//   to zero, and set miningRate to the desired per-node mining rate.
//
//   Round-by-round mode, where blocks are mined as in PoS. Set roundIntv to the rount
//   interval, and set miningRate to the desired per-node mining rate (blocks per
//   second). Set numFixedMiners to zero.
//
//   Fixed miner mode, where the same set of miners mine one block in each round. Set
//   numFixedMiners to the number of miners in each round, and set roundIntv to the
//   desired round interval. Ignore the mining rate setting.
class Miner : public cSimpleModule
{
	private:
		// parameters
		unsigned short id;          // id of the node
		int numFixedMiners; // number of miners in each round; used for network experiments
		double roundTime; // round time (round mode), 0 for continuous time mode
		cExponential rvBlockDelay; // block inter-arrival time (continuous time mode)
		cPoisson rvBlocksPerRound; // blocks per round (round mode)

		// internal states
		unsigned int nextBlockSeq;    // sequence of the block; combined with id identifies a block
		cMessage *nextMine;  // event when a block is mined
		cMessage *nextProcBlock;  // event when a block is processed
		cQueue blockProcQueue;   // blocks to be processed
		unsigned int bestLevel;       // the highest block level

		// internal methods
		void scheduleNextMine();
		void procBlock(NewBlock *block);
		NewBlock* mineBlock();

		// stats
		cHistogram delayStats;	// records the block delay
		// TODO: also try cPSquare

	public:
		Miner();
		virtual ~Miner();

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
};

// Register the module with omnet
Define_Module(Miner);

Miner::Miner()
{
	nextMine = new cMessage("mined");
	nextProcBlock = new cMessage("proced");
	bestLevel = 0;
	nextBlockSeq = 0;
	blockProcQueue = cQueue("blockProcQueue");
	delayStats = cHistogram("blockDelay", 200);
}

Miner::~Miner()
{
	cancelAndDelete(nextMine);
	cancelAndDelete(nextProcBlock);
}

void Miner::initialize()
{
	id = par("id").intValue(); // cannot be placed in the constructor because the parameter was not ready
	WATCH(bestLevel);
	WATCH(nextBlockSeq);

	// set up mining RVs
	numFixedMiners = par("numFixedMiners").intValue();
	roundTime = par("roundIntv").doubleValueInUnit("s");
	double miningRate = par("miningRate").doubleValue(); // in blocks per second
	if (roundTime == 0.0) {
		// continuous time mode
		rvBlockDelay = cExponential(getRNG(0), 1.0 / miningRate);
	}
	else {
		// round mode
		rvBlocksPerRound = cPoisson(getRNG(0), roundTime * miningRate);
	}

	scheduleNextMine();
}

// Randomly samples an exponential delay and schedules the nextMine
// event to happen after that time.
void Miner::scheduleNextMine() {
	if (roundTime == 0.0) {
		scheduleAt(simTime()+rvBlockDelay.draw(), nextMine);
	}
	else {
		scheduleAt(simTime()+roundTime, nextMine);
	}
}

// Creates a NewBlock message with appropriate height, miner ID, and sequence number, and
// increases the block sequence number.
NewBlock* Miner::mineBlock() {
	NewBlock *newBlock = new NewBlock("block");
	newBlock->setHeight(bestLevel+1);
	newBlock->setMiner(id);
	newBlock->setSeq(nextBlockSeq);
	newBlock->setTimeMined(simTime());
	nextBlockSeq += 1;
	return newBlock;
}

// Processes a new block. Update the best height, and records the reception of the block.
// Then announces the block to peers.
void Miner::procBlock(NewBlock *block) {
	delayStats.collect(simTime() - block->getTimeMined());
	if (block->getHeight() > bestLevel) {
		bestLevel = block->getHeight();
	}
	send(block, "p2p$o");
}

void Miner::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		// block mining event
		if (roundTime == 0.0) {
			NewBlock *newBlock = mineBlock();
			procBlock(newBlock);	// process it locally, does not take time
		}
		else {
			int nBlocks;
			if (numFixedMiners > 0) {
				nBlocks = id < numFixedMiners? 1 : 0;
			}
			else {
				nBlocks = int(rvBlocksPerRound.draw());
			}
			for (int i = 0; i < nBlocks; i++) {
				NewBlock *newBlock = mineBlock();
				procBlock(newBlock);	// process it locally, does not take time
			}
		}
		scheduleNextMine();
	} else if (msg == nextProcBlock) {
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

void Miner::finish() {
	EV << "Node " << id << " max delay: " << delayStats.getMax() << endl;
	EV << "Node " << id << " min delay: " << delayStats.getMin() << endl;
}
