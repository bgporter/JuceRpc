/*
  Copyright 2016 Art & Logic Software Development. 
 */


#ifndef h_RpcException
#define h_RpcException

#include "../JuceLibraryCode/JuceHeader.h"

class RpcException
{
public:
   RpcException(uint32 code) : fCode(code) {};
   // use default ctor, op=, copy ctor.
   // 
   
   uint32 GetCode() const {return fCode;};

   void AppendExtraData(const var& v) { fExtraData.add(v);};

   int GetExtraDataSize() const { return fExtraData.size(); };

   var GetExtraData(const int index) const
   {
      if (index < this->GetExtraDataSize())
      {
         return fExtraData.getReference(index);
      }
      else
      {
         return var();
      }
   }

private:
   uint32 fCode;
   // we can pass in additional data values as JUCE var objects if that's useful. 
   // They'll be added to the RPC message and can be unpacked at the other end.
   Array<var> fExtraData;

};




#endif