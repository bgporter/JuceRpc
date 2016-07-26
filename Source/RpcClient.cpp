/*
  Copyright 2016 Art & Logic Software Development. 
 */




#include "RpcTest.h"
#include "RpcClient.h"


#include "Controller.h"

RpcClient::RpcClient()
:  InterprocessConnection(false, 0xf2b49e2c)
,  fController(nullptr)
,  fIsConnected(false)
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
   fIsConnected = true;
}

void RpcClient::connectionLost()
{
   DBG("RpcClient::connectionLost()");
   fIsConnected = true;
}

void RpcClient::messageReceived(const MemoryBlock& message)
{
   if (nullptr != fController)
   {
      fController->HandleReceivedMessage(message);
   }
}