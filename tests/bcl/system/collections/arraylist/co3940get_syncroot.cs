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
public class Co3940get_SyncRoot
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "ArrayList.SyncRoot";
    public static String s_strTFName        = "Co3940get_SyncRoot.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    private ArrayList arrDaughter;
    private ArrayList arrGrandDaughter;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        ArrayList arrSon;
        ArrayList arrMother;
        Int32 iNumberOfElements = 100;
        Int32 iValue;
        Boolean fDescending;
        Boolean fWrongResult;
        Thread[] workers;
        ThreadStart ts1;
        ThreadStart ts2;
        Int32 iNumberOfWorkers = 100;
        Boolean fLoopExit;
        try 
        {
            do
            {
                strLoc = "Loc_8345vdfv";
                arrMother = new ArrayList();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    arrMother.Add(i);
                }
                arrSon = ArrayList.FixedSize(arrMother);
                arrGrandDaughter = ArrayList.Synchronized(arrSon);
                arrDaughter = ArrayList.Synchronized(arrMother);
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
            while(true)
            {
                fLoopExit=false;
                for(int i=0; i<iNumberOfWorkers;i++)
                {
                    if(workers[i].IsAlive)
                        fLoopExit=true;
                }
                if(!fLoopExit)
                    break;
            }
                strLoc = "Loc_7539vfdg";
                fDescending=false;
                if(((Int32)arrMother[0]).CompareTo((Int32)arrMother[1])>0)
                    fDescending=true;
                fWrongResult=false;
                iValue = (Int32)arrMother[0];
                iCountTestcases++;
                for(int i=1;i<iNumberOfElements;i++)
                {
                    if(fDescending)
                    {
                        if(iValue.CompareTo((Int32)arrMother[i])<=0)
                        {
                            Console.WriteLine(i + " " + iValue + " " + (Int32)arrMother[i]);
                            fWrongResult=true;
                            break;
                        }
                    }
                    else
                    {
                        if(iValue.CompareTo((Int32)arrMother[i])>=0)
                        {
                            fWrongResult=true;
                            break;
                        }
                    }
                    iValue=(Int32)arrMother[i];
                }
                if(fWrongResult) 
                {
                    iCountErrors++;
                    Console.WriteLine("fDescending, " + fDescending);
                    for(int i=0; i<iNumberOfElements; i++)
                    {
                        Console.WriteLine(arrMother[i]);
                    }
                    Console.WriteLine("Err_8420fd! Expected value not returned");
                }
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
        arrGrandDaughter.Sort();
    }
    void ReverseElements()
    {
        arrDaughter.Reverse();
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3940get_SyncRoot cbA = new Co3940get_SyncRoot();
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
