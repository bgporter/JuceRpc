/*
  ==============================================================================

    PendingCalls.cpp
    Created: 2 Feb 2016 4:28:43pm
    Author:  Brett Porter

  ==============================================================================
*/

#include "PendingCalls.h"


PendingCallList::PendingCallList()
{

}

PendingCallList::~PendingCallList()
{

}

void PendingCallList::Append(PendingCall* call)
{
   const ScopedLock lock(fMutex);
   fPendingCalls.append(call);
}


PendingCall* PendingCallList::FindCallBySequence(uint32 sequence)
{
   const ScopedLock lock(fMutex);
   PendingCall* retval = nullptr;

   // yep, we iterate linearly. The assumption here is that we'll almost 
   // always only have a single active call at any given time. If that's NOT true, 
   // we're unlikely to have more than a handful, so this shouldn't be too expensive. 

   for (int i = 0; i < this->Size(); ++i)
   {
      PendingCall* pc = fPendingCalls[i].get();
      if (sequence == pc->GetSequence())
      {
         retval = pc;
         break;
      }
   }

   return retval;
}



void PendingCallList::Remove(PendingCall* pending, bool alsoDelete)
{
   const ScopedLock lock(fMutex);

   fPendingCalls.remove(pending);
   if (alsoDelete)
   {
      delete pending;
   }

}


int PendingCallList::Size() const
{
  return fPendingCalls.size();
}


/**
 * UNIT TESTS FOLLOW
 */


class PendingCallsTest : public UnitTest
{
public:
   PendingCallsTest() : UnitTest("PendingCalls tests") {}

   void runTest() override
   {
      this->beginTest("basic");

      PendingCallList pcl;

      PendingCall pc1(5);

      pcl.Append(&pc1);

      PendingCall* ppc = pcl.FindCallBySequence(5);
      this->expect(nullptr != ppc);
      this->expect(&pc1 == ppc);
      this->expect(nullptr == pcl.FindCallBySequence(55));

      pcl.Remove(&pc1);
      this->expect(nullptr == pcl.FindCallBySequence(5));

      this->expect(0 == pcl.Size());

      this->beginTest("multiple calls");

      PendingCall pc2(10);
      PendingCall pc3(11);

      pcl.Append(&pc2);
      pcl.Append(&pc3);

      this->expect(2 == pcl.Size());
      ppc = pcl.FindCallBySequence(11);
      this->expect(&pc3 == ppc);
      ppc = pcl.FindCallBySequence(10);
      this->expect(&pc2 == ppc);


   }

};

static PendingCallsTest tests;