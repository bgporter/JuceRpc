/*
  Copyright 2016 Art & Logic Software Development. 
 */



#include "RpcTest.h"

#include "RpcServer.h"
#include "RpcMessage.h"

#if 0
namespace
{
     ScopedPointer<FileLogger> logger = FileLogger::createDateStampedLogger(
      "RpcTest",
      "Server_",
      ".txt",
      "*** SERVER ***");  
};

#endif


class ValueTreeSyncServer : public ValueTreeSynchroniser
{
public:
  ValueTreeSyncServer(ValueTree& tree, RpcServerConnection* ipcServer, uint32 code)
  :   ValueTreeSynchroniser(tree)
  ,   fServer(ipcServer)
  ,   fMessageCode(code)
  ,   fSequence(0)
  {

  }

  ~ValueTreeSyncServer()
  {

  }

  void stateChanged(const void* change, size_t size)
  {
     DBG("ValueTree code " + String(fMessageCode) + " has changed; " + String(size) + " bytes of data.");
     String delta = String("> ") + String::toHexString(change, size);
     DBG(delta);

     RpcMessage msg(fMessageCode, fSequence);
     fSequence = 0;
     msg.AppendData(change, size);
     fServer->SendRpcMessage(msg);
  }

  uint32 GetMessageCode() const
  {
      return fMessageCode;
  }

private:
  /**
   * We need the RpcServer to send messages back to our client.
   */
  RpcServerConnection* fServer;

  /**
   * Each ValueTree that's watched has its own message code 
   */
  uint32      fMessageCode;

  /**
   *  If the client requests the full tree, we need to use the right sequence number.
   */
  uint32      fSequence;

};


RpcServer::RpcServer(ServerController* controller)
:  fController(controller)
{
  this->startTimer(10 * 1000);

}

RpcServer::~RpcServer()
{

}

void RpcServer::timerCallback()
{
    // iterate through the connections -- if any of them are disconnected, delete them. 
    // NOTE that we iterate from the end to the front so we can delete items without
    // needing to worry about goofing up indexes.
    int connectionListSize = fConnections.size();
    if (connectionListSize)
    {
      for (int i = (connectionListSize - 1); i >= 0; --i)
      {
         // RpcServerConnection* ipc = dynamic_cast<RpcServerConnection*>(fConnections.getUnchecked(i));
         RpcServerConnection* ipc = fConnections.getUnchecked(i);
         if (ipc)
         {
            if (RpcServerConnection::kDisconnected == ipc->GetConnectionState())
            {
               // this connection is no longer operative; delete it.
               fConnections.remove(i);
            }
         }
      }
    }
}


InterprocessConnection* RpcServer::createConnectionObject()
{
   // TODO: store into list, periodically delete disconnected connections.
   RpcServerConnection* ipc = new RpcServerConnection(fController);
   fConnections.add(ipc);
   return ipc;
}


RpcServerConnection::RpcServerConnection(ServerController* controller)
:  InterprocessConnection(false, 0xf2b49e2c)
,  fController(controller)
,  fConnected(RpcServerConnection::kConnecting)
{
  DBG("RpcServerConnection created." );
  MessageManagerLock mmLock;
  fController->addChangeListener(this);
}

RpcServerConnection::~RpcServerConnection()
{
  DBG("RpcServerConnection destroyed." );

}


void RpcServerConnection::connectionMade()
{
   DBG("RpcServerConnection::connectionMade()");
   fConnected = RpcServerConnection::kConnected;
   this->WatchValueTree(0, Controller::kValueTree1Update);
   // this->WatchValueTree(1, Controller::kValueTree2Update);

}

void RpcServerConnection::connectionLost()
{
   DBG("RpcServerConnection::connectionLost()");
   MessageManagerLock mmLock;
   fConnected = RpcServerConnection::kDisconnected;
   fController->removeChangeListener(this);
   // stop listening to any ValueTrees...
   fTreeListeners.clear();
   //delete this;
}

void RpcServerConnection::changeListenerCallback(ChangeBroadcaster* source)
{
   if (RpcServerConnection::kConnected == fConnected)
   {
      DBG("TICK");
      RpcMessage notify(Controller::kTimerAlert, 0);
      this->SendRpcMessage(notify);
   }

}

void RpcServerConnection::SendRpcMessage(const RpcMessage& msg)
{
   const ScopedLock mutex(fLock);
   this->sendMessage(msg.GetMemoryBlock());
}


void RpcServerConnection::messageReceived(const MemoryBlock& message)
{
   // a received message from a client needs to be decoded and converted into a 
   // function call that results in us sending a message back over this connection.
   
   RpcMessage ipcMessage(message);

   uint32 messageCode;
   uint32 sequence;
   ipcMessage.GetMetadata(messageCode, sequence);
   DBG("Received message code " + String(messageCode) + " sequence = " + String(sequence));

   RpcMessage response(messageCode, sequence);

   switch (messageCode)
   {
      case Controller::kVoidFn:
      {
         fController->VoidFn();
      }
      break;

      case Controller::kIntFn:
      {
         int arg = ipcMessage.GetData<int>();
         DBG("IntFn arg = " + String(arg));
         int retval = fController->IntFn(arg);
         DBG("IntFn retval = " + String(retval));

         response.AppendData(retval);
      }
      break;

      case Controller::kStringFn:
      {
        String arg = ipcMessage.GetString();
        DBG("StringFn arg = " + arg);
        String retval = fController->StringFn(arg);
        DBG("StringFn retval = " + retval);

        response.AppendString(retval);

      }
      break;

      case Controller::kValueTree1Update:
      case Controller::kValueTree2Update:
      {
          // client is explicitly requesting the full tree update.
          for (int i = 0; i < fTreeListeners.size(); ++i)
          {
             ValueTreeSyncServer* vts = fTreeListeners.getUnchecked(i);
             if (messageCode == vts->GetMessageCode())
             {
                // we found the sync handler for that tree. Get a full sync:
                vts->sendFullSyncCallback();
                // return immediately -- the listener code will send back the response.
                return;
             }
          }

      }
      break;

      default:
      {
         DBG("Received unknown message code" + String(messageCode));
      }
      break;
   }

   this->SendRpcMessage(response);

    if ((messageCode >= Controller::kValueTree1SetProp) && (messageCode < Controller::kTimerAlert))
   {
      // value tree changes should take place after we've sent the (void) response back to the 
      // client.
      switch (messageCode)
      {
          case Controller::kValueTree2SetProp:
          {
              ValueTree tree = fController->GetTree(1);
              ipcMessage.ApplyTreeProperty(tree);
          }  
      } 
   }

}


 bool RpcServerConnection::WatchValueTree(int index, uint32 messageCode)
 {
    ValueTree tree = fController->GetTree(index);
    // bool retval = fController->GetTree(index, tree);
    bool retval = (ValueTree::invalid != tree);
    if (retval)
    {
        // connect this sync server to the value tree
        ValueTreeSyncServer* vts = new ValueTreeSyncServer(tree, this, messageCode);
        fTreeListeners.add(vts);
        // and force a full update 
        vts->sendFullSyncCallback();

    }
    return retval;

 }