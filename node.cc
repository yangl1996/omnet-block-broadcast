#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

// FullNode is a full node in a blockchain network.
class FullNode : public cSimpleModule
{
	private:
		cMessage *event;  // pointer to the event object for mining the next block

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
	event = nullptr;
}

FullNode::~FullNode()
{
	cancelAndDelete(event);
}

void FullNode::initialize()
{
	// schedule the next block to be mined
	event = new cMessage("mined");
	simtime_t timeToNextBlock = exponential(40);	// PoW on node mines one block every 40 seconds
	scheduleAt(timeToNextBlock, event);
}

void FullNode::handleMessage(cMessage *msg)
{
	if (msg == event) {
		EV << getName() << " mined a block\n";
		cMessage* newBlock = new cMessage("block");
		send(newBlock, "out");
		simtime_t timeToNextBlock = exponential(40);	// PoW on node mines one block every 40 seconds
		scheduleAt(simTime() + timeToNextBlock, event);
	} else {
		EV << getName() << " received a block\n";
		delete msg;	// delete the block
	}

}
