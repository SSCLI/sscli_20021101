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
using System.IO;
using System.Text;
using System;
public class Co1117CompareTo
{
 public virtual bool runTest()
   {
   Console.Error.WriteLine( "Co1117CompareTo  runTest() started." );
   String strLoc="Loc_000oo";
   StringBuilder sblMsg = new StringBuilder( 99 );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   int in4b = -12;
   Int32 int4a = 55;
   try
     {
     LABEL_860_GENERAL:
     do
       {
       strLoc="Loc_120au";
       in4b = -12;
       in4b = int4a.CompareTo( (Int32)66 );
       ++iCountTestcases;
       if ( !( in4b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_337uf!  in4b=="+ in4b  );
	 }
       in4b = -12;
       in4b = int4a.CompareTo( 55 );
       ++iCountTestcases;
       if ( !( in4b == -0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_248ak!  in4b=="+ in4b  );
	 }
       in4b = -12;
       in4b = int4a.CompareTo( -77 );
       ++iCountTestcases;
       if ( !( in4b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_411wq!  in4b=="+ in4b  );
	 }
       strLoc="Loc_201ax";
       in4b = -12;
       in4b = int4a.CompareTo( null );  
       ++iCountTestcases;
       if ( !( in4b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_713tw!  in4b=="+ in4b  );
	 }
       strLoc="Loc_202bx";
       in4b = -12;
       in4b = int4a.CompareTo( (Type)null );  
       ++iCountTestcases;
       if ( !( in4b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_714rw!  in4b=="+ in4b  );
	 }
       strLoc="Loc_205ex";
       in4b = -12;
       in4b = ( -0 ).CompareTo( null );
       ++iCountTestcases;
       if ( !( in4b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_716mw!  in4b=="+ in4b  );
	 }
       strLoc="Loc_301zv";
       in4b = -12;
       in4b = ( Int32.MinValue ).CompareTo( ( Int32.MinValue + 1 ) );
       ++iCountTestcases;
       if ( !( in4b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_958je!  in4b=="+ in4b  );
	 }
       strLoc="Loc_303uv";
       Console.Error.WriteLine( "Info Inf_774dx.  (Integer4.MinValue-Integer4.MaxValue)=="+ unchecked(Int32.MinValue-Int32.MaxValue) );
       strLoc="Loc_302yv";
       in4b = -12;
       in4b = ( Int32.MinValue ).CompareTo( Int32.MaxValue );  
       ++iCountTestcases;
       if ( !( in4b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_842sc!  in4b=="+ in4b  );
	 }
       strLoc="Loc_400hh";
       try
	 {
	 in4b = -12;
	 in4b = int4a.CompareTo( (Int16)22 );  
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_522yh!  in4b=="+ in4b  );
	 }
       catch ( ArgumentException argexc )
	 {}
       catch ( Exception excep1 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_636pi!  excep1=="+ excep1  );
	 }
       } while ( false );
     }
   catch( Exception exc_general )
     {
     ++iCountErrors;
     Console.Error.WriteLine(  "POINTTOBREAK: Error Err_343un! (Co1117CompareTo) exc_general==" + exc_general  );
     Console.Error.WriteLine(  "EXTENDEDINFO: (Err_343un) strLoc==" + strLoc  );
     }
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( "paSs.   Integer4\\Co1117CompareTo.cs   iCountTestcases==" + iCountTestcases );
     return true;
     }
   else
     {
     Console.Error.WriteLine( "FAiL!   Integer4\\Co1117CompareTo.cs   iCountErrors==" + iCountErrors );
     return false;
     }
   }
 public static void Main( String[] args )
   {
   bool bResult = false; 
   Co1117CompareTo cbA = new Co1117CompareTo();
   try
     {
     bResult = cbA.runTest();
     }
   catch ( Exception exc_main )
     {
     bResult = false;
     Console.Error.WriteLine(  "POINTTOBREAK:  FAiL!  Error Err_999zzz! (Co1117CompareTo) Uncaught Exception caught in main(), exc_main==" + exc_main  );
     }
   if ( ! bResult )
     Console.Error.WriteLine(  "Co1117CompareTo.cs   FAiL!"  );
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
}
