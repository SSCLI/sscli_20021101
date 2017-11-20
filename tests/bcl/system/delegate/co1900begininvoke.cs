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
using System;
using System.Threading;
using System.Runtime.Remoting.Messaging;
delegate int Del1(int ms);
delegate void 		dlg_void_void		();
delegate String 	dlg_String_String	( ref String p_str1 );
delegate Boolean	dlg_Boolean_Boolean	( ref Boolean p_1 );  
delegate SByte    	dlg_SByte_SByte  	( ref SByte   p_1 );
delegate Byte   	dlg_Byte_Byte    	( ref Byte   p_1 );
delegate char    	dlg_char_char      	( ref char   p_1 );
delegate double  	dlg_double_double  	( ref double p_1 );
delegate float   	dlg_float_float    	( ref float  p_1 );
delegate long    	dlg_long_long      	( ref long   p_1 );
delegate short   	dlg_short_short    	( ref short  p_1 );
delegate int 		dlg_int_int		( ref int p_1 );
delegate void 		dlg_all			( ref String st, ref Boolean bo, ref SByte sb, ref Byte by, ref char c, ref double d, ref float f, ref long l, ref short sh, ref int i );
public class ExecClass
{
 public void execMethod_void_void 		() {}
 public String execMethod_String_String		( ref String p_str1 ) 	{ value_String = const_String;
 return p_str1; }
 public Boolean execMethod_Boolean_Boolean	( ref Boolean p_1 )	{ value_Boolean = const_Boolean;
 return p_1; }
 public SByte execMethod_SByte_SByte		( ref SByte p_1 ) 	{ value_SByte = const_SByte;
 return p_1; }
 public Byte execMethod_Byte_Byte		( ref Byte p_1 ) 	{ value_Byte = const_Byte;
 return p_1; }
 protected char execMethod_char_char		( ref char p_1 ) 	{ value_char = const_char;
 return p_1; }
 public double execMethod_double_double		( ref double p_1 )	{ value_double = const_double;
 return p_1; }
 public float execMethod_float_float		( ref float p_1 )	{ value_float = const_float;
 return p_1; }
 public long execMethod_long_long		( ref long p_1 ) 	{ value_long = const_long;
 return p_1; }
 internal short execMethod_short_short		( ref short p_1 )	{ value_short = const_short;
 return p_1; }
 internal int   execMethod_int_int		( ref int p_1 )		{ value_int = const_int;
 return p_1; }
 public void execMethod_all			( ref String st, ref Boolean bo, ref SByte sb, ref Byte by, ref char c, 
						  ref double d, ref float f, ref long l, ref short sh, ref int i )
   {
   value_String = const_String;
   value_Boolean = const_Boolean;	
   value_SByte = const_SByte;
   value_Byte = const_Byte;
   value_char = const_char;
   value_double = const_double;
   value_float = const_float;
   value_long = const_long;
   value_short = const_short;
   value_int = const_int;
   }
 public static void static_execMethod_void_void		() {}
 public static String static_execMethod_String_String	( ref String p_str1 )	{ value_String = const_String;
 return p_str1; }
 public static Boolean static_execMethod_Boolean_Boolean	( ref Boolean p_1 )	{ value_Boolean = const_Boolean;
 return p_1; }
 public static SByte static_execMethod_SByte_SByte	( ref SByte p_1 )	{ value_SByte = const_SByte;
 return p_1; }
 public static Byte static_execMethod_Byte_Byte		( ref Byte p_1 )	{ value_Byte = const_Byte;
 return p_1; }
 protected static char static_execMethod_char_char	( ref char p_1 )	{ value_char = const_char;
 return p_1; }
 public static double static_execMethod_double_double	( ref double p_1 )	{ value_double = const_double;
 return p_1; }
 public static float static_execMethod_float_float	( ref float p_1 )	{ value_float = const_float;
 return p_1; }
 public static long static_execMethod_long_long		( ref long p_1 )	{ value_long = const_long;
 return p_1; }
 internal static short static_execMethod_short_short	( ref short p_1 )	{ value_short = const_short;
 return p_1; }
 internal static int static_execMethod_int_int		( ref int p_1 )		{ value_int = const_int;
 return p_1; }
 public static void static_execMethod_all		( ref String st, ref Boolean bo, ref SByte sb, ref Byte by, ref char c, 
							  ref double d, ref float f, ref long l, ref short sh, ref int i )
   {
   value_String = const_String;
   value_Boolean = const_Boolean;	
   value_SByte = const_SByte;
   value_Byte = const_Byte;
   value_char = const_char;
   value_double = const_double;
   value_float = const_float;
   value_long = const_long;
   value_short = const_short;
   value_int = const_int;
   }
 public static String value_String = null;
 public static Boolean value_Boolean = true;
 public static SByte value_SByte = 0;
 public static Byte value_Byte = 0;
 public static char value_char = 'd';
 public static double value_double = 0;
 public static float value_float = 0;
 public static long value_long = 0;
 public static short value_short =0;
 public static int value_int=0;
 public const String const_String = "weird\\,\\+\\;tect\\,";
 public const Boolean const_Boolean = false;
 public const sbyte const_SByte = (sbyte)-4;
 public const byte const_Byte = 1;
 public const char const_char = 'T';
 public const double const_double = (double)234;
 public const float const_float = (float)-4.3;
 public const long const_long = (long)-5.6;
 public const short const_short = (short)-8;
 public const int const_int=2323;
 public const String param_String = "weird\\,\\+\\;tect\\,";
 public const Boolean param_Boolean = false;
 public const sbyte param_SByte = (sbyte)-4;
 public const byte param_Byte = 1;
 public const char param_char = 'T';
 public const double param_double = (double)234;
 public const float param_float = (float)-4.3;
 public const long param_long = (long)-5.6;
 public const short param_short = (short)-8;
 public const int param_int=2323;
 public static void resetValues () {
 value_String = null;
 value_Boolean = true;
 value_SByte = 0;
 value_Byte = 0;
 value_char = 'd';
 value_double = 0;
 value_float = 0;
 value_long = 0;
 value_short =0;
 value_int=0;
 }
}
public class CallbkClass 
{
 public void callbkMethod_void_void	(IAsyncResult ar) {
 dlg_void_void dlg = (dlg_void_void)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ar);
 }
 public void callbkMethod_String_String	(IAsyncResult ar) {
 String result=null;
 dlg_String_String dlg = (dlg_String_String)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_String) {
 throw (new Exception ("Err_Yeh_callBk_001a: String parameter has a wrong value"));
 }
 if (ExecClass.value_String!=ExecClass.const_String) {
 throw (new Exception ("Err_Yeh_callBk_001: String returned has a wrong value"));
 }
 }
 public void callbkMethod_Boolean_Boolean(IAsyncResult ar) {
 Boolean result=true;
 dlg_Boolean_Boolean dlg = (dlg_Boolean_Boolean)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_Boolean) {
 throw (new Exception ("Err_Yeh_callBk_002a: Boolean parameter has a wrong value"));
 }
 if (ExecClass.value_Boolean!=ExecClass.const_Boolean) {
 throw (new Exception ("Err_Yeh_callBk_002: Boolean returned has a wrong value"));
 }
 }
 public void callbkMethod_SByte_SByte	(IAsyncResult ar) {
 sbyte result=0;
 dlg_SByte_SByte dlg = (dlg_SByte_SByte)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_SByte) {
 throw (new Exception ("Err_Yeh_callBk_003a: SByte parameter has a wrong value"));
 }
 if (ExecClass.value_SByte!=ExecClass.const_SByte) {
 throw (new Exception ("Err_Yeh_callBk_003: SByte returned has a wrong value"));
 }
 }
 public void callbkMethod_Byte_Byte	(IAsyncResult ar) {
 byte result=0;
 dlg_Byte_Byte dlg = (dlg_Byte_Byte)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_Byte) {
 throw (new Exception ("Err_Yeh_callBk_004a: Byte parameter has a wrong value"));
 }
 if (ExecClass.value_Byte!=ExecClass.const_Byte) {
 throw (new Exception ("Err_Yeh_callBk_004: Byte returned has a wrong value"));
 }
 }
 public void callbkMethod_char_char	(IAsyncResult ar) {
 char result='i';
 dlg_char_char dlg = (dlg_char_char)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_char) {
 throw (new Exception ("Err_Yeh_callBk_005a: char parameter has a wrong value"));
 }
 if (ExecClass.value_char!=ExecClass.const_char) {
 throw (new Exception ("Err_Yeh_callBk_005: char returned has a wrong value"));
 }
 }
 public void callbkMethod_double_double	(IAsyncResult ar) {
 double result=0;
 dlg_double_double dlg = (dlg_double_double)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_double) {
 throw (new Exception ("Err_Yeh_callBk_006a: double parameter has a wrong value"));
 }
 if (ExecClass.value_double!=ExecClass.const_double) {
 throw (new Exception ("Err_Yeh_callBk_006: double returned has a wrong value"));
 }
 }
 public void callbkMethod_float_float	(IAsyncResult ar) {
 float result=0;
 dlg_float_float dlg = (dlg_float_float)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_float) {
 throw (new Exception ("Err_Yeh_callBk_008a: float parameter has a wrong value"));
 }
 if (ExecClass.value_float!=ExecClass.const_float) {
 throw (new Exception ("Err_Yeh_callBk_008: float returned has a wrong value"));
 }
 }
 public void callbkMethod_long_long	(IAsyncResult ar) {
 long result=0;
 dlg_long_long dlg = (dlg_long_long)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_long) {
 throw (new Exception ("Err_Yeh_callBk_009a: long parameter has a wrong value"));
 }
 if (ExecClass.value_long!=ExecClass.const_long) {
 throw (new Exception ("Err_Yeh_callBk_009: long returned has a wrong value"));
 }
 }
 public void callbkMethod_short_short	(IAsyncResult ar) {
 short result=0;
 dlg_short_short dlg = (dlg_short_short)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_short) {
 throw (new Exception ("Err_Yeh_callBk_010a: short parameter has a wrong value"));
 }
 if (ExecClass.value_short!=ExecClass.const_short) {
 throw (new Exception ("Err_Yeh_callBk_010: short returned has a wrong value"));
 }
 }
 public void callbkMethod_int_int	(IAsyncResult ar) {
 int result=0;
 dlg_int_int dlg = (dlg_int_int)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref result, ar);
 if (result!=ExecClass.param_int) {
 throw (new Exception ("Err_Yeh_callBk_011a: int parameter has a wrong value"));
 }
 if (ExecClass.value_int!=ExecClass.const_int) {
 throw (new Exception ("Err_Yeh_callBk_011: int returned has a wrong value"));
 }
 }
 public void callbkMethod_all	(IAsyncResult ar) {
 String 	st	=	null;
 Boolean bo	=	true;
 SByte 	sb	=	0;
 byte 	by	=	0;
 char 	c 	=	'd';
 double 	d	=	0;
 float 	f	=	0;
 long 	l	=	0;
 short 	sh	=	0;
 int	i	=	0;
 dlg_all dlg = (dlg_all)((AsyncResult)ar).AsyncDelegate;
 dlg.EndInvoke (ref st, ref bo, ref sb, ref by, ref c, ref d, ref f, ref l, ref sh, ref i, ar);
 if 	((st!=ExecClass.param_String) &&
	 (bo!=ExecClass.param_Boolean) &&
	 (sb!=ExecClass.param_SByte) &&
	 (by!=ExecClass.param_Byte) &&
	 (c!=ExecClass.param_char) &&
	 (d!=ExecClass.param_double) &&
	 (f!=ExecClass.param_float) &&
	 (l!=ExecClass.param_long) &&
	 (sh!=ExecClass.param_short) &&
	 (i!=ExecClass.param_int)) 
   {
   throw (new Exception ("Err_Yeh_callBk_012a: int parameter has a wrong value"));
   }
 if 	((ExecClass.value_String!=ExecClass.const_String) &&
	 (ExecClass.value_Boolean!=ExecClass.const_Boolean) &&	
	 (ExecClass.value_SByte!=ExecClass.const_SByte) &&	
	 (ExecClass.value_Byte!=ExecClass.const_Byte) &&	
	 (ExecClass.value_char!=ExecClass.const_char) &&	
	 (ExecClass.value_double!=ExecClass.const_double) &&	
	 (ExecClass.value_float!=ExecClass.const_float) &&	
	 (ExecClass.value_long!=ExecClass.const_long) &&	
	 (ExecClass.value_short!=ExecClass.const_short) &&	
	 (ExecClass.value_int!=ExecClass.const_int))
   {
   throw (new Exception ("Err_Yeh_callBk_012: int returned has a wrong value"));
   }
 }
}
class Co1900BeginInvoke: ExecClass
{
 public String s_strActiveBugNums          = "";
 public String s_strDtTmVer                = "";
 public String s_strComponentBeingTested   = "Delegate.???()";
 public String s_strTFName                 = "Co1900BeginInvokector()";
 public String s_strTFAbbrev               = "Co1900BeginInvoke";
 public String s_strTFPath                 = "";
 public Boolean runTest()
   {
   int iCountTestcases = 0;
   int iCountErrors    = 0;
   iCountTestcases++;
   IAsyncResult void_void_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_void_void inst_dlg_void_void = new dlg_void_void (eClass.execMethod_void_void);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_void_void);
   Object state = new Object();
   void_void_result = inst_dlg_void_void.BeginInvoke (cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_001,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult String_String_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_String_String inst_dlg_String_String = new dlg_String_String (eClass.execMethod_String_String);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_String_String);
   Object state = new Object();
   String temp = ExecClass.param_String;
   String_String_result = inst_dlg_String_String.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_001,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult Boolean_Boolean_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_Boolean_Boolean inst_dlg_Boolean_Boolean = new dlg_Boolean_Boolean (eClass.execMethod_Boolean_Boolean);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_Boolean_Boolean);
   Object state = new Object();
   Boolean temp = ExecClass.param_Boolean;
   Boolean_Boolean_result = inst_dlg_Boolean_Boolean.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_003,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult SByte_SByte_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_SByte_SByte inst_dlg_SByte_SByte = new dlg_SByte_SByte (eClass.execMethod_SByte_SByte);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_SByte_SByte);
   Object state = new Object();
   SByte temp = ExecClass.param_SByte;
   SByte_SByte_result = inst_dlg_SByte_SByte.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_004,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult Byte_Byte_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_Byte_Byte inst_dlg_Byte_Byte = new dlg_Byte_Byte (eClass.execMethod_Byte_Byte);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_Byte_Byte);
   Object state = new Object();
   Byte temp = ExecClass.param_Byte;
   Byte_Byte_result = inst_dlg_Byte_Byte.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_005,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult char_char_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_char_char inst_dlg_char_char = new dlg_char_char (execMethod_char_char);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_char_char);
   Object state = new Object();
   char temp = ExecClass.param_char;
   char_char_result = inst_dlg_char_char.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_006,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult double_double_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_double_double inst_dlg_double_double = new dlg_double_double (eClass.execMethod_double_double);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_double_double);
   Object state = new Object();
   double temp = ExecClass.param_double;
   double_double_result = inst_dlg_double_double.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_007,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult float_float_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_float_float inst_dlg_float_float = new dlg_float_float (eClass.execMethod_float_float);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_float_float);
   Object state = new Object();
   float temp = ExecClass.param_float;
   float_float_result = inst_dlg_float_float.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_008,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult long_long_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_long_long inst_dlg_long_long = new dlg_long_long (eClass.execMethod_long_long);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_long_long);
   Object state = new Object();
   long temp = ExecClass.param_long;
   long_long_result = inst_dlg_long_long.BeginInvoke (ref temp, cb, state);		
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_009,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult short_short_result=null; 
   try {
   ExecClass eClass = new ExecClass ();
   dlg_short_short inst_dlg_short_short = new dlg_short_short (eClass.execMethod_short_short);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_short_short);
   Object state = new Object();
   short temp = ExecClass.param_short;
   short_short_result = inst_dlg_short_short.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_010,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   IAsyncResult int_int_result = null;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_int_int inst_dlg_int_int = new dlg_int_int (eClass.execMethod_int_int);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_int_int);
   Object state = new Object();
   int temp = ExecClass.param_int;
   int_int_result = inst_dlg_int_int.BeginInvoke (ref temp, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_011,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   try{
   while (!(short_short_result.IsCompleted &&	
	    void_void_result.IsCompleted &&
	    String_String_result.IsCompleted &&
	    Boolean_Boolean_result.IsCompleted &&
	    SByte_SByte_result.IsCompleted &&
	    Byte_Byte_result.IsCompleted &&
	    char_char_result.IsCompleted &&
	    double_double_result.IsCompleted &&
	    float_float_result.IsCompleted &&
	    long_long_result.IsCompleted &&
	    short_short_result.IsCompleted &&
	    int_int_result.IsCompleted))
     {
     ExecClass.resetValues();
     }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_ddd,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   ExecClass.resetValues();
   iCountTestcases++;
   try {
   ExecClass eClass = new ExecClass ();
   dlg_all inst_dlg_all = new dlg_all (eClass.execMethod_all);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_all);
   Object state = new Object();
   String st	=	ExecClass.param_String;
   Boolean bo	=	ExecClass.param_Boolean;
   SByte sb	=	ExecClass.param_SByte;
   Byte by		=	ExecClass.param_Byte;
   char c 		=	ExecClass.param_char;
   double d	=	ExecClass.param_double;
   float f		=	ExecClass.param_float;
   long l		=	ExecClass.param_long;
   short sh	=	ExecClass.param_short;
   int i		=	ExecClass.param_int;
   IAsyncResult all_result = inst_dlg_all.BeginInvoke (ref st, ref bo, ref sb, ref by, ref c, ref d, 
						       ref f, ref l, ref sh, ref i, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_012,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_all static_dlg_all = new dlg_all (ExecClass.static_execMethod_all);
   CallbkClass cbClass = new CallbkClass();
   AsyncCallback cb = new AsyncCallback (cbClass.callbkMethod_all);
   Object state = new Object();
   String st	=	ExecClass.param_String;
   Boolean bo	=	ExecClass.param_Boolean;
   SByte sb	=	ExecClass.param_SByte;
   Byte by		=	ExecClass.param_Byte;
   char c	 	=	ExecClass.param_char;
   double d	=	ExecClass.param_double;
   float f		=	ExecClass.param_float;
   long l		=	ExecClass.param_long;
   short sh	=	ExecClass.param_short;
   int i		=	ExecClass.param_int;
   IAsyncResult all_result = static_dlg_all.BeginInvoke (ref st, ref bo, ref sb, ref by, ref c, ref d, 
							 ref f, ref l, ref sh, ref i, cb, state);
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_013,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   ExecClass.resetValues();
   iCountTestcases++;
   try {
   dlg_void_void static_dlg_void_void = new dlg_void_void (ExecClass.static_execMethod_void_void);
   CallbkClass cbClass = new CallbkClass();
   void_void_result = static_dlg_void_void.BeginInvoke (null, null);
   void_void_result.AsyncWaitHandle.WaitOne(10000, false);
   if (void_void_result.IsCompleted) {
   static_dlg_void_void.EndInvoke(void_void_result);
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_014,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_int_int static_dlg_int_int = new dlg_int_int (ExecClass.static_execMethod_int_int);
   CallbkClass cbClass = new CallbkClass();
   int temp = ExecClass.param_int;
   int_int_result = static_dlg_int_int.BeginInvoke (ref temp, null, null);
   int_int_result.AsyncWaitHandle.WaitOne(10000, false);
   int result = ExecClass.const_int;
   if (int_int_result.IsCompleted) {			
   static_dlg_int_int.EndInvoke(ref result, int_int_result);
   }
   if (result!=ExecClass.param_int) {
   throw (new Exception ("Err_Yeh_run_15: int parameter has a wrong value"));
   }
   if (ExecClass.value_int!=ExecClass.const_int) {
   throw (new Exception ("Err_Yeh_run_15: int returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_015,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_String_String static_dlg_String_String = new dlg_String_String (ExecClass.static_execMethod_String_String);
   CallbkClass cbClass = new CallbkClass();
   String temp = ExecClass.param_String;
   String_String_result = static_dlg_String_String.BeginInvoke (ref temp, null, null);
   String_String_result.AsyncWaitHandle.WaitOne(10000, false);
   String result = ExecClass.const_String;
   if (String_String_result.IsCompleted) {			
   static_dlg_String_String.EndInvoke(ref result, String_String_result);
   }
   if (result!=ExecClass.param_String) {
   throw (new Exception ("Err_Yeh_run_16: String parameter has a wrong value"));
   }
   if (ExecClass.value_String!=ExecClass.const_String) {
   throw (new Exception ("Err_Yeh_run_16: String returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_016,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_Boolean_Boolean static_dlg_Boolean_Boolean = new dlg_Boolean_Boolean (ExecClass.static_execMethod_Boolean_Boolean);
   CallbkClass cbClass = new CallbkClass();
   Boolean temp = ExecClass.param_Boolean;
   Boolean_Boolean_result = static_dlg_Boolean_Boolean.BeginInvoke (ref temp, null, null);
   Boolean_Boolean_result.AsyncWaitHandle.WaitOne(10000, false);
   Boolean result = ExecClass.const_Boolean;
   if (Boolean_Boolean_result.IsCompleted) {			
   static_dlg_Boolean_Boolean.EndInvoke(ref result, Boolean_Boolean_result);
   }
   if (result!=ExecClass.param_Boolean) {
   throw (new Exception ("Err_Yeh_run_17: Boolean parameter has a wrong value"));
   }
   if (ExecClass.value_Boolean!=ExecClass.const_Boolean) {
   throw (new Exception ("Err_Yeh_run_17: Boolean returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_017,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_SByte_SByte static_dlg_SByte_SByte = new dlg_SByte_SByte (ExecClass.static_execMethod_SByte_SByte);
   CallbkClass cbClass = new CallbkClass();
   SByte temp = ExecClass.param_SByte;
   SByte_SByte_result = static_dlg_SByte_SByte.BeginInvoke (ref temp, null, null);
   SByte_SByte_result.AsyncWaitHandle.WaitOne(10000, false);
   SByte result = ExecClass.const_SByte;
   if (SByte_SByte_result.IsCompleted) {			
   static_dlg_SByte_SByte.EndInvoke(ref result, SByte_SByte_result);
   }
   if (result!=ExecClass.param_SByte) {
   throw (new Exception ("Err_Yeh_run_18: SByte parameter has a wrong value"));
   }
   if (ExecClass.value_SByte!=ExecClass.const_SByte) {
   throw (new Exception ("Err_Yeh_run_18: SByte returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_018,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_Byte_Byte static_dlg_Byte_Byte = new dlg_Byte_Byte (ExecClass.static_execMethod_Byte_Byte);
   CallbkClass cbClass = new CallbkClass();
   byte temp = ExecClass.param_Byte;
   Byte_Byte_result = static_dlg_Byte_Byte.BeginInvoke (ref temp, null, null);
   Byte_Byte_result.AsyncWaitHandle.WaitOne(10000, false);
   byte result = ExecClass.const_Byte;
   if (Byte_Byte_result.IsCompleted) {			
   static_dlg_Byte_Byte.EndInvoke(ref result, Byte_Byte_result);
   }
   if (result!=ExecClass.param_Byte) {
   throw (new Exception ("Err_Yeh_run_19: Byte parameter has a wrong value"));
   }
   if (ExecClass.value_Byte!=ExecClass.const_Byte) {
   throw (new Exception ("Err_Yeh_run_19: Byte returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_019,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_char_char static_dlg_char_char = new dlg_char_char (static_execMethod_char_char);
   CallbkClass cbClass = new CallbkClass();
   char temp = ExecClass.param_char;
   char_char_result = static_dlg_char_char.BeginInvoke (ref temp, null, null);
   char_char_result.AsyncWaitHandle.WaitOne(10000, false);
   char result = ExecClass.const_char;
   if (char_char_result.IsCompleted) {			
   static_dlg_char_char.EndInvoke(ref result, char_char_result);
   }
   if (result!=ExecClass.param_char) {
   throw (new Exception ("Err_Yeh_run_20: char parameter has a wrong value"));
   }
   if (ExecClass.value_char!=ExecClass.const_char) {
   throw (new Exception ("Err_Yeh_run_20: char returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_020,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_double_double static_dlg_double_double = new dlg_double_double (ExecClass.static_execMethod_double_double);
   CallbkClass cbClass = new CallbkClass();
   double temp = ExecClass.param_double;
   double_double_result = static_dlg_double_double.BeginInvoke (ref temp, null, null);
   double_double_result.AsyncWaitHandle.WaitOne(10000, false);
   double result = ExecClass.const_double;
   if (double_double_result.IsCompleted) {			
   static_dlg_double_double.EndInvoke(ref result, double_double_result);
   }
   if (result!=ExecClass.param_double) {
   throw (new Exception ("Err_Yeh_run_21: double parameter has a wrong value"));
   }
   if (ExecClass.value_double!=ExecClass.const_double) {
   throw (new Exception ("Err_Yeh_run_21: double returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_021,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_float_float static_dlg_float_float = new dlg_float_float (ExecClass.static_execMethod_float_float);
   CallbkClass cbClass = new CallbkClass();
   float temp = ExecClass.param_float;
   float_float_result = static_dlg_float_float.BeginInvoke (ref temp, null, null);
   float_float_result.AsyncWaitHandle.WaitOne(10000, false);
   float result = ExecClass.const_float;
   if (float_float_result.IsCompleted) {			
   static_dlg_float_float.EndInvoke(ref result, float_float_result);
   }
   if (result!=ExecClass.param_float) {
   throw (new Exception ("Err_Yeh_run_22: float parameter has a wrong value"));
   }
   if (ExecClass.value_float!=ExecClass.const_float) {
   throw (new Exception ("Err_Yeh_run_22: float returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_022,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_long_long static_dlg_long_long = new dlg_long_long (ExecClass.static_execMethod_long_long);
   CallbkClass cbClass = new CallbkClass();
   long temp = ExecClass.param_long;
   long_long_result = static_dlg_long_long.BeginInvoke (ref temp, null, null);
   long_long_result.AsyncWaitHandle.WaitOne(10000, false);
   long result = ExecClass.const_long;
   if (long_long_result.IsCompleted) {			
   static_dlg_long_long.EndInvoke(ref result, long_long_result);
   }
   if (result!=ExecClass.param_long) {
   throw (new Exception ("Err_Yeh_run_23: long parameter has a wrong value"));
   }
   if (ExecClass.value_long!=ExecClass.const_long) {
   throw (new Exception ("Err_Yeh_run_23: long returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_023,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   dlg_short_short static_dlg_short_short = new dlg_short_short (ExecClass.static_execMethod_short_short);
   CallbkClass cbClass = new CallbkClass();
   short temp = ExecClass.param_short;
   short_short_result = static_dlg_short_short.BeginInvoke (ref temp, null, null);
   short_short_result.AsyncWaitHandle.WaitOne(10000, false);
   short result = ExecClass.const_short;
   if (short_short_result.IsCompleted) {			
   static_dlg_short_short.EndInvoke(ref result, short_short_result);
   }
   if (result!=ExecClass.param_short) {
   throw (new Exception ("Err_Yeh_run_24: short parameter has a wrong value"));
   }
   if (ExecClass.value_short!=ExecClass.const_short) {
   throw (new Exception ("Err_Yeh_run_24: short returned has a wrong value"));
   }
   }
   catch (Exception ex){
   ++iCountErrors;
   Console.WriteLine("Err_Yeh_024,  Unexpected exception was thrown ex: " + ex.ToString());
   }	
   iCountTestcases++;
   try {
   slowmethod sm = new slowmethod();
   Del1 del1 = new Del1(sm.goslow);
   IAsyncResult ar = del1.BeginInvoke(50, null, null);
   if (ar.IsCompleted)
     throw new Exception("delegate ran *synchronously*");
   Object obj = del1.EndInvoke(ar);
   if ((int) obj != 50)
     throw new Exception("EndInvoke returned wrong value");
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_001a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   iCountTestcases++;
   try {
   slowmethod sm = new slowmethod();
   Del1 del1 = new Del1(sm.goslow);
   IAsyncResult ar = del1.BeginInvoke(50, null, null);
   Thread.Sleep(100);
   if (! ar.IsCompleted)
     throw new Exception("delegate ran synchronously");
   Object obj = del1.EndInvoke(ar);
   if ((int) obj != 50)
     throw new Exception("EndInvoke returned wrong value");
   }
   catch (Exception ex) {
   ++iCountErrors;
   Console.WriteLine("Err_002a,  Unexpected exception was thrown ex: " + ex.ToString());
   }
   if ( iCountErrors == 0 ) {   return true; }
   else {  return false;}
   }
 public static void Main( String [] args )
   {
   Co1900BeginInvoke runClass = new Co1900BeginInvoke();
   Boolean bResult = runClass.runTest();
   if (! bResult) {
   Console.WriteLine( runClass.s_strTFPath  + runClass.s_strTFName );
   Console.Error.WriteLine( " " );
   Console.Error.WriteLine( "FAiL!  " + runClass.s_strTFAbbrev );
   Console.Error.WriteLine( " " );
   Console.Error.WriteLine( "ACTIVE BUGS: "  + runClass.s_strActiveBugNums );
   }
   if ( bResult == true )
     Environment.ExitCode = 0;
   else
     Environment.ExitCode = 11;
   }
}
class slowmethod {
 public int goslow(int ms) {
 Thread.Sleep(ms);
 return ms;
 }
}
