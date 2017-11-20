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
 *
 * Class:  INormalizeForIsolatedStorage
 *
 *                                    
 *
 * Purpose: Evidence types can optionaly implement this interface.
 *          IsolatedStorage calls Normalize method before evidence
 *          is serialized. The Normalize method should return a copy
 *          of the evidence instance if any of it's fields is changed.
 *
 * Date:  Oct 17, 2000
 *
 ===========================================================*/
namespace System.IO.IsolatedStorage {

	using System;

    /// <include file='doc\INormalizeForIsolatedStorage.uex' path='docs/doc[@for="INormalizeForIsolatedStorage"]/*' />
    public interface INormalizeForIsolatedStorage
    {
        /// <include file='doc\INormalizeForIsolatedStorage.uex' path='docs/doc[@for="INormalizeForIsolatedStorage.Normalize"]/*' />
        // Return a copy of the normalized version of this instance,
        // so that a the serialized version of this object can be 
        // mem-compared to another serialized object
        //
        // 1. Eg.  (pseudo code to illustrate usage)
        //
        // obj1 = MySite(WWW.MSN.COM)
        // obj2 = MySite(www.msn.com)
        //
        // obj1Norm = obj1.Normalize() 
        // obj2Norm = obj1.Normalize() 
        //
        // stream1 = Serialize(obj1Norm)
        // stream2 = Serialize(obj2Norm)
        //
        // AreStreamsEqual(stream1, stream2) returns true
        //
        // If the Object returned is a stream, the stream will be used without 
        // serialization. If the Object returned is a string, the string will 
        // be used in naming the Store. If the string is too long or if the
        // string contains chars that are illegal to use in naming the store,
        // the string will be serialized.
        //
        // 2. Eg. (pseudo code to illustrate returning string)
        //
        // obj1 = MySite(WWW.MSN.COM)
        // obj2 = MySite(www.msn.com)
        //
        // string1 = obj1.Normalize() 
        // string2 = obj1.Normalize() 
        //
        // AreStringsEqual(string1, string2) returns true
        //
        // 3. Eg. (pseudo code to illustrate returning stream)
        //
        // obj1 = MySite(WWW.MSN.COM)
        // obj2 = MySite(www.msn.com)
        //
        // stream1 = obj1.Normalize() 
        // stream2 = obj1.Normalize() 
        //
        // AreStreamsEqual(stream1, stream2) returns true

        Object Normalize();
    }
}

