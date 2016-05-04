/*
  ==============================================================================

    IpcServer.cpp
    Created: 29 Jan 2016 4:43:02pm
    Author:  Brett Porter

  ==============================================================================
*/

#include "IpcTest.h"

#include "IpcServer.h"
#include "IpcMessage.h"


namespace
{
     ScopedPointer<FileLogger> logger = FileLogger::createDateStampedLogger(
      "IpcTest",
      "Server_",
      ".txt",
      "*** SERVER ***");  
};

class ValueTreeSyncServer : public ValueTreeSynchroniser
{
public:
  ValueTreeSyncServer(ValueTree& tree, IpcServerConnection* ipcServer, uint32 code)
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

     logger->logMessage(delta);
     IpcMessage msg(fMessageCode, fSequence);
     fSequence = 0;
     msg.AppendData(change, size);
     fServer->SendIpcMessage(msg);
  }

  uint32 GetMessageCode() const
  {
      return fMessageCode;
  }

private:
  /**
   * We need the IpcServer to send messages back to our client.
   */
  IpcServerConnection* fServer;

  /**
   * Each ValueTree that's watched has its own message code 
   */
  uint32      fMessageCode;

  /**
   *  If the client requests the full tree, we need to use the right sequence number.
   */
  uint32      fSequence;

};


IpcServer::IpcServer(ServerController* controller)
:  fController(controller)
{
  this->startTimer(10 * 1000);

}

IpcServer::~IpcServer()
{

}

void IpcServer::timerCallback()
{
    // iterate through the connections -- if any of them are disconnected, delete them. 
    // NOTE that we iterate from the end to the front so we can delete items without
    // needing to worry about goofing up indexes.
    int connectionListSize = fConnections.size();
    if (connectionListSize)
    {
      for (int i = (connectionListSize - 1); i >= 0; --i)
      {
         // IpcServerConnection* ipc = dynamic_cast<IpcServerConnection*>(fConnections.getUnchecked(i));
         IpcServerConnection* ipc = fConnections.getUnchecked(i);
         if (ipc)
         {
            if (IpcServerConnection::kDisconnected == ipc->GetConnectionState())
            {
               // this connection is no longer operative; delete it.
               fConnections.remove(i);
            }
         }
      }
    }
}


InterprocessConnection* IpcServer::createConnectionObject()
{
   // TODO: store into list, periodically delete disconnected connections.
   IpcServerConnection* ipc = new IpcServerConnection(fController);
   fConnections.add(ipc);
   return ipc;
}


IpcServerConnection::IpcServerConnection(ServerController* controller)
:  InterprocessConnection(false, 0xf2b49e2c)
,  fController(controller)
,  fConnected(IpcServerConnection::kConnecting)
{
  DBG("IpcServerConnection created." );
  MessageManagerLock mmLock;
  fController->addChangeListener(this);

}

IpcServerConnection::~IpcServerConnection()
{
  DBG("IpcServerConnection destroyed." );

}


void IpcServerConnection::connectionMade()
{
   DBG("IpcServerConnection::connectionMade()");
   fConnected = IpcServerConnection::kConnected;
   this->WatchValueTree(0, Controller::kValueTree1Update);
   // this->WatchValueTree(1, Controller::kValueTree2Update);

}

void IpcServerConnection::connectionLost()
{
   DBG("IpcServerConnection::connectionLost()");
   MessageManagerLock mmLock;
   fConnected = IpcServerConnection::kDisconnected;
   fController->removeChangeListener(this);
   // stop listening to any ValueTrees...
   fTreeListeners.clear();
   //delete this;
}

void IpcServerConnection::changeListenerCallback(ChangeBroadcaster* source)
{
   if (IpcServerConnection::kConnected == fConnected)
   {
      DBG("TICK");
      IpcMessage notify(Controller::kTimerAlert, 0);
      this->SendIpcMessage(notify);
   }

}

void IpcServerConnection::SendIpcMessage(const IpcMessage& msg)
{
   const ScopedLock mutex(fLock);
   this->sendMessage(msg.GetMemoryBlock());
}


void IpcServerConnection::messageReceived(const MemoryBlock& message)
{
   // a received message from a client needs to be decoded and converted into a 
   // function call that results in us sending a message back over this connection.
   
   IpcMessage ipcMessage(message);

   uint32 messageCode;
   uint32 sequence;
   ipcMessage.GetMetadata(messageCode, sequence);
   DBG("Received message code " + String(messageCode) + " sequence = " + String(sequence));

   IpcMessage response(messageCode, sequence);

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

   this->SendIpcMessage(response);

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


 bool IpcServerConnection::WatchValueTree(int index, uint32 messageCode)
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