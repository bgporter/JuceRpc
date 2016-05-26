/*
  ==============================================================================

    Controller.h
    Created: 29 Jan 2016 4:50:08pm
    Author:  Brett Porter

  ==============================================================================
*/

#ifndef CONTROLLER_H_INCLUDED
#define CONTROLLER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "IpcClient.h"
#include "IpcMessage.h"
#include "PendingCalls.h"

/**
 * abstract base class defining the API that the controller supports.
 */

class Controller : public ChangeBroadcaster
{
public:

   enum FunctionCodes
   {
      /**
       * Codes below 1000 are function calls.
       */
      kVoidFn = 1,
      kIntFn,
      kStringFn,
      /**
       * A range of codes to alter value trees
       */
      kValueTree1SetProp = 1000,
      kValueTree2SetProp,


      /**
       * A range of codes that are only sent from the server to the client
       * as one-way messages.
       */
      kTimerAlert = 10000,
      kValueTree1Update,
      kValueTree2Update
   };

   Controller();

   virtual ~Controller()
   {

   }

   /**
    * Need to be able to call fn returning void
    */

   virtual void VoidFn() = 0;

   /**
    * fn returning int
    */
   virtual int IntFn(int val) = 0;


   /**
    * fn taking/returning strings.
    */
   virtual String StringFn(const String& inString) = 0;


   /**
    * The Controller has two ValueTree objects; request one by index, and 
    * operate on it directly.
    * @param  index Index (0/1) of the tree you'd like to work with
    * @return       ValueTree object.
    */
   ValueTree GetTree(int index);


protected:
  ValueTree   fTree1;
  ValueTree   fTree2;


};




class NullSynchronizer : public ValueTreeSynchroniser
{
public:
   NullSynchronizer(const ValueTree& tree);

   void stateChanged(const void* change, size_t size) override;
   
private:
};

// forward declaration...
class IpcMessage;

class ClientController: public Controller
                      // , public ChangeBroadcaster
{
public:
  ClientController(IpcClient* ipc);

  ~ClientController();


  /**
   * Attempt to establish a connection to a server at a specified domain 
   * name or IP address.
   * @param  hostName   Name or IP address to connect to
   * @param  portNumber Port number
   * @param  msTimeout  Number of milliseconds to wait for a successful 
   *                    connection.
   * @return            True if we connected.
   */
  bool ConnectToServer(const String& hostName, int portNumber, int msTimeout);

  /**
   * Called when we receive a new message from the server. It's either going to be 
   * - a response to a function call we made 
   * - a change notification that won't be in the pending calls table.
   * @param message Chunk of data from the server.
   */
  void HandleReceivedMessage(const MemoryBlock& message);

   /**
    * Need to be able to call fn returning void
    */

   void VoidFn() override;

   /**
    * fn returning int
    */
   int IntFn(int val) override;


   /**
    * fn taking/returning strings.
    */
   String StringFn(const String& inString) override;    


   template <typename T>
   void SetTreeProperty(uint32 messageCode, const String& path, IpcMessage::DataType type, T val)
   {
      IpcMessage msg(messageCode);
      IpcMessage response;

      msg.SetTreeProperty<T>(path, type, val);
      if (this->CallFunction(msg, response))
      {
         // nothing to do.
      }
      else
      {
          DBG("ERROR setting tree property");
      }
   }
private:

  /**
   * Perform a function call across the socket connection. 
   * @param  call     Populated IpcMessage object containing message sequence, 
   *                  function code, and optional block of parameter data
   * @param  response IcpMessage object that will be populated on 
   *                  exit with the contents of the response from the server
   * @return          True if the call completed successfully. False if the
   *                  server timed out or there was some other error.
   */
  bool CallFunction(IpcMessage& call, IpcMessage& response);

  bool UpdateValueTree(int index, const void* data, size_t size);

private:

  ScopedPointer<IpcClient> fIpc;
  PendingCallList fPending;


  ScopedPointer<FileLogger> fLogger;

  NullSynchronizer fSync;
};



class ServerController: public Controller
                      // , public ChangeBroadcaster
                      , public Timer
{
public:
  ServerController();

  ~ServerController();

   /**
    * Need to ba able to call fn returning void
    */

   void VoidFn() override;

   /**
    * fn returning int
    */
   int IntFn(int val) override;


   /**
    * fn taking/returning strings.
    */
   String StringFn(const String& inString) override;  

   /**
    * Function called by our timer that will send change notifications back to the clients. 
    */
   void timerCallback() override;


  int         fTimerCount;
    
};

#endif  // CONTROLLER_H_INCLUDED
