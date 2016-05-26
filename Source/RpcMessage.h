/*
  ==============================================================================

    RpcMessage.h
    Created: 1 Feb 2016 11:06:54am
    Author:  Brett Porter

  ==============================================================================
*/

#ifndef IPCMESSAGE_H_INCLUDED
#define IPCMESSAGE_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"


/**
 * @class RpcMessage
 *
 * Class to hold function call data and useful metadata:
 * - an integer code indicating the function call associated with this message.
 * - a sequence identifier that should help us remain thread-safe and match asynch 
 *    function return data with the same function call that's waiting for a response.
 *
 * The issue: The JUCE IP Classes are async w/r/t sending/receiving messages. Our design will 
 * be that the client serializes call parameters into one of these messages and we 
 * send the message to the server. The client then blocks on a waitable event. 
 *
 * Eventually the server sends us back a message with the function results, and we look up the 
 * pending transaction, signaling the waiting thread that it has data, which it can unpack
 * and return as if this had been a synchronous in-process function call.
 */

class RpcMessage
{
public:
   enum OffsetType
   {
      kUseNextOffset = 0xFFFFFFFF,
      kUseNextSequence = 0xFFFFFFFF
   };


   /**
    * We support setting value tree properties for most of the data types that the
    * JUCE `var` type can hold.
    */
   enum DataType
   {
      kInt = 0,
      kInt64,
      kBool,
      kDouble,
      kString
   };

   /**
    * Create an empty RpcMessage object.
    */
   RpcMessage(uint32 code=0, uint32 sequence=kUseNextSequence);

   /**
    * Initialize with an existing MemoryBlock object, which we must be in our
    * messagecode + data format. Used when parsing received messages.
    */
   RpcMessage(const MemoryBlock& message);

   ~RpcMessage();


   const MemoryBlock& GetMemoryBlock() const
   {
      return fData;
   }

   /**
    * Replace our existing contents with the provided memory block.
    * @param message [description]
    */
   void FromMemoryBlock(const MemoryBlock& message);


   /**
    * Retrieve the message's function code and sequence number, leaving the offset
    * index set to get the first real piece of data in the message.
    * @param code     Message function code
    * @param sequence 
    * @return true if the code/sequence are valid.
    */
   bool GetMetadata(uint32& code, uint32& sequence);


   /**
    * Add some data bytes to the end of our block.
    * @param data     Pointer to the new data
    * @param numBytes Length of data to be added.
    */
   void AppendData(const void* data, size_t numBytes);
   
   template <typename T>
   void AppendData(T val)
   {
      this->AppendData(&val, sizeof(T));
   }


   /**
    * Append a JUCE var object to our data. Note that we only support a subset of 
    * the allowable var data types, since some of them don't make sense across 
    * process boundaries. 
    * @param val the `var` object to append. 
    */
   void AppendVar(var val);
    
   void AppendString(const String& s);


   /**
    * Add a ValueTree object to this message. NOTE that for things to work, we
    * require that the ValueTree data be the last thing in the message.
    * @param tree ValueTree object to send.
    */
   void AppendValueTree(const ValueTree& tree);

   /**
    * Set a property value in a ValueTree on the server. Since ValueTrees can 
    * be recursive, the `path` variable may contain forward slashes to indicate 
    * the path through a top-level tree down to the node that we want to actually 
    * set the value of (so the last component in this list will be the property name, 
    * any preceding components will be the names of child trees)
    *
    * This method is only used on the client side. On the client side, use 
    * the ApplyTreeProperty method instead.
    *
    * @param path The slash-separated path in the target tree (which must be 
    *             specified elsewhere)
    *
    * @param type One of the DataType enum values (above). You need to manually 
    *             verify that this type indicator matches the actual template type 
    *             being passed into this function. TODO: !!! find a more elegant 
    *             method for this that doesn't require manual 
    *
    * @param val  The actual value to set on this property
    */
   template <typename T>
   void SetTreeProperty(const String& path, DataType type, T val)
   {

      jassert(RpcMessage::kString != type);
      this->AppendString(path);
      this->AppendData<int>(type);
      this->AppendData<T>(val);
   }


   /**
    * Return a pointer to data inside the message object. 
    * @param offset Index at which to look for the data.
    */
   void* GetDataPointer(size_t offset=kUseNextOffset) const
   {
      if (kUseNextOffset == offset)
      {
         offset = fNextOffset;
      }

      char* dataStart = offset + static_cast<char*>(fData.getData());
      return static_cast<void*>(dataStart);   
   }

   /**
    * Return a primitive POD object from the message object, by default at the next
    * offset.
    */
   template <typename T>
   T GetData(size_t offset=kUseNextOffset)
   {
      void* p = this->GetDataPointer(offset);
      fNextOffset += sizeof(T);
      return * (static_cast<T*>(p));
   }


    var GetVar(size_t offset = kUseNextOffset);

    String GetString(size_t offset=kUseNextOffset);

    // void GetValueTree(ValueTree& target, size_t offset=kUseNextOffset);
    String GetValueTree(ValueTree& target, size_t offset=kUseNextOffset);


    /**
     * Unpack a message that is setting a property in the tree (with optional path 
     * to sub-trees) and apply that change to the tree. If the subtrees specified in the 
     * path don't exist, they'll be created. 
     * @param root Root of the ValueTree to update.
     */
    void ApplyTreeProperty(ValueTree root);


    /**
     * Clear all data after the code and sequence number. Mostly useful for 
     * unit testing.
     */
    void ResetData();

private:
    static uint32 GetSequence(uint32 sequence);

private:

   MemoryBlock fData;
   size_t      fNextOffset;

};



/*
template <>
void RpcMessage::SetTreeProperty<String>(const String& path, DataType type, String val)
{

}
*/

#endif  // IPCMESSAGE_H_INCLUDED
