/*
  ==============================================================================

    RpcClient.cpp
    Created: 29 Jan 2016 4:43:23pm
    Author:  Brett Porter

  ==============================================================================
*/


#include "RpcTest.h"
#include "RpcClient.h"


#include "Controller.h"

RpcClient::RpcClient()
:  InterprocessConnection(false, 0xf2b49e2c)
,  fController(nullptr)
{

}

RpcClient::~RpcClient()
{

}

void RpcClient::SetController(ClientController* controller)
{
   fController = controller;
}


void RpcClient::connectionMade()
{
   DBG("RpcClient::connectionMade()");
}

void RpcClient::connectionLost()
{
   DBG("RpcClient::connectionLost()");
}

void RpcClient::messageReceived(const MemoryBlock& message)
{
   if (nullptr != fController)
   {
      fController->HandleReceivedMessage(message);
   }
}