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
using System.Runtime.Serialization.Formatters.Binary;
public class Co3922Synchronized
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "Hashtable.Synchronized(Hashtable)";
    public static String s_strTFName        = "Co3922Synchronized";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    private Hashtable hsh2;
    private Int32 iNumberOfElements = 20;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        Hashtable hsh1;
        String strValue;
        Thread[] workers;
        ThreadStart ts1;
        Int32 iNumberOfWorkers = 15;
        Boolean fLoopExit;
        DictionaryEntry[] strValueArr;
        String[] strKeyArr;
        Hashtable hsh3;
        Hashtable hsh4;
        IDictionaryEnumerator idic;
        MemoryStream ms1;
        Boolean fPass;
        Object oValue;
        try 
        {
            do
            {
                hsh1 = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    hsh1.Add("Key_" + i, "Value_" + i);
                }
                hsh2 = Hashtable.Synchronized(hsh1);
                fPass = true;
                iCountTestcases++;
                if(hsh2.Count != hsh1.Count) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_742dsf! Expected value not returned, " + hsh2.Count);
                }
                for(int i=0; i<iNumberOfElements; i++)
                {
                    if(!((String)hsh2["Key_" + i]).Equals("Value_" + i))
                    {
                        Console.WriteLine(hsh2["Key_" + i]);
                        fPass = false;
                    }
                }
                try
                {
                    oValue = hsh2[null];
                    fPass = false;
                }
                catch(ArgumentNullException)
                {
                }
                catch(Exception)
                {
                    fPass = false;
                }
                hsh2.Clear();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    hsh2["Key_" + i] =  "Value_" + i;
                }
                if(!fPass)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_752dsgf! Oh man! This is busted!!!!");
                }
                strValueArr = new DictionaryEntry[hsh2.Count];
                hsh2.CopyTo(strValueArr, 0);
                hsh3 = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    if(!hsh2.Contains("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742ds8f! Expected value not returned, " + hsh2.Contains("Key_" + i));
                    }				
                    if(!hsh2.ContainsKey("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742389dsaf! Expected value not returned, " + hsh2.ContainsKey("Key_" + i));
                    }				
                    if(!hsh2.ContainsValue("Value_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_563fgd! Expected value not returned, " + hsh2.ContainsValue("Value_" + i));
                    }				
                    if(!hsh1.ContainsValue(((DictionaryEntry)strValueArr[i]).Value)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_87429dsfd! Expected value not returned, " + ((DictionaryEntry)strValueArr[i]).Value);
                    }				
                    try
                    {
                        hsh3.Add(strValueArr[i], null);
                    }
                    catch(Exception)
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_74298dsd! Exception thrown for  " + strValueArr[i]);
                    }
                }
                hsh4 = (Hashtable)hsh2.Clone();
                if(hsh4.Count != hsh1.Count) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_342342! Expected value not returned, " + hsh4.Count);
                }				
                strValueArr = new DictionaryEntry[hsh4.Count];
                hsh4.CopyTo(strValueArr, 0);
                hsh3 = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    if(!hsh4.Contains("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742ds8f! Expected value not returned, " + hsh4.Contains("Key_" + i));
                    }				
                    if(!hsh4.ContainsKey("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742389dsaf! Expected value not returned, " + hsh4.ContainsKey("Key_" + i));
                    }				
                    if(!hsh4.ContainsValue("Value_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_6-4142dsf! Expected value not returned, " + hsh4.ContainsValue("Value_" + i));
                    }				
                    if(!hsh1.ContainsValue(((DictionaryEntry)strValueArr[i]).Value)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_87429dsfd! Expected value not returned, " + ((DictionaryEntry)strValueArr[i]).Value);
                    }				
                    try
                    {
                        hsh3.Add(((DictionaryEntry)strValueArr[i]).Value, null);
                    }
                    catch(Exception)
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_74298dsd! Exception thrown for  " + ((DictionaryEntry)strValueArr[i]).Value);
                    }
                }
                if(hsh4.IsReadOnly) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh4.IsReadOnly);
                }
                if(!hsh4.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh4.IsSynchronized);
                }
                idic = hsh2.GetEnumerator();
                hsh3 = new Hashtable();
                hsh4 = new Hashtable();
            while(idic.MoveNext())
            {
                if(!hsh2.ContainsKey(idic.Key)) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_4532sfds! Expected value not returned");
                }				
                if(!hsh2.ContainsValue(idic.Value)) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_682wm! Expected value not returned");
                }				
                try
                {
                    hsh3.Add(idic.Key, null);
                }
                catch(Exception)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_5243sfd! Exception thrown for  " + idic.Key);
                }
                try
                {
                    hsh4.Add(idic.Value, null);
                }
                catch(Exception)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_25sfs! Exception thrown for  " + idic.Value);
                }
            }
                BinaryFormatter formatter = new BinaryFormatter();
                ms1 = new MemoryStream();
                formatter.Serialize(ms1, hsh2);
                ms1.Position = 0;
                hsh4 = (Hashtable)formatter.Deserialize(ms1);
                if(hsh4.Count != hsh1.Count) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_072xsf! Expected value not returned, " + hsh4.Count);
                }				
                strValueArr = new DictionaryEntry[hsh4.Count];
                hsh4.CopyTo(strValueArr, 0);
                hsh3 = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    if(!hsh4.Contains("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742ds8f! Expected value not returned, " + hsh4.Contains("Key_" + i));
                    }				
                    if(!hsh4.ContainsKey("Key_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_742389dsaf! Expected value not returned, " + hsh4.ContainsKey("Key_" + i));
                    }				
                    if(!hsh4.ContainsValue("Value_" + i)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_0672esfs! Expected value not returned, " + hsh4.ContainsValue("Value_" + i));
                    }				
                    if(!hsh1.ContainsValue(((DictionaryEntry)strValueArr[i]).Value)) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_87429dsfd! Expected value not returned, " + ((DictionaryEntry)strValueArr[i]).Value);
                    }				
                    try
                    {
                        hsh3.Add(((DictionaryEntry)strValueArr[i]).Value, null);
                    }
                    catch(Exception)
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_74298dsd! Exception thrown for  " + strValueArr[i]);
                    }
                }
                if(hsh4.IsReadOnly) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh4.IsReadOnly);
                }
                if(!hsh4.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh4.IsSynchronized);
                }
                if(hsh2.IsReadOnly) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh2.IsReadOnly);
                }
                if(!hsh2.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh2.IsSynchronized);
                }
                if(hsh2.SyncRoot != hsh1) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_7428dsafd! Expected value not returned, ");
                }
                String[] strValueArr11 = new String[hsh1.Count];
                strKeyArr = new String[hsh1.Count];
                hsh2.Keys.CopyTo(strKeyArr, 0);
                hsh2.Values.CopyTo(strValueArr11, 0);
                hsh3 = new Hashtable();
                hsh4 = new Hashtable();
                for(int i=0; i<iNumberOfElements; i++)
                {
                    if(!hsh2.ContainsKey(strKeyArr[i])) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_4532sfds! Expected value not returned");
                    }				
                    if(!hsh2.ContainsValue(strValueArr11[i])) 
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_074dsd! Expected value not returned, " + strValueArr11[i]);
                    }				
                    try
                    {
                        hsh3.Add(strKeyArr[i], null);
                    }
                    catch(Exception)
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_5243sfd! Exception thrown for  " + idic.Key);
                    }
                    try
                    {
                        hsh4.Add(strValueArr11[i], null);
                    }
                    catch(Exception)
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_25sfs! Exception thrown for  " + idic.Value);
                    }
                }
                hsh2.Remove("Key_1");
                if(hsh2.ContainsKey("Key_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_423ewd! Expected value not returned, ");
                }				
                if(hsh2.ContainsValue("Value_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_64q213d! Expected value not returned, ");
                }				
                hsh2.Add("Key_1", "Value_1");
                if(!hsh2.ContainsKey("Key_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_423ewd! Expected value not returned, ");
                }				
                if(!hsh2.ContainsValue("Value_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_74523esf! Expected value not returned, ");
                }				
                hsh2["Key_1"] = "Value_Modified_1";
                if(!hsh2.ContainsKey("Key_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_423ewd! Expected value not returned, ");
                }				
                if(hsh2.ContainsValue("Value_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_74523esf! Expected value not returned, ");
                }				
                if(!hsh2.ContainsValue("Value_Modified_1")) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_342fs! Expected value not returned, ");
                }		
                hsh3 = Hashtable.Synchronized(hsh2);
                if(hsh3.Count != hsh1.Count) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_742dsf! Expected value not returned, " + hsh3.Count);
                }				
                if(!hsh3.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_723sadf! Expected value not returned, " + hsh3.IsSynchronized);
                }
                hsh2.Clear();		
                if(hsh2.Count != 0) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_742dsf! Expected value not returned, " + hsh2.Count);
                }				
                strLoc = "Loc_8345vdfv";
                hsh1 = new Hashtable();
                hsh2 = Hashtable.Synchronized(hsh1);
                workers = new Thread[iNumberOfWorkers];
                ts1 = new ThreadStart(AddElements);
                for(int i=0; i<workers.Length; i++)
                {
                    workers[i] = new Thread(ts1);
                    workers[i].Name = "Thread worker " + i;
                    workers[i].Start();
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
                iCountTestcases++;
                if(hsh2.Count != iNumberOfElements*iNumberOfWorkers) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_75630fvbdf! Expected value not returned, " + hsh2.Count);
                }
                iCountTestcases++;
                for(int i=0; i<iNumberOfWorkers; i++)
                {
                    for(int j=0; j<iNumberOfElements; j++)
                    {
                        strValue = "Thread worker " + i + "_" + j;
                        if(!hsh2.Contains(strValue))
                        {
                            iCountErrors++;
                            Console.WriteLine("Err_452dvdf_" + i + "_" + j + "! Expected value not returned, " + strValue);
                        }
                    }
                }
                workers = new Thread[iNumberOfWorkers];
                ts1 = new ThreadStart(RemoveElements);
                for(int i=0; i<workers.Length; i++)
                {
                    workers[i] = new Thread(ts1);
                    workers[i].Name = "Thread worker " + i;
                    workers[i].Start();
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
                iCountTestcases++;
                if(hsh2.Count != 0) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_6720fvdg! Expected value not returned, " + hsh2.Count);
                }
                iCountTestcases++;
                if(hsh1.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_4820fdf! Expected value not returned, " + hsh1.IsSynchronized);
                }
                iCountTestcases++;
                if(!hsh2.IsSynchronized) 
                {
                    iCountErrors++;
                    Console.WriteLine("Err_4820fdf! Expected value not returned, " + hsh2.IsSynchronized);
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
    void AddElements()
    {
        Thread thrd1 = Thread.CurrentThread;
        String strName = thrd1.Name;
        for(int i=0;i<iNumberOfElements;i++)
        {
            hsh2.Add(strName + "_" + i, "String_" + i);
        }
    }
    void RemoveElements()
    {
        Thread thrd1 = Thread.CurrentThread;
        String strName = thrd1.Name;
        for(int i=0;i<iNumberOfElements;i++)
        {
            hsh2.Remove(strName + "_" + i);
        }
    }
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3922Synchronized cbA = new Co3922Synchronized();
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
    internal String strValue;
    internal Foo()
    {
        strValue = "Hello World";
    }
}
