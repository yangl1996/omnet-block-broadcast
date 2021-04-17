#include <string.h>
#include <omnetpp.h>
#include "NewBlock_m.h"
#include "NewBlockHash_m.h"

using namespace omnetpp;
using namespace std;

// Packs the miner ID and block sequence number into a long int
long packBlockId(unsigned short miner, unsigned int seq) {
	unsigned long m = (unsigned long)miner;
	unsigned long s = (unsigned long)seq;
	return (long)(m << sizeof(unsigned int)) + s;
}

NewBlockHash* hashFromBlock(NewBlock* block) {
	NewBlockHash* hash = new NewBlockHash();
	hash->setHeight(block->getHeight());
	hash->setMiner(block->getMiner());
	hash->setSeq(block->getSeq());
	return hash;
}

NewBlock* blockFromHash(NewBlockHash* hash) {
	NewBlock* newBlock = new NewBlock();
	newBlock->setHeight(hash->getHeight());
	newBlock->setMiner(hash->getMiner());
	newBlock->setSeq(hash->getSeq());
	return newBlock;
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
		unordered_set<long> reqdBlocks;  // set of blocks that have been requestd

		void scheduleNextMine();
		void procBlock(NewBlock *block);
		void announceBlock(NewBlock *block);
		NewBlock* mineBlock();
		void maybeBroadcastBlockHash(NewBlockHash *hash);

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
	reqdBlocks = unordered_set<long>();
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
void FullNode::procBlock(NewBlock *block) {
	if (block->getHeight() > bestLevel) {
		bestLevel = block->getHeight();
	}
	reqdBlocks.insert(packBlockId(block->getMiner(), block->getSeq()));
}

// Check if we have broadcast the hash before. If not, send out hash to all neighbors and
// record that we have done so.
void FullNode::maybeBroadcastBlockHash(NewBlockHash *hash) {
	long id = packBlockId(hash->getMiner(), hash->getSeq());
	if (heardBlocks.find(id) == heardBlocks.end()) {
		// if we have not heard of this block
		int n = gateSize("link");
		for (int i = 0; i < n; i++) {
			send(hash->dup(), "link$o", i);
		}
		heardBlocks.insert(id);
	}
}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == nextMine) {
		// block mining event
		NewBlock *newBlock = mineBlock();
		procBlock(newBlock);	// process it locally, does not take time
		NewBlockHash *hash = hashFromBlock(newBlock);
		maybeBroadcastBlockHash(hash);
		delete hash;
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
	}
}
