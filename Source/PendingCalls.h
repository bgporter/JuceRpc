/*
  ==============================================================================

    PendingCalls.h
    Created: 2 Feb 2016 4:28:43pm
    Author:  Brett Porter

  ==============================================================================
*/

#ifndef PENDINGCALLS_H_INCLUDED
#define PENDINGCALLS_H_INCLUDED



#include "../JuceLibraryCode/JuceHeader.h"

class PendingCall
{
public:
   PendingCall(uint32 sequence)
   : fSequence(sequence)
   {

   }

   /**
    * Tell our waitable event to block here, infinitely (default), or for a specified 
    * # of ms.
    * @param  milliseconds ms to wait for the call to complete; -1 == infinite wait.
    * @return              true if we were signalled, false if we timed out.
    */
   bool Wait(int milliseconds=-1)
   {
      return fEvent.wait(milliseconds);
   }

   void Signal() const { fEvent.signal(); }

   uint32 GetSequence() const { return fSequence; }

   void SetMemoryBlock(const MemoryBlock& mb) { fData = mb; }

   const MemoryBlock& GetMemoryBlock() const { return fData; }

   bool operator==(const PendingCall& rhs) const { return (rhs.fSequence == fSequence);};

private:

   /**
    * Sequence number of this function call.
    */
   uint32 fSequence;

   /**
    * An event for the returned message to signal.
    */
   WaitableEvent fEvent;

   /**
    * ...and a place to store the data returned from the server.
    */
   MemoryBlock fData;



public:
   /**
    * NOTE that the type and name of the linked list pointer are required
    * to be this by JUCE
    */
   LinkedListPointer<PendingCall> nextListItem;



};


class PendingCallList
{
public:
   PendingCallList();

   ~PendingCallList();

   /**
    * Append a new PendingCall object to the end of our list.
    * @param call [description]
    */
   void Append(PendingCall* call);

   /**
    * Look for a pending call with the specified sequence id.
    * @param  sequence id of the call we're looking for.
    * @return          nullptr if there's no such id.
    */
   PendingCall* FindCallBySequence(uint32 sequence);



   /**
    * Remove this object from the list, but do not delete it.
    * @param pending pointer to the object.
    */
   void Remove(PendingCall* pending, bool alsoDelete=false);


   int Size() const;


private:

   LinkedListPointer<PendingCall> fPendingCalls;
   
   /**
    * Function calls/returns will happen on separate threads. 
    */
   CriticalSection fMutex;
};



#endif  // PENDINGCALLS_H_INCLUDED
