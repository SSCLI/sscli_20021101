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
using System.IO;
using System.Collections;
using System.Globalization;
using System.Threading;
public class Co5584BeginRead
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "File.BeginRead(Byte[], Int32, Int32, AsyncCallback, Object";
    public static String s_strTFName        = "co5584beginread.cs";
    public static String s_strTFAbbrev      = "co5584";
    public static String s_strTFPath        = Environment.CurrentDirectory;
    Boolean bCallBackCalled = false;
    int iCountErrors = 0;
    int iCountTestcases = 0;
    int[] iArrInvalidValues = new Int32[]{ -1, -2, -100, -1000, -10000, -100000, -1000000, -10000000, -100000000, -1000000000, Int32.MinValue};
    int[] iArrLargeValues = new Int32[]{ Int32.MaxValue, Int32.MaxValue/2, Int32.MaxValue/10, 10000, 100000 , Int32.MaxValue/20, Int32.MaxValue/100 , Int32.MaxValue/1000 };
    public void CallBackMethod(IAsyncResult iar) 
    {
        bCallBackCalled = true;
        iCountTestcases++;
        if(iar.IsCompleted == false) 
        {
            iCountErrors++;
            printerr( "Error_998t7! Callback called before operation was completed");
        }
    }
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + "\\" + s_strTFName + " , for " + s_strClassMethod + " , Source ver " + s_strDtTmVer);
        String strLoc = "Loc_000oo";
        String strValue = String.Empty;
        try
        {
            FileStream fs2;
            Byte[] bArr, bResultArr;
            String fileName = s_strTFAbbrev+"TestFile.tmp";
            AsyncCallback ascb;
            IAsyncResult iar;
            fs2 = new FileStream(fileName, FileMode.Create);
            bArr = new Byte[26];
            for(int i = 0 ; i < 26 ; i++)
                bArr[i] = (Byte)((i%26)+65);
            fs2.Flush();
            fs2.Close();
            strLoc = "Loc_7882h";
            fs2 = new FileStream(fileName, FileMode.Open);
            iCountTestcases++;
            try 
            {
                fs2.BeginRead(null, 0, 0, null, null);
                iCountErrors++;
                printerr( "Error_18983! Expected exception not thrown");
            } 
            catch ( ArgumentNullException aexc) 
            {
                printinfo( "Info_989su! Caught expected exception, aexc=="+aexc.Message);
            } 
            catch (Exception exc) 
            {
                iCountErrors++;
                printerr( "Error_9t898! Incorrect exception thrown, exc=="+exc.ToString());
            }
            fs2.Close();
            strLoc = "Loc_1298x";
            for(int iLoop = 0 ; iLoop < iArrInvalidValues.Length ; iLoop++)
            {
                fs2 = new FileStream(fileName, FileMode.Open);
                iCountTestcases++;
                bArr = new Byte[]{0,1,2,3};
                try 
                {
                    fs2.BeginRead(bArr, iArrInvalidValues[iLoop], 0, null, null);
                    iCountErrors++;
                    printerr("Error_298tk! Expected exception not thrown");
                } 
                catch (ArgumentOutOfRangeException aexc) 
                {
                    printinfo ( "Info_9888b! Caught expected exception, aexc=="+aexc.Message);
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_28g8b! Incorrect exception thrown, exc=="+exc.ToString());
                }
                fs2.Close();
            }
            strLoc = "Loc_77f8h";
            for(int iLoop = 0 ; iLoop < iArrLargeValues.Length ; iLoop++)
            {
                fs2 = new FileStream(fileName, FileMode.Open);
                iCountTestcases++;
                bArr = new Byte[]{0,1,2,3};
                try 
                {
                    fs2.BeginRead(bArr, iArrLargeValues[iLoop], 0, null, null);
                    iCountErrors++;
                    printerr( "Error_9t8g8! Expected exception not thrown");
                } 
                catch (ArgumentException aexc) 
                {
                    printinfo( "Info_t87gy! Caught expected exception, aexc=="+aexc.Message);
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_29y8g! Incorrect exception thrown, exc=="+exc.ToString());
                }
                fs2.Close();
            }
            strLoc = "Loc_89t8y";
            for(int iLoop = 0 ; iLoop < iArrInvalidValues.Length ; iLoop++)
            {
                fs2 = new FileStream(fileName, FileMode.Open);
                iCountTestcases++;
                bArr = new Byte[]{0,1};
                try 
                {
                    fs2.BeginRead(bArr, 0, iArrInvalidValues[iLoop], null, null);
                    iCountErrors++;
                    printerr( "Error_2868b! Expected exception not thrown");
                } 
                catch (ArgumentOutOfRangeException aexc) 
                {
                    printinfo( "Info_3388g! Caught expected exception, aexc=="+aexc.Message);
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_t958g! Incorrect exception thrown, exc=="+exc.ToString());
                }
                fs2.Close();
            }
            strLoc = "Loc_399il";
            for(int iLoop = 0 ; iLoop < iArrLargeValues.Length ; iLoop++)
            {
                fs2 = new FileStream(fileName, FileMode.Open);
                iCountTestcases++;
                bArr = new Byte[]{0,1};
                try 
                {
                    fs2.BeginRead(bArr, 0, iArrLargeValues[iLoop], null, null);
                    iCountErrors++;
                    printerr( "Error_5520j! Expected exception not thrown");
                } 
                catch (ArgumentException aexc) 
                {
                    printinfo( "Info_88tum! Caught expected exception, aexc=="+aexc.Message);
                } 
                catch (Exception exc) 
                {
                    iCountErrors++;
                    printerr( "Error_2090t! Incorrect exception thrown, exc=="+exc.ToString());
                }
                fs2.Close();
            }
            strLoc = "Loc_388hj";
            fs2 = new FileStream(fileName, FileMode.Open);
            ascb = new AsyncCallback(this.CallBackMethod);
            bCallBackCalled = false;
            bArr = new Byte[]{0,1,2,3};
            iar = fs2.BeginWrite(bArr, 0, 4, ascb, 5);
            fs2.EndWrite(iar);
            Thread.Sleep(1000);
            iCountTestcases++;
            if(!bCallBackCalled) 
            {
                iCountErrors++;
                printerr( "Error_489xh! CallBackmethod not called");
            }
            iCountTestcases++;
            if(!iar.IsCompleted) 
            {
                iCountErrors++;
                printerr( "Error_29ycy! Completed not set correctly");
            }
            iCountTestcases++;
            if(!iar.CompletedSynchronously) 
            {
                iCountErrors++;
                printerr( "Error_8998c! Not done async");
            }
            iCountTestcases++;
            if((Int32)iar.AsyncState != 5) 
            {
                iCountErrors++;
                printerr( "Error_20hvb! Incorrect AsyncState");
            }
            fs2.Position = 0;
            bResultArr = new Byte[8];
            iar = fs2.BeginRead(bResultArr, 2, 3, ascb, 5);
            fs2.EndRead(iar);
            for(int i = 2 ; i < 5 ; i++) 
            {
                iCountTestcases++;
                if(bResultArr[i] != i-2) 
                {
                    iCountErrors++;
                    printerr( "Error_1888v! Expected=="+bArr[i-2]+", got=="+bResultArr[i]);
                }
            }
            fs2.Close();
            IAsyncResult iar2;
            strLoc = "Loc_98y8b";
            fs2 = new FileStream(fileName, FileMode.Create);
            ascb = new AsyncCallback(this.CallBackMethod);
            bCallBackCalled = false;
            bArr = new Byte[100000];
            bResultArr = new Byte[bArr.Length];
            for(int i = 0 ; i < bArr.Length ; i++) 
                bArr[i] = (Byte)((i%26)+65);
            iar = fs2.BeginWrite(bArr, 0, bArr.Length, ascb, null);
            Console.WriteLine( bArr.Length );
            fs2.EndWrite(iar);
            if(fs2.Length != bArr.Length) 
            {
                iCountErrors++;
                printerr( "Error_98yxh! Incorrect number of chars written :: " + fs2.Length + " expected " + bArr.Length);
                }
            fs2.Position = 0;
            iar2 = fs2.BeginRead(bResultArr, 0, bResultArr.Length, null, null);
            Console.WriteLine( bArr.Length );     
            int iByteCount = fs2.EndRead(iar2) ;                   
            if( iByteCount != bResultArr.Length) 
            {
                iCountErrors++;
                printerr( "Error_875y7! Incorrect number of chars read" + iByteCount);
            }
            for(int i = 0 ; i < bResultArr.Length ; i++)
            {
                if(bResultArr[i] != bArr[i]) 
                {
                    iCountErrors++;
                    printerr( "Error_2439v! Expected=="+bArr[i]+" , got=="+bResultArr[i]);
                }
            }
            iCountTestcases++;
            if(!iar.CompletedSynchronously) 
            {
                iCountErrors++;
                printerr( "Error_90ud9! Not done async");
            }
            fs2.Close();
            File.Delete(fileName);
            strLoc = "Loc_98yay";
            fs2 = new FileStream(fileName, FileMode.Create);
            ascb = new AsyncCallback(this.CallBackMethod);
            bCallBackCalled = false;
            bArr = new Byte[2000];
            bResultArr = new Byte[500];
            for(int i = 0 ; i < bArr.Length ; i++) 
                bArr[i] = (Byte)((i%26)+65);
            iar = fs2.BeginWrite(bArr, 104, 104, ascb, null);
            fs2.EndWrite( iar );
            if(fs2.Length != 104) 
            {
                iCountErrors++;
                printerr( "ERror_298yh! Incorrect number of bytes written");
            }
            fs2.Position = 0;
            iar = fs2.BeginRead(bResultArr, 26, 104, null, null);
            if(fs2.EndRead(iar)  != 104) 
            {
                iCountErrors++;
                printerr( "ERror_49yxy! Incorrect number of byuytes read");
            }
            for(int i = 26; i < 130 ; i++) 
            {
                iCountTestcases++;
                if(bResultArr[i] != (Byte)((i%26)+65)) 
                {
                    iCountErrors++;
                    printerr( "Error_298vc! value=="+bResultArr[i]);
                }
            }
            fs2.Close();
            File.Delete(fileName);
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
            Console.WriteLine("FAiL! "+s_strTFName+" ,iCountErrors=="+iCountErrors.ToString()+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    public void printerr ( String err )
    {
        Console.WriteLine ("POINTTOBREAK: ("+ s_strTFAbbrev + ") "+ err);
    }
    public void printinfo ( String info )
    {
        Console.WriteLine ("INFO: ("+ s_strTFAbbrev + ") "+ info);
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co5584BeginRead cbA = new Co5584BeginRead();
        try 
        {
            bResult = cbA.runTest();
        } 
        catch (Exception exc_main)
        {
            bResult = false;
            Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main.ToString());
        }
        if (!bResult)
        {
            Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
            Console.WriteLine( " " );
            Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode = 0; else Environment.ExitCode = 1;
    }
}
