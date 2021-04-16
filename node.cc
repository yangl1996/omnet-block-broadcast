#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

// FullNode is a full node in a blockchain network.
class FullNode : public cSimpleModule
{
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

// Register the module with omnet
Define_Module(FullNode);

void FullNode::initialize()
{
    // Am I Tic or Toc?
    EV << "sending\n";
    if (strcmp("tic", getName()) == 0) {
        // create and send first message on gate "out". "tictocMsg" is an
        // arbitrary string which will be the name of the message object.
        cMessage *msg = new cMessage("tictocMsg");
        send(msg, "out");
    }
}

void FullNode::handleMessage(cMessage *msg)
{
    // The handleMessage() method is called whenever a message arrives
    // at the module. Here, we just send it to the other module, through
    // gate `out'. Because both `tic' and `toc' does the same, the message
    // will bounce between the two.
    send(msg, "out"); // send out the message
}
