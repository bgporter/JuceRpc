/*
  Copyright 2016 Art & Logic Software Development. 
 */



#include "Controller.h"
#include "RpcException.h"
#include "RpcMessage.h"





Controller::Controller()
:  fTree1("one")
,  fTree2("two")
{
    
}


ValueTree Controller::GetTree(int index)
{
   ValueTree retval = ValueTree::invalid;
   switch(index)
   {
      case 0:
      retval = fTree1;
      break;

      case 1: 
      retval = fTree2;
      break;

      default:
      break;
   }
   return retval;
}



NullSynchronizer::NullSynchronizer(const ValueTree& tree) 
: ValueTreeSynchroniser(tree)
{

}

void NullSynchronizer::stateChanged(const void* change, size_t size)
{
   DBG("Got " + String(size) + " bytes of valueTree data");
}


ClientController::ClientController(RpcClient* ipc)
:  fRpc(ipc)
,  fSync(fTree1)
{
   #if 0
   fLogger = FileLogger::createDateStampedLogger(
      "RpcTest",
      "Client_",
      ".txt",
      "*** CLIENT ***");
   #endif


   fRpc->SetController(this);


   //fTree1.setProperty("test", 1, nullptr);
}

ClientController::~ClientController()
{
   fRpc->disconnect();
}

bool ClientController::ConnectToServer(const String& hostName, int portNumber, int msTimeout)
{
   return fRpc->connectToSocket(hostName, portNumber, msTimeout);
}

void ClientController::HandleReceivedMessage(const MemoryBlock& message)
{
   RpcMessage ipc(message);

   uint32 code;
   uint32 sequence;

   ipc.GetMetadata(code, sequence);
   DBG("RECEIVED message back, code = " + String(code) + ", sequence = " + String(sequence));

   if (sequence != 0)
   {

      PendingCall* pc = fPending.FindCallBySequence(sequence);
      if (pc)
      {
         // copy in the reply data and wake that thread up.
         pc->SetMemoryBlock(message);
         pc->Signal();

      }
      else 
      {
         DBG("ERROR: Unexpected reply to call sequence #" + String(sequence));
      }
   }
   else
   {
      // this is a change notification and we're not expecting it.
      if (Controller::kValueTree1Update == code)
      {
         ValueTree tree = this->GetTree(0);
         if (tree != ValueTree::invalid)
         {
            DBG("BEFORE: ");
            //fTree1.setProperty("foo", Random().nextInt(), nullptr);
            DBG(fTree1.toXmlString());
            String delta = String("> ") + ipc.GetValueTree(fTree1);
            //DBG(tree.toXmlString());
            //String delta = String("> ") + ipc.GetValueTree(tree);
            // fLogger->logMessage(delta);
         }
          /*
          var count = fTree1.getProperty("count");
          DBG("*** Tree 1 count = " + count.toString());
          ValueTree sub = fTree1.getChildWithName("sub");
          var txt = sub.getProperty("text");
          DBG("*** tree1.sub.text == `" + txt.toString()+ "'");
          */
          DBG("\nAFTER:");
          // DBG(tree.toXmlString());

          // DBG(fTree1.toXmlString());
          DBG(fTree1.toXmlString());
          DBG(fTree1.isEquivalentTo(tree));
      }
      else if (Controller::kValueTree2Update == code)
      {
         ipc.GetValueTree(fTree2);
      }
      else
      {
         DBG("Change notification, code " + String(code));
         this->sendChangeMessage();
      }
   }
}

void ClientController::VoidFn()
{
   RpcMessage msg(Controller::kVoidFn);
   RpcMessage response;

   if (this->CallFunction(msg, response))
   {
      // nothing to do. 
   }
   else 
   {
      DBG("ERROR calling VoidFn();");
   }
}

int ClientController::IntFn(int val)
{
   RpcMessage msg(Controller::kIntFn);
   RpcMessage response;

   int retval = 0;
   msg.AppendData(val);
   if (this->CallFunction(msg, response))
   {
      retval = response.GetData<int>();
   }
   else
   {
      DBG("ERROR calling IntFn();");
   }

   return retval;
}


String ClientController::StringFn(const String& inString)
{
   RpcMessage msg(Controller::kStringFn);
   RpcMessage response;

   String retval;
   msg.AppendString(inString);
   if (this->CallFunction(msg, response))
   {
      retval = response.GetString();
      DBG("StringFn returns " + retval);
   }
   else
   {
      DBG("ERROR calling StringFn();");
   }
   return retval;
}  

bool ClientController::CallFunction(RpcMessage& call, 
                                    RpcMessage& response)
{
   bool retval = false;
   uint32 messageCode;
   uint32 sequence;

   call.GetMetadata(messageCode, sequence);

   // Remember which call we're waiting for. 
   PendingCall pc(sequence);
   fPending.Append(&pc);

   if (fRpc->sendMessage(call.GetMemoryBlock()))
   {
      // wait for a response
      if (pc.Wait(50000))
      {
         // if we get here, the pending call object has a memoryBlock 
         // that we can use to populate our response.
         response.FromMemoryBlock(pc.GetMemoryBlock());
         // advance past the header bits...
         response.GetMetadata(messageCode, sequence);
         retval = true;

      }
      else
      {  
         DBG("ERROR (timeout?) calling function code = " + String(messageCode));
         throw RpcException<Controller::kTimeout>();
      }
   }
   else
   {
      DBG("ERROR sending call message.");
   }

   // remove the pending call object from the list
   fPending.Remove(&pc);

   return retval;
}

bool ClientController::UpdateValueTree(int index, const void* data, size_t size)
{
   ValueTree tree = this->GetTree(index);
   bool retval = false;
   if (tree != ValueTree::invalid)
   {
      retval = ValueTreeSynchroniser::applyChange(tree, data, size, nullptr);
   }

   return retval;

}



ServerController::ServerController()
:  fTimerCount(0)
{
   fTree1.setProperty("count", 0, nullptr);
   ValueTree sub = ValueTree("sub");
   sub.setProperty("text", "THIS IS A STRING", nullptr);
   fTree1.addChild(sub, -1, nullptr);
   DBG(fTree1.toXmlString());

   fTree2.setProperty("count", 0, nullptr);

   // call our timer callback 1x/second
   this->startTimerHz(1);
}
 
ServerController::~ServerController()
{

}


void ServerController::VoidFn()
{
   DBG("Server: VoidFn()");
}

int ServerController::IntFn(int val)
{
   DBG("Server: IntFn()");
   return val * 2;
}


String ServerController::StringFn(const String& inString)
{
   DBG("Server: StringFn(" + inString + ")");
   Time now = Time::getCurrentTime();
   return now.toString(true, true);
}  


void ServerController::timerCallback()
{
   ++fTimerCount;
   if (0 == fTimerCount % 15)
   {
      var lastVal = fTree1.getProperty("count");
      int newVal = (int) lastVal + 1;

      ValueTree sub = fTree1.getChildWithName("sub");
      Random r;
      sub.setProperty("text", String("*** ") + String(r.nextInt()) + \
                      " ***", nullptr);

      fTree1.setProperty("count", newVal, nullptr);
      fTree1.setProperty("even", (0 == newVal % 2), nullptr);

      DBG(fTree1.toXmlString());
   }
  
   // Notify listeners that we've changed. 
   this->sendChangeMessage();
}


/**
 *  UNIT TEST CODE FOLLOWS
 */



class SyncTest : public UnitTest
{
public:
   SyncTest() : UnitTest("ValueTree sync Tests") {}

   void runTest() override
   {
      ValueTree tree1("source");

      ValueTree tree2;
   }
};

static SyncTest syncTest;