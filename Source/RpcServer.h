/*
  ==============================================================================

    RpcServer.h
    Created: 29 Jan 2016 4:43:02pm
    Author:  Brett Porter

  ==============================================================================
*/

#ifndef IPCSERVER_H_INCLUDED
#define IPCSERVER_H_INCLUDED

#include "Controller.h"

class RpcMessage;
class RpcServerConnection;

class RpcServer : public InterprocessConnectionServer
                , public Timer
{
public: 
   RpcServer(ServerController* controller);

   ~RpcServer();

   /**
    * Will actually return an instance of RpcServerConnection (below)
    * @return [pointer to server connection]
    */
   InterprocessConnection* createConnectionObject();

   /**
    * Housekeeping -- periodically cleans out any disconnected ServerConnection 
    * objects that aren't needed any more.
    */
   void timerCallback();



private:
   ScopedPointer<ServerController> fController;
   OwnedArray<RpcServerConnection> fConnections;

};



// forward ref
class ValueTreeSyncServer;

class RpcServerConnection : public InterprocessConnection
                          , public ChangeListener
{
public:
   RpcServerConnection(ServerController* controller);

   ~RpcServerConnection();

   enum ConnectionState
   {
      kConnecting = 0,
      kConnected, 
      kDisconnected
   };

   void connectionMade() override;

   void connectionLost() override;

   void messageReceived(const MemoryBlock& message) override;

   void changeListenerCallback(ChangeBroadcaster* source) override;

   void SendRpcMessage(const RpcMessage& msg);

   /**
    * ValueTree-related functions:
    */
   
   bool WatchValueTree(int index, uint32 messageCode);

   ConnectionState GetConnectionState() const { return fConnected; };


private:
   // raw pointer; we do NOT own this controller.
   ServerController* fController;

   // When ValueTrees change, these listeners will make sure that clients 
   // receive sync info.
   OwnedArray<ValueTreeSyncServer>  fTreeListeners;

   CriticalSection fLock;

   ConnectionState fConnected;
};



#endif  // IPCSERVER_H_INCLUDED
