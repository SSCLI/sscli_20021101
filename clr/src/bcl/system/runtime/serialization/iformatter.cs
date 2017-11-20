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
** Interface: IFormatter;
**
**                                        
**
** Purpose: The interface for all formatters.
**
** Date:  April 22, 1999
**
===========================================================*/
namespace System.Runtime.Serialization {
	using System.Runtime.Remoting;
	using System;
	using System.IO;

    /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter"]/*' />
    public interface IFormatter {
        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Deserialize"]/*' />
        Object Deserialize(Stream serializationStream);

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Serialize"]/*' />
        void Serialize(Stream serializationStream, Object graph);


        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.SurrogateSelector"]/*' />
        ISurrogateSelector SurrogateSelector {
            get;
            set;
        }

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Binder"]/*' />
        SerializationBinder Binder {
            get;
            set;
        }

        /// <include file='doc\IFormatter.uex' path='docs/doc[@for="IFormatter.Context"]/*' />
        StreamingContext Context {
            get;
            set;
        }
    }
}
