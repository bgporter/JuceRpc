/*
  ==============================================================================

    IpcClient.cpp
    Created: 29 Jan 2016 4:43:23pm
    Author:  Brett Porter

  ==============================================================================
*/


#include "IpcTest.h"
#include "IpcClient.h"


#include "Controller.h"

IpcClient::IpcClient()
:  InterprocessConnection(false, 0xf2b49e2c)
,  fController(nullptr)
{

}

IpcClient::~IpcClient()
{

}

void IpcClient::SetController(ClientController* controller)
{
   fController = controller;
}


void IpcClient::connectionMade()
{
   DBG("IpcClient::connectionMade()");
}

void IpcClient::connectionLost()
{
   DBG("IpcClient::connectionLost()");
}

void IpcClient::messageReceived(const MemoryBlock& message)
{
   if (nullptr != fController)
   {
      fController->HandleReceivedMessage(message);
   }
}