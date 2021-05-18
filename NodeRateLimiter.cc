#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;
using namespace std;

int compare(cObject *a, cObject *b) {
	cMessage* m1 = dynamic_cast<cMessage*>(a);
	cMessage* m2 = dynamic_cast<cMessage*>(b);
	int idx1 = m1->getArrivalGate()->getIndex();
	int idx2 = m2->getArrivalGate()->getIndex();
	return (idx2-idx1);
}

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
		int outQueueLength() const;

	protected:
		virtual void initialize() override;
		virtual void finish() override;
		virtual void handleMessage(cMessage *msg) override;
		cGate* getSendGate(cMessage *msg);
		bool isFromOutside(cMessage *msg);
};

// Register the module with omnet
Define_Module(NodeRateLimiter);

NodeRateLimiter::NodeRateLimiter()
{
	nextSend = new cMessage("send");
	nextReceive = new cMessage("receive");
	incomingQueue = cQueue("incoming");
	outgoingQueue = cQueue("outgoing", compare);
}

NodeRateLimiter::~NodeRateLimiter()
{
	cancelAndDelete(nextSend);
	cancelAndDelete(nextReceive);
}

int NodeRateLimiter::outQueueLength() const {
	return outgoingQueue.getLength();
}

void NodeRateLimiter::initialize()
{
	innerGateStartId = gate("inner$i", 0)->getBaseId();
	outerGateStartId = gate("outer$i", 0)->getBaseId();
	incomingRate = par("incomingRate").doubleValueInUnit("bps");
	outgoingRate = par("outgoingRate").doubleValueInUnit("bps");
}

bool NodeRateLimiter::isFromOutside(cMessage *msg) {
	cGate* fromGate = msg->getArrivalGate();
	int baseId = fromGate->getBaseId();
	if (baseId == innerGateStartId) {
		return false;
	}
	else if (baseId == outerGateStartId) {
		return true;
	}
	else {
		throw 10;
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
		throw 10;
	}
}

void NodeRateLimiter::handleMessage(cMessage *msg)
{
	// internal events
	if (msg == nextSend) {
		cPacket *toSend = check_and_cast<cPacket*>(outgoingQueue.pop());
		send(toSend, getSendGate(toSend));
		if (!outgoingQueue.isEmpty()) {
			simtime_t t = ((cPacket*)outgoingQueue.front())->getBitLength() / outgoingRate;
			scheduleAt(simTime()+t, nextSend);
		}
		return;
	}
	else if (msg == nextReceive) {
		cPacket *toReceive = check_and_cast<cPacket*>(incomingQueue.pop());
		send(toReceive, getSendGate(toReceive));
		if (!incomingQueue.isEmpty()) {
			simtime_t t = ((cPacket*)incomingQueue.front())->getBitLength() / incomingRate;
			scheduleAt(simTime()+t, nextReceive);
		}
		return;
	}

	// external messages
	cPacket *pkt = dynamic_cast<cPacket*>(msg);
	if (pkt != nullptr) {
		if (isFromOutside(pkt)) {
			if (incomingRate == 0.0) {
				send(pkt, getSendGate(pkt));
				return;
			}
			if (incomingQueue.isEmpty()) {
				simtime_t t = pkt->getBitLength() / incomingRate;
				scheduleAt(simTime()+t, nextReceive);
			}
			incomingQueue.insert(pkt);
		}
		else {
			if (outgoingRate == 0.0) {
				send(pkt, getSendGate(pkt));
				return;
			}
			if (outgoingQueue.isEmpty()) {
				simtime_t t = pkt->getBitLength() / outgoingRate;
				scheduleAt(simTime()+t, nextSend);
			}
			outgoingQueue.insert(pkt);
		}
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
