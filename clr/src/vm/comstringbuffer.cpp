// ==++==
// 
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
// 
// ==--==
/*============================================================
**
** Class:  COMStringBuffer
**
**                                        
**
** Purpose: The implementation of the StringBuffer class.
**
** Date:  March 9, 1998
** 
===========================================================*/
#include "common.h"

#include "object.h"
#include "excep.h"
#include "frames.h"
#include "vars.hpp"
#include "comstring.h"
#include "comstringcommon.h"
#include "comstringbuffer.h"

#define DEFAULT_CAPACITY 16
#define DEFAULT_MAX_CAPACITY 0x7FFFFFFF

//
//Static Class Variables
//
MethodTable* COMStringBuffer::s_pStringBufferClass;

FCIMPL0(void*, COMStringBuffer::GetCurrentThread)
    return ::GetThread();
FCIMPLEND

/*==============================CalculateCapacity===============================
**Calculates the new capacity of our buffer.  If the size of the buffer is 
**less than a fixed number (10000 in this case), we just double the buffer until
**we have enough space.  Once we get larger than 10000, we use a series of heuristics
**to determine the most appropriate size.
**
**Args:  currentCapacity:  The current capacity of the buffer
**       requestedCapacity: The minimum required capacity of the buffer
**Returns: The new capacity of the buffer.
**Exceptions: None.
==============================================================================*/
INT32 COMStringBuffer::CalculateCapacity (STRINGBUFFERREF thisRef, INT32 currentCapacity, INT32 requestedCapacity) {
    THROWSCOMPLUSEXCEPTION();

    INT32 newCapacity=currentCapacity;
    INT32 maxCapacity=thisRef->GetMaxCapacity();
    
    //This unfortunate situation can occur if they manually set the capacity to 0.
    if (newCapacity<=0) {
        newCapacity=DEFAULT_CAPACITY; 
    }

    if (requestedCapacity>maxCapacity) {
        COMPlusThrowArgumentOutOfRange(L"capacity", L"ArgumentOutOfRange_Capacity");
    }

    //Double until we find something bigger than what we need.
    while (newCapacity<requestedCapacity && newCapacity>0) {
        newCapacity*=2;
    }
    //Check if we overflowed.
    if (newCapacity<=0) {
        COMPlusThrowArgumentOutOfRange(L"capacity", L"ArgumentOutOfRange_NegativeCapacity");
    }
    
    //Also handle the unlikely case where we double so much that we're larger
    //than maxInt.
    if (newCapacity<=maxCapacity && newCapacity>0) {
        return newCapacity;
    }
    
    return maxCapacity;
}


/*==================================CopyString==================================
**Action: Creates a new copy of the string and then clears the dirty bit.
**        The Allocated String has a capacity of exactly newCapacity (we assume that
**        the checks for maxCapacity have been done elsewhere.)  If newCapacity is smaller
**        than the current size of the String, we truncate the String.
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
STRINGREF COMStringBuffer::CopyString(STRINGBUFFERREF *thisRef, STRINGREF CurrString, int newCapacity) {
  int CurrLen;
  int copyLength;
  STRINGREF Local;

  _ASSERTE(newCapacity>=0);
  _ASSERTE(newCapacity<=(*thisRef)->GetMaxCapacity());
  
  //Get the data out of our current String.
  CurrLen = CurrString->GetStringLength();

  //Calculate how many characters to copy.  If we have enough capacity to 
  //accomodate all of our current String, we'll take the entire thing, otherwise
  //we'll just take the most that we can fit.
   if (newCapacity>=CurrLen) {
       copyLength = CurrLen;
   } else {
       _ASSERTE(!"Copying less than the full String.  Was this intentional?");
       copyLength = newCapacity;
   }

   //CurrString needs to be protected because it is used in NewString only *after*
   //we allocate a new string.
  GCPROTECT_BEGIN(CurrString);
  Local = COMString::NewString(&CurrString, 0, copyLength, newCapacity);
  GCPROTECT_END(); //CurrString

  return Local;
}

STRINGREF COMStringBuffer::GetRequiredString(STRINGBUFFERREF *thisRef, STRINGREF thisString, int requiredCapacity) {
    INT32 currCapacity = thisString->GetArrayLength()-1;
    if ((currCapacity>=requiredCapacity)) {
        return thisString;
    }
    return CopyString(thisRef, thisString, CalculateCapacity((*thisRef), currCapacity, requiredCapacity));
}

/*================================ReplaceBuffer=================================
**This is a helper function designed to be used by N/Direct it replaces the entire
**contents of the String with a new string created by some native method.  This 
**will not be exposed through the StringBuilder class.
==============================================================================*/
void COMStringBuffer::ReplaceBuffer(STRINGBUFFERREF *thisRef, WCHAR *newBuffer, INT32 newLength) {
    STRINGREF thisString = NULL;
    WCHAR *thisChars;

    _ASSERTE(thisRef);
    _ASSERTE(*thisRef);
    _ASSERTE(newBuffer);
    _ASSERTE(newLength>=0);

    THROWSCOMPLUSEXCEPTION();

    void *tid;
    thisString = GetThreadSafeString(*thisRef,&tid);

    //This will ensure that we have enough space and take care of the CopyOnWrite
    //if needed.
    thisString = GetRequiredString(thisRef, thisString, newLength);
    thisChars = thisString->GetBuffer();

    //memcpy should blithely ignore any nulls which it finds in newBuffer.
    memcpyNoGCRefs(thisChars, newBuffer, newLength*sizeof(WCHAR));
    thisChars[newLength]='\0';
    thisString->SetStringLength(newLength);
	_ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));
    ReplaceStringRef(*thisRef, tid, thisString);
}


/*================================ReplaceBufferAnsi=================================
**This is a helper function designed to be used by N/Direct it replaces the entire
**contents of the String with a new string created by some native method.  This 
**will not be exposed through the StringBuilder class.
**
**This version does Ansi->Unicode conversion along the way. Although
**making it a member of COMStringBuffer exposes more stringbuffer internals
**than necessary, it does avoid requiring a temporary buffer to hold
**the Ansi->Unicode conversion.
==============================================================================*/
void COMStringBuffer::ReplaceBufferAnsi(STRINGBUFFERREF *thisRef, CHAR *newBuffer, INT32 newCapacity) {
    STRINGREF thisString;
    WCHAR *thisChars;

    _ASSERTE(thisRef);
    _ASSERTE(*thisRef);
    _ASSERTE(newBuffer);
    _ASSERTE(newCapacity>=0);

    THROWSCOMPLUSEXCEPTION();

    void *tid;
    thisString = GetThreadSafeString(*thisRef,&tid);

    //This will ensure that we have enough space and take care of the CopyOnWrite
    //if needed.
    thisString = GetRequiredString(thisRef, thisString, newCapacity);
    thisChars = thisString->GetBuffer();


    // NOTE: This call to MultiByte also writes out the null terminator
    // which is currently part of the String representation.
    INT32 ncWritten = MultiByteToWideChar(CP_ACP,
                                          MB_PRECOMPOSED,
                                          newBuffer,
                                          -1,
                                          thisChars,
                                          newCapacity+1);

    if (ncWritten == 0)
    {
        // Normally, we'd throw an exception if the string couldn't be converted.
        // In this particular case, we paper over it instead. The reason is
        // that most likely reason a P/Invoke-called api returned a
        // poison string is that the api failed for some reason, and hence
        // exercised its right to leave the buffer in a poison state.
        // Because P/Invoke cannot discover if an api failed, it cannot
        // know to ignore the buffer on the out marshaling path.
        // Because normal P/Invoke procedure is for the caller to check error
        // codes manually, we don't want to throw an exception on him.
        // We certainly don't want to randomly throw or not throw based on the
        // nondeterministic contents of a buffer passed to a failing api.
        *thisChars = L'\0';
        ncWritten++;
    }
    thisString->SetStringLength(ncWritten - 1);
	_ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));
    ReplaceStringRef(*thisRef, tid, thisString);
}



//
//
// Private Quasi-Constructors.
//
//
//

/*================================MakeFromString================================
**We can't have native constructors, so the managed constructor calls
**this method.  If args->value is null, we simply create an empty string with
**a default length.  If it does contain data, we allocate space for twice this
**amount of data, copy the data, associate it with the StringBuffer and clear
**the CopyOnWrite bit.
**
**Returns void.  It's sideeffects are visible through the changes made to thisRef
**
**Args: typedef struct {STRINGBUFFERREF thisRef; INT32 capacity; INT32 length; INT32 startIndex; STRINGREF value;} _makeFromStringArgs;
==============================================================================*/
FCIMPL5(void, COMStringBuffer::MakeFromString, StringBufferObject* thisRefUNSAFE, StringObject* valueUNSAFE, INT32 startIndex, INT32 lengthIn, INT32 capacityIn)
{
  INT32 capacity;
  INT32 newCapacity;
  INT32 length;

  struct _gc
  {
    STRINGBUFFERREF thisRef;
    STRINGREF       Local;
    STRINGREF       EmptyString;
    STRINGREF       value;
  } gc;

  
  gc.thisRef        = (STRINGBUFFERREF)ObjectToOBJECTREF(thisRefUNSAFE);
  gc.value          = ObjectToSTRINGREF(valueUNSAFE);
  gc.Local          = NULL;
  gc.EmptyString    = NULL;

  THROWSCOMPLUSEXCEPTION();
  HELPER_METHOD_FRAME_BEGIN_0();
  GCPROTECT_BEGIN(gc);

  _ASSERTE(gc.thisRef);

  //Figure out the capacity requested.
  capacity = ((capacityIn<=0))?DEFAULT_CAPACITY:capacityIn;
  gc.thisRef->SetMaxCapacity(DEFAULT_MAX_CAPACITY);

  //Handle a null string by creating a default string and setting the length to 0
  if (!gc.value) 
  {
      //NullString needs to be protected because it is used in NewString only *after*
      //we allocate a new string.
      gc.EmptyString    = COMString::GetEmptyString();
      gc.Local          = COMString::NewString(&gc.EmptyString, 0, gc.EmptyString->GetStringLength(), capacity);
  } 
  else 
  {
      //Figure out the actual length.  This is valueLength if lengthIn is -1 (which indicates that we're using
      //the entire string) or lengthIn, which indicates that we're using part of the string.  Check to ensure
      //that the part which we're using isn't larger than the actual String.
      int valueLength = gc.value->GetStringLength(); 
      length = ((-1==lengthIn) ? valueLength : lengthIn);

      if ((startIndex<0)) 
      {
          COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
      }

      if (startIndex>valueLength-length) 
      {
          COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_IndexLength");
      }

      newCapacity = CalculateCapacity(gc.thisRef, capacity, length);
      
      gc.Local = COMString::NewString(&gc.value, startIndex, length, newCapacity);
  }
  
  //Set the StringRef and clear the CopyOnWrite bit.
  gc.thisRef->SetStringRef(gc.Local);
 
  GCPROTECT_END();
  HELPER_METHOD_FRAME_END();


}
FCIMPLEND

// GetThreadSafeString
STRINGREF COMStringBuffer::GetThreadSafeString(STRINGBUFFERREF thisRef,void** currentThread) {
    STRINGREF thisString = thisRef->GetStringRef();
    *currentThread = ::GetThread();
    if (thisRef->GetCurrentThread() != *currentThread) {
        INT32 currCapacity = thisString->GetArrayLength()-1;
        thisString = CopyString(&thisRef, thisString, currCapacity);
    }
    return thisString;
}

//
// BUFFER STATE QUERIES AND MODIFIERS
//


/*=================================SetCapacity==================================
**Action: Sets the capacity to be args->capacity.  If capacity is < the current length,
**        throws an ArgumentException;
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(Object*, COMStringBuffer::SetCapacity, StringBufferObject* thisRefUNSAFE, INT32 capacity)
{
    STRINGBUFFERREF thisRef = (STRINGBUFFERREF) thisRefUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, thisRef);

    THROWSCOMPLUSEXCEPTION();

    if (thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    }

    void *tid;
    STRINGREF thisString = GetThreadSafeString(thisRef,&tid);

    //Verify that our capacity is greater than 0 and that we can fit our
    //current string into the new capacity.
    if (capacity<0) {
        COMPlusThrowArgumentOutOfRange(L"capacity", L"ArgumentOutOfRange_NegativeCapacity");
    }

    if (capacity< (INT)thisString->GetStringLength()) {
        COMPlusThrowArgumentOutOfRange(L"capacity", L"ArgumentOutOfRange_SmallCapacity");
    }

    if (capacity>thisRef->GetMaxCapacity()) {
        COMPlusThrowArgumentOutOfRange(L"capacity", L"ArgumentOutOfRange_Capacity");
    }

    INT32 currCapacity = thisString->GetArrayLength()-1;
    //If we already have the correct capacity, bail out early.
    if (capacity!=currCapacity) {
    
       //Allocate a new String with the capacity and copy all of our old characters
       //into it.  We've already guaranteed that our String will fit within that capacity.
      //We don't need to worry about the COW bit because we're always allocating a new String.
      STRINGREF newString = CopyString(&thisRef,thisString, capacity);
      ReplaceStringRef(thisRef, tid, newString);
    }
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(thisRef);
}
FCIMPLEND


/*==============================InsertStringBuffer==============================
**Insert args->value into this buffer at location args->index.  Move all characters
**after args->index so that nothing gets overwritten.
**
**Returns a pointer to the current StringBuffer.
**
**Args:typedef struct {STRINGBUFFERREF thisRef; INT32 count; int index; STRINGREF value;} _insertStringBufferArgs;
==============================================================================*/
FCIMPL4(Object*, COMStringBuffer::InsertString, StringBufferObject* thisRefUNSAFE, INT32 index, StringObject* valueUNSAFE, INT32 count)
{
    STRINGBUFFERREF thisRef = (STRINGBUFFERREF) thisRefUNSAFE;
    STRINGREF       value   = (STRINGREF)       valueUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, thisRef, value);
    
  WCHAR *thisChars, *valueChars;
  STRINGREF thisString = NULL;
  int thisLength, valueLength;
  int length;
  
  THROWSCOMPLUSEXCEPTION();

    if (thisRef==NULL) 
    {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }
  
  //If value isn't null, get all of our required values.
    if (!value) 
    {
        if (index == 0 && count==0) 
        {
            goto lExit;
	}
	COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
  }

  void *tid;
  thisString = GetThreadSafeString(thisRef,&tid);
  thisLength = thisString->GetStringLength();
    valueLength = value->GetStringLength();

  //Range check the index.
    if (index<0 || index > (INT32)thisLength) {
      COMPlusThrowArgumentOutOfRange(L"index", L"ArgumentOutOfRange_Index");
  }

    if (count<1) {
      COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_GenericPositive");
  }

  //Calculate the new length, ensure that we have the space and set the space variable for this buffer
  length = thisLength + (valueLength*count);
  thisString = GetRequiredString(&thisRef, thisString, length);
  
  //Get another pointer to the buffer in case it changed during LocalEnsure
  //Assumes that thisRef points to a StringBuffer.
  thisChars = thisString->GetBuffer();
    valueChars = value->GetBuffer();
  thisString->SetStringLength(length);

  //Copy the old characters over to make room and then insert the new characters.
    memmove(&(thisChars[index+(valueLength*count)]),&(thisChars[index]),(thisLength-index)*sizeof(WCHAR));
    for (int i=0; i<count; i++) {
        memcpyNoGCRefs(&(thisChars[index+(i*valueLength)]),valueChars,valueLength*sizeof(WCHAR));
  }
  thisChars[length]='\0';
  _ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));

  ReplaceStringRef(thisRef, tid, thisString);
    
lExit: ;
    HELPER_METHOD_FRAME_END();
  //Force the info into an LPVOID to return.
    return OBJECTREFToObject(thisRef);
}
FCIMPLEND



/*===============================InsertCharArray================================
**Action: Inserts the characters from value into this at position index.  The characters
**        are taken from value starting at position startIndex and running for count
**        characters.
**Returns: A reference to this with the new characters inserted.
**Arguments:
**Exceptions: ArgumentException if index is outside of the range of this.
**            ArgumentException if count<0, startIndex<0 or startIndex+count>value.length
==============================================================================*/
FCIMPL5(Object*, COMStringBuffer::InsertCharArray, StringBufferObject* thisRefUNSAFE, INT32 index, CHARArray* valueUNSAFE, INT32 startIndex, INT32 charCount)
{
    STRINGBUFFERREF thisRef = (STRINGBUFFERREF) thisRefUNSAFE;
    CHARARRAYREF    value   = (CHARARRAYREF)    valueUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_2(Frame::FRAME_ATTR_RETURNOBJ, thisRef, value);

  WCHAR *thisChars, *valueChars;
  STRINGREF thisString = NULL;
  int thisLength, valueLength;
  int length;
  
  THROWSCOMPLUSEXCEPTION();

    if (thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  void *tid;
  thisString = GetThreadSafeString(thisRef,&tid);
  thisLength = thisString->GetStringLength();

  //Range check the index.
    if (index<0 || index > (INT32)thisLength) {
      COMPlusThrowArgumentOutOfRange(L"index", L"ArgumentOutOfRange_Index");
  }
  
  //If they passed in a null char array, just jump out quickly.
    if (!value) {
        if (startIndex == 0 && charCount==0) 
        {
            goto lExit;
	}
	COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");
  }


  //Range check the array.
    if (startIndex<0) {
      COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
  }

    if (charCount<0) {
      COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_GenericPositive");
  }

    if (startIndex > ((INT32)value->GetNumComponents()-charCount)) {
      COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
  }
  
  length = thisLength + charCount;
  thisString = GetRequiredString(&thisRef, thisString, length);  

  thisChars = thisString->GetBuffer();
    valueChars = (WCHAR *)(value->GetDataPtr());
    valueLength = charCount;

  //Copy the old characters over to make room and then insert the new characters.
    memmove(&(thisChars[index+valueLength]),&(thisChars[index]),(thisLength-index)*sizeof(WCHAR));
    memcpyNoGCRefs(&(thisChars[index]), &(valueChars[startIndex]), valueLength*sizeof(WCHAR));
    thisChars[length]='\0';
  thisString->SetStringLength(length);  
  _ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));
  
  ReplaceStringRef(thisRef, tid, thisString);

lExit: ;
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(thisRef);
}
FCIMPLEND

/*=============================REMOVEBUFFER================================
**Removes all of the args->length characters starting at args->startIndex.
**
**Returns a pointer to the current buffer.
**
**Args:typedef struct {STRINGBUFFERREF thisRef; int length; int startIndex;} _removeBufferArgs;
=========================================================================*/
FCIMPL3(Object*, COMStringBuffer::Remove, StringBufferObject* thisRefUNSAFE, INT32 startIndex, INT32 length)
{
    STRINGBUFFERREF thisRef = (STRINGBUFFERREF) thisRefUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_RETURNOBJ, thisRef);

  WCHAR *thisChars;
  int thisLength;
  int newLength;
  STRINGREF thisString = NULL;

  THROWSCOMPLUSEXCEPTION();

    if (thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  void *tid;
  thisString = GetThreadSafeString(thisRef,&tid);
  thisLength = thisString->GetArrayLength() - 1;

  
  //Get the needed values
  thisChars = thisString->GetBuffer();
  thisLength = thisString->GetStringLength();
  
    if (length<0) {
      COMPlusThrowArgumentOutOfRange(L"length", L"ArgumentOutOfRange_NegativeLength");
  } 

    if (startIndex<0) {
      COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_StartIndex");
  }

    if ((length) > (thisLength-startIndex)) {
      COMPlusThrowArgumentOutOfRange(L"index", L"ArgumentOutOfRange_Index");
  }

  //Move the remaining characters to the left and set the string length.
    memcpyNoGCRefs(&(thisChars[startIndex]),&(thisChars[startIndex+length]), (thisLength-(startIndex+length))*sizeof(WCHAR));
    newLength=thisLength-length;

  thisString->SetStringLength(newLength);
  thisChars[newLength]='\0';
  _ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));

  ReplaceStringRef(thisRef, tid, thisString);

    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(thisRef);
}
FCIMPLEND

/*==============================LocalIndexOfString==============================
**Finds search within base and returns the index where it was found.  The search
**starts from startPos and we return -1 if search isn't found.  This is a direct 
**copy from COMString::IndexOfString, but doesn't require that we build up
**an instance of indexOfStringArgs before calling it.  
**
**Args:
**base -- the string in which to search
**search -- the string for which to search
**strLength -- the length of base
**patternLength -- the length of search
**startPos -- the place from which to start searching.
**
==============================================================================*/
INT32 COMStringBuffer::LocalIndexOfString(WCHAR *base, WCHAR *search, int strLength, int patternLength, int startPos) {
  int iThis, iPattern;
  for (iThis=startPos; iThis < (strLength-patternLength+1); iThis++) {
    for (iPattern=0; iPattern<patternLength && base[iThis+iPattern]==search[iPattern]; iPattern++);
    if (iPattern == patternLength) return iThis;
  }
  return -1;
}

/*================================ReplaceString=================================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL5(Object*, COMStringBuffer::ReplaceString, StringBufferObject* thisRefUNSAFE, StringObject* oldValueUNSAFE, StringObject* newValueUNSAFE, INT32 startIndex, INT32 count)
{
    struct _gc
    {
        STRINGBUFFERREF thisRef;
        STRINGREF       oldValue;
        STRINGREF       newValue;
    } gc;

    gc.thisRef     = (STRINGBUFFERREF)  thisRefUNSAFE;
    gc.oldValue    = (STRINGREF)        oldValueUNSAFE;
    gc.newValue    = (STRINGREF)        newValueUNSAFE;

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_NOPOLL(Frame::FRAME_ATTR_RETURNOBJ);
    GCPROTECT_BEGIN(gc);
    HELPER_METHOD_POLL();

  int *replaceIndex;
  int index=0;
  INT64 newBuffLength=0;
  int replaceCount=0;
  int readPos, writePos;
  int indexAdvance=0;
  WCHAR *thisBuffer, *oldBuffer, *newBuffer;
  int thisLength, oldLength, newLength;
  int endIndex;
  CQuickBytes replaceIndices;
  STRINGREF thisString=NULL;

  THROWSCOMPLUSEXCEPTION();

    if (gc.thisRef==NULL) {
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
  }

  //Verify all of the arguments.
    if (!gc.oldValue) {
    COMPlusThrowArgumentNull(L"oldValue", L"ArgumentNull_Generic");
  }

  //If they asked to replace oldValue with a null, replace all occurances
  //with the empty string.
    if (!gc.newValue) {
        gc.newValue = COMString::GetEmptyString();
  }

  void *tid;
  thisString = GetThreadSafeString(gc.thisRef,&tid);
  thisLength = thisString->GetStringLength();
  thisBuffer = thisString->GetBuffer();

    RefInterpretGetStringValuesDangerousForGC(gc.oldValue, &oldBuffer, &oldLength);
    RefInterpretGetStringValuesDangerousForGC(gc.newValue, &newBuffer, &newLength);

  //Range check our String.
    if (startIndex<0 || startIndex>thisLength) {
      COMPlusThrowArgumentOutOfRange(L"startIndex", L"ArgumentOutOfRange_Index");
  }
  
    if (count<0 || startIndex > thisLength - count) {
      COMPlusThrowArgumentOutOfRange(L"count", L"ArgumentOutOfRange_Index");
  }

  //Record the endIndex so that we don't need to do this calculation all over the place.
    endIndex = startIndex + count;

  //If our old Length is 0, we won't know what to replace
  if (oldLength==0) {
      COMPlusThrowArgumentException(L"oldValue", L"Argument_StringZeroLength");
  }

  //replaceIndex is made large enough to hold the maximum number of replacements possible:
  //The case where every character in the current buffer gets replaced.
  replaceIndex = (int *)replaceIndices.Alloc((thisLength/oldLength+1)*sizeof(int));

  //Calculate all of the indices where our oldStrings end.  Finding the
  //ending is important because we're about to walk the string backwards.
  //If we're going to be walking the array backwards, we need to record the
  //end of the matched string, hence indexAdvance.
  if (newLength>oldLength) {
      indexAdvance = oldLength - 1;
  }

    index=startIndex;
  while (((index=LocalIndexOfString(thisBuffer,oldBuffer,thisLength,oldLength,index))>-1) && (index<=endIndex-oldLength)) {
      replaceIndex[replaceCount++] = index + indexAdvance;
      index+=oldLength;
  }

  //Calculate the new length of the string and ensure that we have sufficent room.
  newBuffLength = thisLength - ((oldLength - newLength) * (INT64)replaceCount);
  if (newBuffLength > 0x7FFFFFFF)
       COMPlusThrowOM();

  thisString = GetRequiredString(&gc.thisRef, thisString, (INT32)newBuffLength);

  //Get another pointer to the buffer in case it changed during the assure.
  thisBuffer = thisString->GetBuffer();
    newBuffer = gc.newValue->GetBuffer();

    int replaceHolder = 0;

  //Handle the case where our new string is longer than the old string.
  //This requires walking the buffer backwards in order to do an in-place
  //replacement.
  if (newLength > oldLength) {
    //Decrement replaceCount so that we can use it as an actual index into our array.
    replaceCount--;

    //Walk the array backwards copying each character as we go.  If we reach an instance
    //of the string being replaced, replace the old string with the new string.
    readPos = thisLength-1;
    writePos = newBuffLength-1; 
    while (readPos>=0) {
      if (replaceCount>=0&&readPos==replaceIndex[replaceCount]) {
    replaceCount--;
    readPos-=(oldLength);
    writePos-=(newLength);
    memcpyNoGCRefs(&thisBuffer[writePos+1], newBuffer, newLength*sizeof(WCHAR));
      } else {
    thisBuffer[writePos--] = thisBuffer[readPos--];
      }
    }
    thisBuffer[newBuffLength]='\0';
    //Set the new String length and return.
    thisString->SetStringLength(newBuffLength);
	_ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));

    ReplaceStringRef(gc.thisRef, tid, thisString);

        goto lExit;
  }

  //Handle the case where our old string is longer than or the same size as
  //the string with which we're about to replace it. This requires us to walk 
  //the buffer forward, differentiating it from the above case which requires us
  //to walk the array backwards.
  
  //Set replaceHolder to be the upper limit of our array.
    replaceHolder = replaceCount;
  replaceCount=0;

  //Walk the array forwards copying each character as we go.  If we reach an instance
  //of the string being replaced, replace the old string with the new string.
  readPos = 0;
  writePos = 0;
  while (readPos<thisLength) {
    if (replaceCount<replaceHolder&&readPos==replaceIndex[replaceCount]) {
      replaceCount++;
      readPos+=(oldLength);
      memcpyNoGCRefs(&thisBuffer[writePos], newBuffer, newLength*sizeof(WCHAR));
      writePos+=(newLength);
    } else {
      thisBuffer[writePos++] = thisBuffer[readPos++];
    }
  }
  thisBuffer[newBuffLength]='\0';

  thisString->SetStringLength(newBuffLength);
  _ASSERTE(IS_STRING_STATE_UNDETERMINED(thisString->GetHighCharState()));

  //Set the new String length and return.
    ReplaceStringRef(gc.thisRef, tid, thisString);

lExit: ;
    GCPROTECT_END();
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(gc.thisRef);
}
FCIMPLEND

/*==============================NewStringBuffer=================================
**Makes a new empty string buffer with the given capacity.  For EE use.
==============================================================================*/

STRINGBUFFERREF COMStringBuffer::NewStringBuffer(INT32 capacity) {
    STRINGREF Local;
    THROWSCOMPLUSEXCEPTION();

    STRINGBUFFERREF Buffer;

    Local = COMString::NewString(capacity);
    Local->SetStringLength(0); //This is a cheap hack to get the empty string.

    _ASSERTE(s_pStringBufferClass != NULL);

    GCPROTECT_BEGIN(Local);
    Buffer = (STRINGBUFFERREF) AllocateObject(s_pStringBufferClass);
    GCPROTECT_END();//Local

    Buffer->SetStringRef(Local);
    Buffer->SetCurrentThread(::GetThread());
    Buffer->SetMaxCapacity(capacity);

    return Buffer;
}


/*===============================LoadStringBuffer===============================
**Initialize the COMStringBuffer Class.  Stores a reference to the class in  
**a static member of COMStringBuffer.
**
**Returns S_OK if it succeeded.  E_FAIL if it is unable to initialze the class.
**
**Args: None
==============================================================================*/
HRESULT __stdcall COMStringBuffer::LoadStringBuffer() {
  
  // Load the StringBuffer
  COMStringBuffer::s_pStringBufferClass = g_Mscorlib.GetClass(CLASS__STRING_BUILDER);
  
  return S_OK;
}









