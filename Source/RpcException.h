/*
  Copyright 2016 Art & Logic Software Development. 
 */


#ifndef h_RpcException
#define h_RpcException

class RpcExceptionBase
{
public:
   RpcExceptionBase(uint32 code) : fCode(code) {};
   // use default ctor, op=, copy ctor.
   // 
   
   uint32 GetCode() const {return fCode;};

private:
   uint32 fCode;

};


template <uint32 Code>
class RpcException : public RpcExceptionBase
{
public:
   RpcException() : RpcExceptionBase(Code)
   {

   }

private:

};



#endif