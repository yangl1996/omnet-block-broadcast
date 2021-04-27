#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;
using namespace std;


// NodeRateLimiter imposes node capacity limits. 
class NodeRateLimiter : public cSimpleModule
{
	private:
		// internal states
		cMessage *nextSend;
		cMessage *nextReceive;
		cQueue incomingQueue;
		cQueue outgoingQueue;
		double incomingRate;
		double outgoingRate;
		int innerGateStartId;
		int outerGateStartId;

	public:
		NodeRateLimiter();
		virtual ~NodeRateLimiter();

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
		cGate* getSendGate(cMessage *msg);
		bool fromOutside(cMessage *msg);
};

// Register the module with omnet
Define_Module(NodeRateLimiter);

NodeRateLimiter::NodeRateLimiter()
{
	nextSend = new cMessage("send");
	nextReceive = new cMessage("receive");
	incomingQueue = cQueue("incoming");
	outgoingQueue = cQueue("outgoing");
}

NodeRateLimiter::~NodeRateLimiter()
{
	cancelAndDelete(nextSend);
	cancelAndDelete(nextReceive);
}

void NodeRateLimiter::initialize()
{
	innerGateStartId = gate("inner$i", 0)->getBaseId();
	outerGateStartId = gate("outer$i", 0)->getBaseId();
}

bool NodeRateLimiter::fromOutside(cMessage *msg) {
	cGate* fromGate = msg->getArrivalGate();
	int baseId = fromGate->getBaseId();
	if (baseId == innerGateStartId) {
		return false;
	}
	else if (baseId == outerGateStartId) {
		return true;
	}
	else {
		// should not happen
		EV << "help me" << endl;
		return false;
		
	}
}

cGate* NodeRateLimiter::getSendGate(cMessage *msg) {
	cGate* fromGate = msg->getArrivalGate();
	int baseId = fromGate->getBaseId();
	int gateIdx = fromGate->getIndex();
	if (baseId == innerGateStartId) {
		// coming from the inner node
		return gate("outer$o", gateIdx);
	}
	else if (baseId == outerGateStartId) {
		// coming from the outside world
		return gate("inner$o", gateIdx);
	}
	else {
		// this really should not happen
		EV << "help me" << endl;
		return nullptr;
	}
}

void NodeRateLimiter::handleMessage(cMessage *msg)
{
	cPacket *pkt = dynamic_cast<cPacket*>(pkt);
	if (pkt != nullptr) {
		return;
	}
	else {
		// we only limit rate on packet types
		cGate *sendGate = getSendGate(msg);
		send(msg, sendGate);
	}
}

void NodeRateLimiter::finish() {
	// nothing to do here
}
