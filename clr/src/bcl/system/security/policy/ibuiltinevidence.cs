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
//  IBuiltInEvidence.cs
//

namespace System.Security.Policy
{
	using System;
	using System.IO;

    internal interface IBuiltInEvidence
    {
        int OutputToBuffer( char[] buffer, int position, bool verbose );

        // Initializes a class according to data in the buffer. Returns new position within buffer
        int InitFromBuffer( char[] buffer, int position);

        int GetRequiredSize(bool verbose);

    }

    internal class BuiltInEvidenceHelper
    {
        internal const char idApplicationDirectory = (char)0;
        internal const char idStrongName = (char)2;
        internal const char idZone = (char)3;
        internal const char idUrl = (char)4;
        internal const char idWebPage = (char)5;
        internal const char idSite = (char)6;
        internal const char idPermissionRequestEvidence = (char)7;

        internal static void CopyIntToCharArray( int value, char[] buffer, int position )
        {
            buffer[position    ] = (char)((value >> 16) & 0x0000FFFF);
            buffer[position + 1] = (char)((value      ) & 0x0000FFFF);
        }

        internal static int GetIntFromCharArray(char[] buffer, int position )
        {
            int value = (int)buffer[position];
            value = value << 16;
            value += (int)buffer[position + 1];
            return value;
        }
    }
            

}
