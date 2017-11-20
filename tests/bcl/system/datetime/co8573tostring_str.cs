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
using System.Collections;
using System.Globalization;
using System.IO;
using System.Text;
public class Co8573ToString_str
{
 public static String s_strActiveBugNums = "";
 public static String s_strDtTmVer       = "";
 public static String s_strClassMethod   = "DateTime.ToString(String)";
 public static String s_strTFName        = "Co8573ToString_str.cs";
 public static String s_strTFAbbrev      = s_strTFName.Substring(0,6);
 public static String s_strTFPath        = Environment.CurrentDirectory;
 public bool runTest()
   {
   Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
   String strLoc = "Loc_000oo";
   String strValue = String.Empty;
   int iCountErrors = 0;
   int iCountTestcases = 0;
   DateTime date;
   String returnedValue;
   String formatString;
   String expectedValue;
   String[] formalFormats = {"d",		
			     "D",	
			     "f",	
			     "F",	
			     "g",	
			     "G",	
			     "m",	
			     "M",	
			     "r",	
			     "R",	
			     "s",	
			     "t",	
			     "T",	
			     "u",	
			     "y",	
			     "Y",	
   };
   String[] expectedFormalValues = {
     "4/18/2001",
     "Wednesday, April 18, 2001",
     "Wednesday, April 18, 2001 9:41 AM",
     "Wednesday, April 18, 2001 9:41:26 AM",
     "4/18/2001 9:41 AM",
     "4/18/2001 9:41:26 AM",
     "April 18",
     "April 18",
     "Wed, 18 Apr 2001 09:41:26 GMT",
     "Wed, 18 Apr 2001 09:41:26 GMT",
     "2001-04-18T09:41:26",
     "9:41 AM",
     "9:41:26 AM",
     "2001-04-18 09:41:26Z",
     "April, 2001",
     "April, 2001",
   };
   String[] dateInfoCustomFormats;
   String[] dateinfoExpectedCustomValues = {
     "Wed, 18 Apr 2001 09:41:26 GMT",	
     "Wed, 18 Apr 2001 09:41:26 GMT",	
     "Wednesday, 18 April 2001",			
     "Wednesday, 18 April 2001 9:41",	
     "Wednesday, 18 April 2001 9:41 AM",	
     "Wednesday, 18 April 2001 09:41",	
     "Wednesday, 18 April 2001 09:41 AM",	
     "Wednesday, 18 April 2001 09:41:26",	
     "Wednesday, 18 April 2001 09:41:26",	
     "9:41",	
     "9:41 AM",	
     "09:41",	
     "09:41 AM",	
     "09:41:26",	
     "04/18/2001",	
     "04/18/2001 9:41",	
     "04/18/2001 9:41 AM",	
     "04/18/2001 09:41",	
     "04/18/2001 09:41 AM",	
     "04/18/2001 09:41:26",	
     "April 18",	
     "April 18",	
     "2001 April",	
     "2001 April",	
     "2001-04-18 09:41:26Z",	
     "2001-04-18T09:41:26",	
   };
   String[] ourOwnCustomFormats = {
     "dddd",
     "ddd",
     "dd",
     "ddd, dd",
     "dddd, ddd, dd",
     "dddd ddd dd",
     "dddd~ddd~dd",
     "DD",	
     "ddddDDdddDDddD",	
     "dddddddddddd",	
     "MMMM",
     "MMM",
     "MM",
     "MMMM~MMM~MM",
     "MMMMddddMMMdddMMdd",
     "yyyy",
     "yyy",
     "yy",
     "yyyyyyyyyy",
     "HHHH",
     "HHH",
     "HH",
     "mmmm",
     "mmm",
     "mm",
     "ssss",
     "sss",
     "ss",
   };
   String[] ourOwnExpectedCustomValues = {
     "Wednesday",
     "Wed",
     "18",
     "Wed, 18",
     "Wednesday, Wed, 18",
     "Wednesday Wed 18",
     "Wednesday~Wed~18",
     "DD",
     "WednesdayDDWedDD18D",
     "Wednesday",
     "April",
     "Apr",
     "04",
     "April~Apr~04",
     "AprilWednesdayAprWed0418",
     "2001",
     "2001",
     "01",
     "0000002001",
     "09",
     "09",
     "09",
     "41",
     "41",
     "41",
     "26",
     "26",
     "26",
   };
   String[] exceptionValues = {
     "a", "A", "b", "B",  "c",  "C",  "e",  "E",  "h",  "H",  "i",  "I",  "j",  "J",  "k",  "K",  
     "l",  "L",  "n",  "N",  "o",  "O",  "p", 
     "q",  "Q",  "S",  "v",  "V", 
     "w",  "W",  "x",  "X",  "z",  "Z",
     ":",  ",",  "/",  "\\",  " ",  "-", "%", "*",
   };
   SortedList table;
   Random random;
   Int32 numberOfRounds;
   Int32 numberOfStringConcats;
   StringBuilder builder1;
   StringBuilder builder2;
   Int32 i1;
   String previous;
   String temp;
   String temp1;
   Boolean getOut;
   try
     {
     strLoc = "Loc_97345sdg";
     iCountTestcases++;
     date = new DateTime(2001, 4, 18, 9, 41, 26, 980);
     for(int i=0; i<formalFormats.Length; i++){
     returnedValue = date.ToString(formalFormats[i]);
     if(!returnedValue.Equals(expectedFormalValues[i])){
     iCountErrors++;
     Console.WriteLine("Err_9347tsdgs_{3}! Format: {0}, Returned: {1}, Expected: {2}", formalFormats[i], returnedValue, expectedFormalValues[i], i);
     }
     }
     strLoc = "Loc_8346tdgs";
     iCountTestcases++;
     dateInfoCustomFormats = CultureInfo.InvariantCulture.DateTimeFormat.GetAllDateTimePatterns();
     Array.Sort(dateInfoCustomFormats);
     for(int i=0; i<dateInfoCustomFormats.Length; i++){
     returnedValue = date.ToString(dateInfoCustomFormats[i]);
     if(!returnedValue.Equals(dateinfoExpectedCustomValues[i])){
     iCountErrors++;
     Console.WriteLine("Err_027y43rsdg_{3}! Format: {0}, Returned: {1}, Expected: {2}", dateInfoCustomFormats[i], returnedValue, dateinfoExpectedCustomValues[i], i);
     }
     }
     strLoc = "Loc_398dgds";
     iCountTestcases++;
     for(int i=0; i<ourOwnCustomFormats.Length; i++){
     returnedValue = date.ToString(ourOwnCustomFormats[i]);
     if(!returnedValue.Equals(ourOwnExpectedCustomValues[i])){
     iCountErrors++;
     Console.WriteLine("Err_845sdg_{3}! Format: {0}, Returned: {1}, Expected: {2}", ourOwnCustomFormats[i], returnedValue, ourOwnExpectedCustomValues[i], i);
     }
     }
     strLoc = "Loc_97345sdg";
     iCountTestcases++;			
     table = new SortedList();
     table.Add("yy", "01");
     table.Add("yyy", "2001");
     table.Add("yyyy", "2001");
     table.Add("yyyyyy", "002001");
     table.Add("MM", "04");
     table.Add("MMM", "Apr");
     table.Add("MMMM", "April");
     table.Add("MMMMMMM", "April");
     table.Add("dd", "18");
     table.Add("ddd", "Wed");
     table.Add("dddd", "Wednesday");
     table.Add("ddddddd", "Wednesday");
     table.Add("HH", "09");
     table.Add("HHH", "09");
     table.Add("HHHH", "09");
     table.Add("HHHHHHH", "09");
     table.Add("mm", "41");
     table.Add("mmm", "41");
     table.Add("mmmm", "41");
     table.Add("mmmmmmmmm", "41");
     table.Add("ss", "26");
     table.Add("sss", "26");
     table.Add("ssss", "26");
     table.Add("sssss", "26");
     table.Add("Lak", "Lak");
     table.Add(":", ":");
     table.Add(",", ",");
     table.Add("/", "/");
     table.Add("-", "-");
     table.Add(" ", " ");
     table.Add("~", "~");
     table.Add("   ", "   ");
     random = new Random();
     numberOfRounds = random.Next(100, 10000);
     formatString = String.Empty;expectedValue = String.Empty;
     for(int i=0; i<numberOfRounds; i++){
     returnedValue=String.Empty;
     try{
     numberOfStringConcats = random.Next(1, 20);
     builder1 = new StringBuilder();
     builder2 = new StringBuilder();
     previous = String.Empty;
     for(int j=0; j<numberOfStringConcats; j++){
     do{
     i1 = random.Next(table.Count);
     temp = (String)table.GetKey(i1);
     getOut = true;
     if(j!=0){
     if(temp.Length>previous.Length){
     if(temp.EndsWith(previous))
       getOut = false;
     }else{
     if(previous.EndsWith(temp))
       getOut = false;
     }
     }
     }while(!getOut);
     previous = temp;
     builder1.Append(temp);
     temp1 = (String)table.GetByIndex(i1);
     builder2.Append(temp1);
     if((String)table[temp] != temp1)
       throw new Exception("Loc_7432edgs! What is happening here? " + temp + " " + temp1);
     }
     formatString = builder1.ToString();
     if(formatString.Length==1){
     i--;
     continue;
     }
     expectedValue = builder2.ToString();
     returnedValue = date.ToString(formatString);
     if(!returnedValue.Equals(expectedValue)){
     iCountErrors++;
     Console.WriteLine("Err_87sdg! Format: {0}, Returned: {1}, Expected: {2}", formatString, returnedValue, expectedValue);
     }
     }catch(Exception){
     Console.WriteLine("Err_EDfdfd! Exception thrown, Format: {0}, Returned: {1}, Expected: {2}", formatString, returnedValue, expectedValue);					
     }
     }
     strLoc = "Loc_97345sdg";
     iCountTestcases++;
     formatString = null;
     returnedValue = date.ToString(formatString);
     if(!returnedValue.Equals(expectedFormalValues[5])){
     iCountErrors++;
     Console.WriteLine("Err_753gfs! Unexpcted value returned, " + returnedValue);
     }
     strLoc = "Loc_34975sgd";
     iCountTestcases++;
     formatString = String.Empty;
     returnedValue = date.ToString(formatString);
     if(!returnedValue.Equals(expectedFormalValues[5])){
     iCountErrors++;
     Console.WriteLine("Err_3249dsg! Unexpcted value returned, " + returnedValue);
     }
     strLoc = "Loc_34927sdg";
     iCountTestcases++;
     for(int i=0; i<exceptionValues.Length; i++){
     try {
     returnedValue = date.ToString(exceptionValues[i]);
     iCountErrors++;
     Console.WriteLine("Err_9347sdg_" + i + "! Exception now thrown, " + returnedValue);
     }
     catch(FormatException){}
     catch(Exception ex){
     iCountErrors++;
     Console.WriteLine("Err_3457dsg_" + i + "! Unexpected Exception thrown " + ex.GetType().Name);
     }
     }
     iCountTestcases++;
     } 
   catch (Exception exc_general ) 
     {
     ++iCountErrors;
     Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general=="+exc_general.ToString());
     }
   if ( iCountErrors == 0 )
     {
     Console.WriteLine( "paSs. "+s_strTFName+" ,iCountTestcases=="+iCountTestcases.ToString());
     return true;
     }
   else
     {
     Console.WriteLine("FAiL! "+s_strTFName+" ,inCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
     return false;
     }
   }
 public static void Main(String[] args)
   {
   bool bResult = false;
   Co8573ToString_str cbA = new Co8573ToString_str();
   try {
   bResult = cbA.runTest();
   } catch (Exception exc_main){
   bResult = false;
   Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main.ToString());
   }
   if (!bResult)
     {
     Console.WriteLine ("Path: "+s_strTFPath+"\\"+s_strTFName);
     Console.WriteLine( " " );
     Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
     Console.WriteLine( " " );
     }
   if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
   }
}
