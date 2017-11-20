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
public class Co3996AllMethods
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "DictionaryBase.AllMethods";
    public static String s_strTFName        = "Co3996AllMethods.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        MyDictionaryBase mycol1;
        MyDictionaryBase mycol2;
        FooKey fk1;
        FooValue fv1;
        FooKey[] arrFK1;
        FooValue[] arrFV1;
        IDictionaryEnumerator enu1;
        Int32 iCount;
        DictionaryEntry[] arrDicEnt1;
        DictionaryEntry dicEnt1;
        Object obj1;
        try 
        {
            do
            {
                iCountTestcases++;
                iCountTestcases++;
                strLoc = "Loc_001oo";
                mycol1 = new MyDictionaryBase();
                if(mycol1.Count!=0)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_234dnvf! Expected value not returned, " + mycol1.Count);
                }
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                if(mycol1.Count!=100)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_2075sg! Expected value not returned, " + mycol1.Count);
                }
                iCountTestcases++;
                strLoc = "Loc_002oo";
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                arrDicEnt1 = new DictionaryEntry[100];
                mycol1.CopyTo(arrDicEnt1, 0);
                arrFK1 = new FooKey[100];
                arrFV1 = new FooValue[100];
                for(int i=0; i<100; i++)
                {
                    arrFK1[i] = (FooKey)arrDicEnt1[i].Key;
                    arrFV1[i] = (FooValue)arrDicEnt1[i].Value;
                }
                Array.Sort(arrFK1);
                Array.Sort(arrFV1);
                for(int i=0; i<100; i++)
                {
                    if((arrFK1[i].IValue != i) || (arrFK1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_2874sf_" + i + "! Expected value not returned, " + arrFK1[i].IValue);
                    }
                    if((arrFV1[i].IValue != i) || (arrFV1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_93765dg_" + i + "! Expected value not returned");
                    }
                }
                iCountTestcases++;
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                arrDicEnt1 = new DictionaryEntry[100];
                try
                {
                    mycol1.CopyTo(arrDicEnt1, 50);
                    iCountErrors++;
                    Console.WriteLine("Err_2075dfgv! Exception not thrown");
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_854732f! Unexception not thrown, " + ex.GetType().Name);
                }
                try
                {
                    mycol1.CopyTo(arrDicEnt1, -1);
                    iCountErrors++;
                    Console.WriteLine("Err_2075dfgv! Exception not thrown");
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_854732f! Unexception not thrown, " + ex.GetType().Name);
                }
                arrDicEnt1 = new DictionaryEntry[200];
                mycol1.CopyTo(arrDicEnt1, 100);
                arrFK1 = new FooKey[100];
                arrFV1 = new FooValue[100];
                for(int i=0; i<100; i++)
                {
                    arrFK1[i] = (FooKey)arrDicEnt1[i+100].Key;
                    arrFV1[i] = (FooValue)arrDicEnt1[i+100].Value;
                }
                Array.Sort(arrFK1);
                Array.Sort(arrFV1);
                for(int i=0; i<100; i++)
                {
                    if((arrFK1[i].IValue != i) || (arrFK1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_974gd_" + i + "! Expected value not returned, " + arrFK1[i].IValue);
                    }
                    if((arrFV1[i].IValue != i) || (arrFV1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_2075sg_" + i + "! Expected value not returned");
                    }
                }
                iCountTestcases++;
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                enu1 = mycol1.GetEnumerator();
                try
                {
                    dicEnt1 = (DictionaryEntry)enu1.Current;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                try
                {
                    fk1 = (FooKey)enu1.Key;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                try
                {
                    fv1 = (FooValue)enu1.Value;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                iCount=0;
                arrFK1 = new FooKey[100];
                arrFV1 = new FooValue[100];
            while( enu1.MoveNext() )
            {			
                dicEnt1 = (DictionaryEntry)enu1.Current;
                arrFK1[iCount] = (FooKey)dicEnt1.Key;
                arrFV1[iCount] = (FooValue)dicEnt1.Value;
                fk1 = (FooKey)enu1.Key;
                if(fk1!=arrFK1[iCount])
                {
                    iCountErrors++;
                    Console.WriteLine("Err_8543s_" + iCount + "! Expected value not returned, " + arrFK1[iCount].IValue);
                }
                fv1 = (FooValue)enu1.Value;
                if(fv1!=arrFV1[iCount])
                {
                    iCountErrors++;
                    Console.WriteLine("Err_29075gd_" + iCount + "! Expected value not returned, " + arrFV1[iCount].IValue);
                }
                iCount++;
            }
                if(iCount!=100)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_87543! doesnot match");
                }
                Array.Sort(arrFK1);
                Array.Sort(arrFV1);
                for(int i=0; i<100; i++)
                {
                    if((arrFK1[i].IValue != i) || (arrFK1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_89275sgf_" + i + "! Expected value not returned, " + arrFK1[i].IValue);
                    }
                    if((arrFV1[i].IValue != i) || (arrFV1[i].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_275g_" + i + "! Expected value not returned");
                    }
                }
                try
                {
                    dicEnt1 = (DictionaryEntry)enu1.Current;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                try
                {
                    fk1 = (FooKey)enu1.Key;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                try
                {
                    fv1 = (FooValue)enu1.Value;
                    iCountErrors++;
                    Console.WriteLine("Err_87543! Exception not thrown");
                }
                catch(InvalidOperationException)
                {
                }
                catch(Exception ex)
                {
                    Console.WriteLine("fail, should throw InvalidOperationException, thrown, " + ex.GetType().Name);
                }
                strLoc = "Loc_004oo";
                iCountTestcases++;
                mycol1 = new MyDictionaryBase();
                if(mycol1.IsSynchronized)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_275eg! Expected value not returned, " + mycol1.IsSynchronized);
                }
                iCountTestcases++;
                strLoc = "Loc_005oo";
                mycol1 = new MyDictionaryBase();
                obj1 = mycol1.SyncRoot;
                mycol2 = mycol1;
                if(obj1 != mycol2.SyncRoot)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_9745sg! Expected value not returned");
                }
                strLoc = "Loc_006oo";
                iCountTestcases++;
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                for(int i=0; i<100; i++)
                {
                    if((mycol1[new FooKey(i, i.ToString())].IValue != i) || (mycol1[new FooKey(i, i.ToString())].SValue!= i.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_2974swsg_" + i + "! Expected value not returned");
                    }
                }
                for(int i=0; i<100; i++)
                {
                    mycol1.Remove(new FooKey(i, i.ToString()));
                    if((mycol1.Count != (99-i)) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_2975sg_" + i + "! Expected value not returned");
                    }
                    if((mycol1.Contains(new FooKey(i, i.ToString()))) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_975wg_" + i + "! Expected value not returned");
                    }
                }				
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                mycol1.Clear();
                if(mycol1.Count!=0)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_234dnvf! Expected value not returned, " + mycol1.Count);
                }
                strLoc = "Loc_006oo";
                iCountTestcases++;
                mycol1 = new MyDictionaryBase();
                if(mycol1.IsFixedSize)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_9753sfg! Expected value not returned, " + mycol1.IsFixedSize);
                }
                if(mycol1.IsReadOnly)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_834sg! Expected value not returned, " + mycol1.IsReadOnly);
                }
                mycol1 = new MyDictionaryBase();
                for(int i=0; i<100; i++)
                    mycol1.Add(new FooKey(i, i.ToString()), new FooValue(i, i.ToString()));
                for(int i=0, j=100; i<100; i++, j--)
                    mycol1[new FooKey(i, i.ToString())] = new FooValue(j, j.ToString());
                for(int i=0, j=100; i<100; i++, j--)
                {
                    if((mycol1[new FooKey(i, i.ToString())].IValue != j) || (mycol1[new FooKey(i, i.ToString())].SValue!= j.ToString()) )
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_7342rfg_" + i + "! Expected value not returned");
                    }
                }
                mycol1 = new MyDictionaryBase();
                mycol1.Add(new FooKey(), new FooValue());
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
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3996AllMethods cbA = new Co3996AllMethods();
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
public class FooKey:IComparable
{
    private Int32 iValue;
    private String strValue;
    public FooKey()
    {
    }
    public FooKey(Int32 i, String str)
    {
        iValue = i;
        strValue = str;
    }
    public Int32 IValue
    {
        get{return iValue;}
        set{iValue=value;}
    }
    public String SValue
    {
        get{return strValue;}
        set{strValue=value;}
    }
    public override Boolean Equals(Object obj)
    {		
        if(obj==null)
            return false;
        if(!(obj is FooKey))
            return false;
        if((((FooKey)obj).IValue==iValue) && (((FooKey)obj).SValue==strValue))
            return true;
        return false;
    }
    public override Int32 GetHashCode()
    {
        return iValue;
    }
    public Int32 CompareTo(Object obj)
    {
        if(!(obj is FooKey))
            throw new ArgumentException("obj must be type FooKey");
        FooKey temp = (FooKey)obj;
        if(temp.IValue>iValue)
            return -1;
        else if(temp.IValue<iValue)
            return 1;
        else
            return 0;
    }
}
public class FooValue:IComparable
{
    private Int32 iValue;
    private String strValue;
    public FooValue()
    {
    }
    public FooValue(Int32 i, String str)
    {
        iValue = i;
        strValue = str;
    }
    public Int32 IValue
    {
        get{return iValue;}
        set{iValue=value;}
    }
    public String SValue
    {
        get{return strValue;}
        set{strValue=value;}
    }
    public override Boolean Equals(Object obj)
    {		
        if(obj==null)
            return false;
        if(!(obj is FooValue))
            return false;
        if((((FooValue)obj).IValue==iValue) && (((FooValue)obj).SValue==strValue))
            return true;
        return false;
    }
    public override Int32 GetHashCode()
    {
        return iValue;
    }
    public Int32 CompareTo(Object obj)
    {
        if(!(obj is FooValue))
            throw new ArgumentException("obj must be type FooValue");
        FooValue temp = (FooValue)obj;
        if(temp.IValue>iValue)
            return -1;
        else if(temp.IValue<iValue)
            return 1;
        else
            return 0;
    }
}
public class MyDictionaryBase:DictionaryBase
{
    public void Add(FooKey fk1, FooValue f2)
    {
        Dictionary.Add(fk1, f2);
    }
    public FooValue this[FooKey f1]
    {
        get{return (FooValue)Dictionary[f1];}
        set{Dictionary[f1] = value;}
    }	
    public Boolean IsSynchronized
    {
        get {return ((ICollection)Dictionary).IsSynchronized;}
    }
    public Object SyncRoot
    {
        get {return (ICollection)Dictionary.SyncRoot;}
    }
    public Boolean Contains(FooKey f)
    {
        return ((IDictionary)Dictionary).Contains(f);
    }
    public void Remove(FooKey f)
    {
        ((IDictionary)Dictionary).Remove(f);
    }
    public Boolean IsFixedSize 
    {
        get { return Dictionary.IsFixedSize; }
    }
    public Boolean IsReadOnly 
    {
        get { return Dictionary.IsReadOnly; }
    }
    public ICollection Keys
    {
        get{return Dictionary.Keys;}
    }
    public ICollection Values
    {
        get{return Dictionary.Values;}
    }
}
