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
public class Co1118CompareTo
{
 public virtual bool runTest()
   {
   Console.Error.WriteLine( "Co1118CompareTo  runTest() started." );
   String strLoc="Loc_000oo";
   StringBuilder sblMsg = new StringBuilder( 99 );
   int iCountErrors = 0;
   int iCountTestcases = 0;
   long lo8b = -12;
   Int64 int8a = 55;
   try
     {
     LABEL_860_GENERAL:
     do
       {
       strLoc="Loc_120au";
       strLoc="Loc_121eu";
       lo8b = -12;
       lo8b = int8a.CompareTo( (Int64)66 );
       ++iCountTestcases;
       if ( !( lo8b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_337uf!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_122fu";
       lo8b = -12;
       lo8b = int8a.CompareTo( 55L );
       ++iCountTestcases;
       if ( !( lo8b == -0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_248ak!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_123gu";
       lo8b = -12;
       lo8b = int8a.CompareTo( (Int64)(-77) );
       ++iCountTestcases;
       if ( !( lo8b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_411wq!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_201ax";
       lo8b = -12;
       lo8b = int8a.CompareTo( null );  
       ++iCountTestcases;
       if ( !( lo8b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_713tw!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_202bx";
       lo8b = -12;
       lo8b = int8a.CompareTo( (Type)null );  
       ++iCountTestcases;
       if ( !( lo8b > 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_714rw!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_301zv";
       lo8b = -12;
       lo8b = ( Int64.MinValue ).CompareTo( ( Int64.MinValue + 1 ) );
       ++iCountTestcases;
       if ( !( lo8b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_958je!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_303uv";
       Console.Error.WriteLine( "Info Inf_774dx.  (Integer8.MinValue-Integer8.MaxValue)=="+ unchecked(Int64.MinValue-Int64.MaxValue) );
       strLoc="Loc_302yv";
       lo8b = -12;
       lo8b = ( Int64.MinValue ).CompareTo( Int64.MaxValue );  
       ++iCountTestcases;
       if ( !( lo8b < 0 ) )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_842sc!  lo8b=="+ lo8b  );
	 }
       strLoc="Loc_400hh";
       strLoc="Loc_401ih";
       try
	 {
	 ++iCountTestcases;
	 lo8b = -12;
	 lo8b = int8a.CompareTo( (Int16)22 );  
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_522yh!  lo8b=="+ lo8b  );
	 }
       catch ( ArgumentException argexc )
	 {}
       catch ( Exception excep1 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_636pi!  excep1=="+ excep1  );
	 }
       strLoc="Loc_402jh";
       try
	 {
	 ++iCountTestcases;
	 lo8b = -12;
	 lo8b = int8a.CompareTo( 22 );  
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_452yh!  lo8b=="+ lo8b  );
	 }
       catch ( ArgumentException argexc )
	 {}
       catch ( Exception excep1 )
	 {
	 ++iCountErrors;
	 Console.Error.WriteLine(  "POINTTOBREAK:  Error Err_796pi!  excep1=="+ excep1  );
	 }
       } while ( false );
     }
   catch( Exception exc_general )
     {
     ++iCountErrors;
     Console.Error.WriteLine(  "POINTTOBREAK: Error Err_343un! (Co1118CompareTo) exc_general==" + exc_general  );
     Console.Error.WriteLine(  "EXTENDEDINFO: (Err_343un) strLoc==" + strLoc  );
     }
   if ( iCountErrors == 0 )
     {
     Console.Error.WriteLine( "paSs.   Integer8\\Co1118CompareTo.cs   iCountTestcases==" + iCountTestcases );
     return true;
     }
   else
     {
     Console.Error.WriteLine( "FAiL!   Integer8\\Co1118CompareTo.cs   iCountErrors==" + iCountErrors );
     return false;
     }
   }
 public static void Main( String[] args )
   {
   bool bResult = false; 
   Co1118CompareTo cbA = new Co1118CompareTo();
   try
     {
     bResult = cbA.runTest();
     }
   catch ( Exception exc_main )
     {
     bResult = false;
     Console.Error.WriteLine(  "POINTTOBREAK:  FAiL!  Error Err_999zzz! (Co1118CompareTo) Uncaught Exception caught in main(), exc_main==" + exc_main  );
     }
   if ( ! bResult )
     Console.Error.WriteLine(  "Co1118CompareTo.cs   FAiL!"  );
   if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1; 
   }
}
