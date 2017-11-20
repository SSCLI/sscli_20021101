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
public class Co3944get_SyncRoot
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "Hashtable.SyncRoot";
    public static String s_strTFName        = "Co3944get_SyncRoot.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    private Hashtable arrDaughter;
    private Hashtable arrGrandDaughter;
    private Int32 iNumberOfElements = 100;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        Hashtable arrSon;
        Hashtable arrMother;
        Thread[] workers;
        ThreadStart ts1;
        ThreadStart ts2;
        Int32 iNumberOfWorkers = 30;
        Boolean fLoopExit;
        Hashtable hshPossibleValues;
        IDictionaryEnumerator idic;
        Hashtable hsh1;
        Hashtable hsh2;
        Hashtable hsh3;
        try 
        {
            do
            {
                hsh1 = new Hashtable();
                hsh2 = new Hashtable();
                iCountTestcases++;
                if(hsh1.SyncRoot == hsh2.SyncRoot) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_47235fsd! Expected value not returned, ");
                }
                hsh1 = new Hashtable();
                hsh2 = Hashtable.Synchronized(hsh1);
                hsh3 = (Hashtable)hsh2.Clone();
                iCountTestcases++;
                if(hsh2.SyncRoot == hsh3.SyncRoot) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_47235fsd! Expected value not returned, ");
                }
                if(hsh1.SyncRoot == hsh3.SyncRoot) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_47235fsd! Expected value not returned, ");
                }
                strLoc = "Loc_8345vdfv";
                arrMother = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    arrMother.Add("Key_" + i, "Value_" + i);
                }
                arrSon = Hashtable.Synchronized(arrMother);
                arrGrandDaughter = Hashtable.Synchronized(arrSon);
                arrDaughter = Hashtable.Synchronized(arrMother);
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
                    Console.WriteLine("Err_8230fvbd! Expected value not returned, " + (arrSon.SyncRoot==arrMother));
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
                ts1 = new ThreadStart(AddMoreElements);
                ts2 = new ThreadStart(RemoveElements);
                for(int iThreads=0; iThreads<iNumberOfWorkers;iThreads+=2)
                {
                    strLoc = "Loc_74329fd_" + iThreads;
                    workers[iThreads] = new Thread(ts1);
                    workers[iThreads].Name = "Thread_worker_" + iThreads;
                    workers[iThreads+1] = new Thread(ts2);
                    workers[iThreads+1].Name = "Thread_worker_" + iThreads + 1;
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
                iCountTestcases++;
                hshPossibleValues = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    hshPossibleValues.Add("Key_" + i, "Value_" + i);
                }
                for(int i=0; i<iNumberOfWorkers; i++)
                {
                    hshPossibleValues.Add("Key_Thread_worker_" + i, "Thread_worker_" + i);
                }
                if(arrMother.Count>0)
                    Console.WriteLine("Loc_7428fdsg! Adds have it");
                idic = arrMother.GetEnumerator();
            while(idic.MoveNext())
            {
                if(!hshPossibleValues.ContainsKey(idic.Key))
                {
                    iCountErrors++;
                    Console.WriteLine("Err_7429dsf! Expected value not returned, " + idic.Key);
                }
                if(!hshPossibleValues.ContainsValue(idic.Value))
                {
                    iCountErrors++;
                    Console.WriteLine("Err_2487dsf! Expected value not returned, " + idic.Value);
                }
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
    void AddMoreElements()
    {
        Thread thd = Thread.CurrentThread;
        try
        {
            arrGrandDaughter.Add("Key_" + thd.Name, thd.Name);
        }
        catch(Exception ex)
        {
            Console.WriteLine("Loc_4728fdg! Hmmm! this shouldn't happen, " + thd.Name + " " + ex);
        }
    }
    void RemoveElements()
    {
        arrDaughter.Clear();
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3944get_SyncRoot cbA = new Co3944get_SyncRoot();
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
