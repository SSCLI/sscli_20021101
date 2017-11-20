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
using System;
using GenStrings;
using System.Text ;

public class test{

 public static void Main()
   {
   IntlStrings intl = new IntlStrings( 0x41F );
   Console.WriteLine(intl.GetRandomDirectoryName( 5 ));
   for(int iLoop = 0 ; iLoop < 5 ; iLoop++)
     Console.WriteLine( intl.GetTop20String( 20, true, true ));
   for(int iLoop = 0 ; iLoop < 5 ; iLoop++)
     Console.WriteLine( intl.GetProbCharString( 20, true, true ));
   }
}
