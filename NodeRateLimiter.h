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
                int outQueueLength() const;

        protected:
                virtual void initialize() override;
                virtual void finish() override;
                virtual void handleMessage(cMessage *msg) override;
                cGate* getSendGate(cMessage *msg);
                bool isFromOutside(cMessage *msg);
};
