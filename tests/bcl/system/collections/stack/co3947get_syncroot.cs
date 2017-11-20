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
using System.Threading;
using System.IO;
using System.Collections;
public class Co3947get_SyncRoot
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "Stack.SyncRoot";
    public static String s_strTFName        = "Co3947get_SyncRoot.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    private Stack arrDaughter;
    private Stack arrGrandDaughter;
    const Int32 iNumberOfElements = 100;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        Stack arrSon;
        Stack arrMother;
        Thread[] workers;
        ThreadStart ts1;
        ThreadStart ts2;
        Int32 iNumberOfWorkers = 100;
        try 
        {
            do
            {
                strLoc = "Loc_8345vdfv";
                arrMother = new Stack();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    arrMother.Push(i);
                }
                arrSon = Stack.Synchronized(arrMother);
                arrGrandDaughter = Stack.Synchronized(arrSon);
                arrDaughter = Stack.Synchronized(arrMother);
                iCountTestcases++;
                if(arrSon.SyncRoot != arrMother.SyncRoot) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_234dnvf! Expected value not returned, " + (arrSon.SyncRoot==arrMother.SyncRoot));
                }
                iCountTestcases++;
                if(arrSon.SyncRoot!=arrMother) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_8753bdd! Expected value not returned, " + (arrSon.SyncRoot==arrMother));
                }
                iCountTestcases++;
                if(arrGrandDaughter.SyncRoot!=arrMother) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_4927fd0fd! Expected value not returned, " + (arrGrandDaughter.SyncRoot==arrMother));
                }
                iCountTestcases++;
                if(arrDaughter.SyncRoot!=arrMother) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_85390gfg! Expected value not returned, " + (arrDaughter.SyncRoot==arrMother));
                }
                iCountTestcases++;
                if(arrSon.SyncRoot != arrMother.SyncRoot) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_234dnvf! Expected value not returned, " + (arrSon.SyncRoot==arrMother.SyncRoot));
                }
                strLoc = "Loc_845230fgdg";
                workers = new Thread[iNumberOfWorkers];
                ts1 = new ThreadStart(SortElements);
                ts2 = new ThreadStart(ReverseElements);
                for(int iThreads=0; iThreads<iNumberOfWorkers;iThreads+=2)
                {
                    strLoc = "Loc_74329fd_" + iThreads;
                    workers[iThreads] = new Thread(ts1);
                    workers[iThreads].Name = "Thread worker " + iThreads;
                    workers[iThreads+1] = new Thread(ts2);
                    workers[iThreads+1].Name = "Thread worker " + iThreads + 1;
                    workers[iThreads].Start();
                    workers[iThreads+1].Start();
                }
                for(int iThreads=0; iThreads<iNumberOfWorkers;iThreads+=2)
                    workers[iThreads].Join();
                strLoc = "Loc_7539vfdg";
            } while (false);
        } 
        catch (Exception exc_general ) 
        {
            ++iCountErrors;
            Console.WriteLine (s_strTFAbbrev + " : Error Err_8888yyy!  strLoc=="+ strLoc +", exc_general==\n"+exc_general.ToString());
        }
        if ( iCountErrors == 0 )
        {
            Console.WriteLine( "paSs.   "+s_strTFPath +" "+s_strTFName+" ,iCountTestcases=="+iCountTestcases);
            return true;
        }
        else
        {
            Console.WriteLine("FAiL!   "+s_strTFPath+" "+s_strTFName+" ,iCountErrors=="+iCountErrors+" , BugNums?: "+s_strActiveBugNums );
            return false;
        }
    }
    void SortElements()
    {
        arrGrandDaughter.Clear();
        for(int i=0; i<iNumberOfElements;i++)
        {
            arrGrandDaughter.Push(i);
        }
    }
    void ReverseElements()
    {
        arrDaughter.Clear();
        for(int i=0; i<iNumberOfElements;i++)
        {
            arrDaughter.Push(i);
        }
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3947get_SyncRoot cbA = new Co3947get_SyncRoot();
        try 
        {
            bResult = cbA.runTest();
        } 
        catch (Exception exc_main)
        {
            bResult = false;
            Console.WriteLine(s_strTFAbbrev + " : FAiL! Error Err_9999zzz! Uncaught Exception in main(), exc_main=="+exc_main);
        }
        if (!bResult)
        {
            Console.WriteLine ("Path: "+s_strTFName + s_strTFPath);
            Console.WriteLine( " " );
            Console.WriteLine( "FAiL!  "+ s_strTFAbbrev);
            Console.WriteLine( " " );
        }
        if (bResult) Environment.ExitCode=0; else Environment.ExitCode=1;
    }
}
internal class Foo 
{
    internal String iValue;
    internal Foo()
    {
        iValue = "Hello World";
    }
}
