/*
  ==============================================================================

    IpcMessage.cpp
    Created: 1 Feb 2016 11:06:54am
    Author:  Brett Porter

  ==============================================================================
*/

#include "IpcMessage.h"


namespace
{
   Atomic<uint32> sSequence{1};
}


template <>
void IpcMessage::SetTreeProperty<String>(const String& path, DataType type, String val)
{
   this->AppendString(path);
   this->AppendData<int>(type);
   this->AppendString(val);

}

class IpcValueTreeSync : public ValueTreeSynchroniser
{
public:
   IpcValueTreeSync(IpcMessage* msg, const ValueTree& tree)
   :  ValueTreeSynchroniser(tree)
   ,  fMessage(msg)
   {

   }

   void stateChanged(const void* encodedChange, size_t encodedChangeSize) override
   {
      fMessage->AppendData(encodedChange, encodedChangeSize);
   }

   void GetTreeData()
   {
      this->sendFullSyncCallback();
   }


private:
   IpcMessage* fMessage;

};




IpcMessage::IpcMessage(uint32 code, uint32 sequence)
:  fNextOffset(0)
{
   this->AppendData<uint32>(code); 
   this->AppendData<uint32>(IpcMessage::GetSequence(sequence));
}


IpcMessage::IpcMessage(const MemoryBlock& message)
{
   this->FromMemoryBlock(message);

}

IpcMessage::~IpcMessage()
{

}


void IpcMessage::FromMemoryBlock(const MemoryBlock& message)
{
   fData = message;
   fNextOffset = 0;
}


bool IpcMessage::GetMetadata(uint32& code, uint32& sequence)
{
   bool retval = false;
   fNextOffset = 0;
   if (fData.getSize() >= (2 * sizeof(uint32)))
   {
      code = this->GetData<uint32>();
      sequence = this->GetData<uint32>();
      retval = true;
   }
   return retval;
}


void IpcMessage::AppendData(const void* data, size_t numBytes)
{
   fData.append(data, numBytes);
}



void IpcMessage::AppendVar(var value)
{
   if (value.isInt())
   {
      this->AppendData<int>(kInt);
      this->AppendData<int>(int(value));
   }
   else if (value.isInt64())
   {
      this->AppendData<int>(kInt64);
      this->AppendData<int64>(int64(value));
   }
   else if (value.isBool())
   {
      this->AppendData<int>(kBool);
      this->AppendData<bool>(bool(value));
   }

   else if (value.isDouble())
   {
      this->AppendData<int>(kDouble);
      this->AppendData<double>(double(value));
   }
   else if (value.isString())
   {
      this->AppendData<int>(kString);
       String s = value;
      this->AppendString(s);
   }
   else 
   {
      // figure out why we're being passed a value we can't handle.
      jassert(false);
   }
}

void IpcMessage::AppendString(const String& s)
{
   const char* p = s.toRawUTF8();
   // getBytesRequiredFor() does *not* include the trailing NULL.
   size_t len = 1 + CharPointer_UTF8::getBytesRequiredFor(s.getCharPointer());
   this->AppendData(static_cast<void*>(const_cast<char*>(p)), len);

}


void IpcMessage::AppendValueTree(const ValueTree& tree)
{
   IpcValueTreeSync sync(this, tree);
   sync.GetTreeData();
}

/*
void IpcMessage::SetTreePropertyString(const String& path, const String& val)
{
   this->AppendString(path);
   this->AppendData<int>(IpcMessage::kString);
   this->AppendString(val);
}
*/


var IpcMessage::GetVar(size_t offset)
{
   DataType type = static_cast<DataType>(this->GetData<int>(offset));
   var retval;
   bool isValid = true;
   switch (type)
   {
      case kInt:
      {
         retval = this->GetData<int>();
      }
      break;

      case kInt64:
      {
         retval = this->GetData<int64>();
      }
      break;

      case kBool:
      {
         retval = this->GetData<bool>();    

      }
      break;
      case kDouble:
      {
         retval = this->GetData<double>();
      }
      break;

      case kString:
      {
         retval = this->GetString();
      }
      break;

      default:
      {
         isValid = false;
      }
   }

   return retval;
}

String IpcMessage::GetString(size_t offset)
{
   void* p = this->GetDataPointer(offset);
   const char* s = static_cast<const char*>(p);
   size_t rawLen = 1 + strlen(s);
   fNextOffset += rawLen;
   return String(CharPointer_UTF8(s));

}


// void IpcMessage::GetValueTree(ValueTree& target, size_t offset)
String IpcMessage::GetValueTree(ValueTree& target, size_t offset)
{ 
   void* p = this->GetDataPointer(offset);
   size_t len = fData.getSize() - fNextOffset;

   String delta = String::toHexString(p, len);
   DBG(delta);

   ValueTreeSynchroniser::applyChange(target, p, len, nullptr);
   return delta;

}


void IpcMessage::ResetData()
{
   uint32 code;
   uint32 sequence;

   this->GetMetadata(code, sequence);
   fData.reset();

   fNextOffset = 0;

   if (sequence > 0)
   {
       sequence = IpcMessage::GetSequence(kUseNextSequence);
   }
   this->AppendData<uint32>(code);
   this->AppendData<uint32>(sequence);


}

void IpcMessage::ApplyTreeProperty(ValueTree root)
{
   String path = this->GetString();

   DBG("PATH = " + path);


   // !!! unpack path & get the target tree
   ValueTree target = root;
   String propertyName = "";

   while (true)
   {
      int slashPosition = path.indexOfChar('/');
      if (-1 == slashPosition)
      {  
         // if there are no more slashes, the remaining part of 
         // `path` is the property name.
         propertyName = path;
         break;
      }
      else
      {
         String treeName = path.substring(0, slashPosition);
         path = path.substring(slashPosition + 1);

         target = target.getOrCreateChildWithName(treeName, nullptr);
      }
   }
   var newValue = this->GetVar();

   bool isValid = (!newValue.isVoid());

   if (isValid)
   {
      // actually set the property!
      target.setProperty(propertyName, newValue, nullptr);
   }
   else
   {
      // figure out what's wrong...
      jassert(false);
   }
}

uint32 IpcMessage::GetSequence(uint32 sequence)
{
   if (kUseNextSequence == sequence)
   {
       
      sequence = ++sSequence;
      // sequence #0 is reserved for unsolicited messages coming from the server.
      if (0 == sequence)
      {
         sequence = ++sSequence;
      }

   }
   return sequence;   
}

/**
 *  UNIT TEST CODE FOLLOWS
 */



class IpcMessageTest : public UnitTest
{
public:
   IpcMessageTest() : UnitTest("IpcMessage Tests") {}

   void runTest() override
   {

      uint32 code;
      uint32 sequence;
      bool result;

      this->beginTest("Basic operation");
      IpcMessage m1(1);
      result = m1.GetMetadata(code, sequence);
      this->expect(result);
      this->expect(1 == code);
      this->expect(2 == sequence);
      float val = 21.1;
      m1.AppendData(val);

      float retval = m1.GetData<float>();
      this->expect(retval == val);      


      this->beginTest("Serializing data");

      IpcMessage m2(77);
      m2.GetMetadata(code, sequence);
      this->expect(3 == sequence);
      String s("malarkey");
      m2.AppendString(s);
      String s2("baloney");
      m2.AppendString(s2);
      m2.AppendData(44.1f);
      m2.AppendData(-1);

      m2.GetMetadata(code, sequence);

      this->expect(77 == code);
      this->expect(s == m2.GetString());
      this->expect(s2 == m2.GetString());
      this->expect(44.1f == m2.GetData<float>());
      this->expect(-1 == m2.GetData<int>());

      this->beginTest("Init from MemoryBlock");


      IpcMessage m3(m2.GetMemoryBlock());
      m3.GetMetadata(code, sequence);
      this->expect(77 == code);
      this->expect(s == m3.GetString());
      this->expect(s2 == m3.GetString());
      this->expect(44.1f == m3.GetData<float>());
      this->expect(-1 == m3.GetData<int>()); 


      IpcMessage m4;
      m4.FromMemoryBlock(m2.GetMemoryBlock());
      m4.GetMetadata(code, sequence);
      this->expect(77 == code);
      this->expect(s == m4.GetString());
      this->expect(s2 == m4.GetString());
      this->expect(44.1f == m4.GetData<float>());
      this->expect(-1 == m4.GetData<int>()); 


      this->beginTest("ValueTree send");
      IpcMessage m5;
      ValueTree tree("test1");
      tree.setProperty("foo", 1, nullptr);
      tree.setProperty("bar", "a string", nullptr);

      m5.AppendValueTree(tree);
      m5.GetMetadata(code, sequence);

      ValueTree tree2;
      m5.GetValueTree(tree2);

      this->expect(tree.isEquivalentTo(tree2));

      var v = tree2.getProperty("foo");
      this->expect(1 == (int) v);
      this->expect(tree2.getProperty("bar") == String("a string"));

      this->beginTest("ValueTree change");
      ValueTree target;

      IpcMessage m6(1000, 0);
      IpcValueTreeSync sync1(&m6, tree);
      sync1.GetTreeData();
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      this->expect(tree.isEquivalentTo(target));



      m6.ResetData();
      tree.setProperty("foo", 200, nullptr);

      // m6 should now contain the raw data containing just the change in the tree
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      DBG("TREE:");
      DBG(tree.toXmlString());

      DBG("\n\nTarget:");
      DBG(target.toXmlString());
      this->expect(tree.isEquivalentTo(target));

      this->beginTest("ValueTree + subtrees");
      m6.ResetData();

      ValueTree sub = tree.getOrCreateChildWithName("subtree", nullptr);
      // m6 should now contain the raw data containing just the change in the tree
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      DBG("TREE:");
      DBG(tree.toXmlString());

      DBG("\n\nTarget:");
      DBG(target.toXmlString());
      this->expect(tree.isEquivalentTo(target));      

      m6.ResetData();
      sub.setProperty("temperature", 98.6f, nullptr);
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      DBG("TREE:");
      DBG(tree.toXmlString());

      DBG("\n\nTarget:");
      DBG(target.toXmlString());
      this->expect(tree.isEquivalentTo(target));     

      m6.ResetData();
      ValueTree sub2 = sub.getOrCreateChildWithName("basement", nullptr);
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);


      m6.ResetData();
      sub2.setProperty("grandchild", "a value goes here", nullptr) ;
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      DBG("TREE:");
      DBG(tree.toXmlString());

      DBG("\n\nTarget:");
      DBG(target.toXmlString());
      this->expect(tree.isEquivalentTo(target));      
      
       
      /*
      ValueTree tree2("test 2");
      tree.setProperty("rootInt", 100, nullptr);
      sub1 = tree.setProperty
      */



   }

private:


};


class IpcMessageTest2 : public UnitTest
{
public:
   IpcMessageTest2() : UnitTest("more IpcMessage Tests") {}


   void CheckTrees(ValueTree& src, ValueTree& dest)
   {
      this->expect(src.isEquivalentTo(dest));
      DBG("SRC:");
      DBG(src.toXmlString());
      DBG("DEST:");
      DBG(dest.toXmlString());
   }

   void runTest() override
   {
      this->beginTest("VT sync debug");

      ValueTree tree1("one");
      ValueTree target;

      tree1.setProperty("count", 0, nullptr);

      ValueTree sub = ValueTree("sub");
      sub.setProperty("text", "THIS IS A STRING", nullptr);
      tree1.addChild(sub, -1, nullptr);
      DBG(tree1.toXmlString());      

      IpcMessage m6(1000, 0);
      IpcValueTreeSync sync1(&m6, tree1);
      sync1.GetTreeData();
      uint32 code;
      uint32 sequence;
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      this->CheckTrees(tree1, target);

      m6.ResetData();
      tree1.setProperty("count", 1, nullptr);
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      this->CheckTrees(tree1, target);

      m6.ResetData();
      ValueTree sub2 = tree1.getChildWithName("sub");
      sub2.setProperty("text", "this is a different string", nullptr);
      m6.GetMetadata(code, sequence);
      m6.GetValueTree(target);
      this->CheckTrees(tree1, target);





   }
};




class DoubleValueTreeSync : public ValueTreeSynchroniser
{
public:
   DoubleValueTreeSync(IpcMessage* msg, const ValueTree& src, ValueTree& target)
   :  ValueTreeSynchroniser(src)
   ,  fMessage(msg)
   ,  fTarget(target)
   {

   }

   void stateChanged(const void* encodedChange, size_t encodedChangeSize) override
   {
      fMessage->AppendData(encodedChange, encodedChangeSize);
      uint32 code;
      uint32 seq;
      fMessage->GetMetadata(code, seq);
      fMessage->GetValueTree(fTarget);
      fMessage->ResetData();

   }

   void GetTreeData()
   {
      this->sendFullSyncCallback();
   }


private:
   IpcMessage* fMessage;
   ValueTree&  fTarget;

};


class IpcMessageTest3 : public UnitTest
{
public:
   IpcMessageTest3() : UnitTest("IpcMessage SetTreePropertyTests") {}


   void CheckTrees(ValueTree& src, ValueTree& dest)
   {
      DBG("SRC:");
      DBG(src.toXmlString());
      DBG("DEST:");
      DBG(dest.toXmlString());
      this->expect(src.isEquivalentTo(dest));      
   }

   void runTest() override
   {
      ValueTree serverTree("root");
      ValueTree clientTree("root");

      this->beginTest("Basic nested set property test");
      IpcMessage m1(1000);
      IpcMessage m2;

      m1.SetTreeProperty("intVal", IpcMessage::kInt, 1000);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      uint32 code, seq;
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      //m1.SetTreePropertyString("sub1/stringVal", "This is a test");
      m1.SetTreeProperty("sub1/stringVal", IpcMessage::kString, String("This is a test"));
      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      m1.SetTreeProperty("sub1/floatVal", IpcMessage::kDouble, 3.14159);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      m1.SetTreeProperty("sub1/sub2/boolVal", IpcMessage::kBool, true);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      this->beginTest("Setting existing child tree/properties");
       
      m1.ResetData();
      m2.ResetData();
      m1.SetTreeProperty("intVal", IpcMessage::kInt, -4);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      m1.SetTreeProperty("sub1/stringVal", IpcMessage::kString, String("This is a subsequent test"));
      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      m1.SetTreeProperty("sub1/floatVal", IpcMessage::kDouble, 101.0);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      m1.ResetData();
      m2.ResetData();

      m1.SetTreeProperty("sub1/sub2/boolVal", IpcMessage::kBool, false);

      m2.FromMemoryBlock(m1.GetMemoryBlock());
      m2.GetMetadata(code, seq);

      DBG("BEFORE:");
      DBG(serverTree.toXmlString());

      m2.ApplyTreeProperty(serverTree);
      DBG("AFTER:");
      DBG(serverTree.toXmlString());

      this->beginTest("Verifying sync");
      ValueTree server2("root");
      ValueTree client2;

      IpcMessage m3(2000);
      DoubleValueTreeSync sync(&m3, server2, client2);

      sync.GetTreeData();

      this->CheckTrees(server2, client2);

      IpcMessage m4(2000);
      m4.SetTreeProperty("sub/txt", IpcMessage::kString, String("This is some text"));
      m4.GetMetadata(code, seq);
      // apply the set property against the server tree. This should 
      // cause the m3 sync message to contain the deltas 
      m4.ApplyTreeProperty(server2);
      
      this->CheckTrees(server2, client2);





   }

};


class IpcMessageTest4 : public UnitTest
{
public:
   IpcMessageTest4() : UnitTest("IpcMessage get/set JUCE vars.") {}



   void runTest() override
   {
      this->beginTest("setting/getting vars...");

      IpcMessage m1(1);
      int intVal = 201;
      int64 int64Val = -202020;
      bool boolVal = false;
      double doubleVal = 231.010;
      String stringVal = "This is a string value!";

      m1.AppendVar(var(intVal));
      IpcMessage resp;

      resp.FromMemoryBlock(m1.GetMemoryBlock());
      uint32 code;
      uint32 sequence;
      resp.GetMetadata(code, sequence);
      var value = resp.GetVar();
      this->expect(value == var(intVal));

      m1.ResetData();
      m1.AppendVar(var(stringVal));

      resp.FromMemoryBlock(m1.GetMemoryBlock());
      resp.GetMetadata(code, sequence);
      value = resp.GetVar();
      this->expect(value == var(stringVal));
   }
};

static IpcMessageTest tests;
static IpcMessageTest2 test2;
static IpcMessageTest3 test3;
static IpcMessageTest4 test4;


