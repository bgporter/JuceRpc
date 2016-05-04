/*
  ==============================================================================

    IpcClient.h
    Created: 29 Jan 2016 4:43:23pm
    Author:  Brett Porter

  ==============================================================================
*/

#ifndef IPCCLIENT_H_INCLUDED
#define IPCCLIENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"


class ClientController;

class IpcClient : public InterprocessConnection
{
public:
   IpcClient();

   ~IpcClient();

   void SetController(ClientController* controller);

   void connectionMade() override;

   void connectionLost() override;

   void messageReceived(const MemoryBlock& message) override;


private:

   ClientController* fController;

};



#endif  // IPCCLIENT_H_INCLUDED
