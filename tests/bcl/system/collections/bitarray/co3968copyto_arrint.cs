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
public class Co3968CopyTo_ArrInt
{
    public static String s_strActiveBugNums = "";
    public static String s_strDtTmVer       = "";
    public static String s_strClassMethod   = "BitArray.ToCopy(Array, Int32)";
    public static String s_strTFName        = "Co3968CopyTo_ArrInt.cs";
    public static String s_strTFAbbrev      = s_strTFName.Substring(0, 6);
    public static String s_strTFPath        = Environment.CurrentDirectory;
    public bool runTest()
    {
        Console.WriteLine(s_strTFPath + " " + s_strTFName + " , for " + s_strClassMethod + " , Source ver : " + s_strDtTmVer);
        int iCountErrors = 0;
        int iCountTestcases = 0;
        String strLoc = "Loc_000oo";
        BitArray bitArr1;
        BitArray bitArr2;
        Boolean[] bolArr1;
        Boolean[] bolArr2;
        Byte[] bytArr1;
        Byte[] bytArr2;
        Int32[] intArr1;
        Int32[] intArr2;
        SByte[] sbtArr1;
        Int32 iNumOfElements;
        Random rnd1;
        try 
        {
            do
            {
                iNumOfElements = 10;
                rnd1 = new Random();
                strLoc = "Loc_742dsf!";
                iCountTestcases++;
                bolArr1 = new Boolean[iNumOfElements];
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(rnd1.Next(10)>5)
                        bolArr1[i] = true;
                    else
                        bolArr1[i] = false;
                }
                bitArr1 = new BitArray(bolArr1);
                bolArr2 = new Boolean[iNumOfElements];
                bitArr1.CopyTo(bolArr2, 0);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(bolArr1[i] != bolArr2[i])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_753qn_" + i + "! Wrong value returned, " + bolArr1[i] + " " + bolArr1[2]);
                    }
                }
                bolArr2 = new Boolean[iNumOfElements + 20];
                bitArr1.CopyTo(bolArr2, 20);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(bolArr1[i] != bolArr2[i+20])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_763dfsf! Wrong value returned");
                    }
                }
                try
                {
                    bolArr2 = new Boolean[iNumOfElements];
                    bitArr1.CopyTo(bolArr2, 5);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_742dsf! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bolArr2 = new Boolean[iNumOfElements-1];
                    bitArr1.CopyTo(bolArr2, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_753edfsf! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bolArr2 = new Boolean[iNumOfElements];
                    bitArr1.CopyTo(bolArr2, -1);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentOutOfRangeException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_742efsg! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bolArr2 = new Boolean[iNumOfElements];
                    bitArr1.CopyTo(null, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentNullException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_10753sg! Unexpected exception thrown, " + ex);					
                }
                strLoc = "Loc_0472eaf!";
                iCountTestcases++;
                bytArr1 = new Byte[iNumOfElements];
                for(int i=0; i<iNumOfElements; i++)
                {
                    bytArr1[i] = (Byte)i;
                }
                bitArr1 = new BitArray(bytArr1);
                bytArr2 = new Byte[iNumOfElements];
                bitArr1.CopyTo(bytArr2, 0);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(bytArr1[i] != bytArr2[i])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_753qn_" + i + "! Wrong value returned, " + bolArr1[i] + " " + bolArr1[2]);
                    }
                }
                bytArr2 = new Byte[iNumOfElements + 20];
                bitArr1.CopyTo(bytArr2, 20);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(bytArr1[i] != bytArr2[i+20])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_763dfsf! Wrong value returned");
                    }
                }
                try
                {
                    bytArr2 = new Byte[iNumOfElements];
                    bitArr1.CopyTo(bytArr2, 5);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_6924qsgf! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bytArr2 = new Byte[iNumOfElements-1];
                    bitArr1.CopyTo(bytArr2, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_24683rsdg! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bytArr2 = new Byte[iNumOfElements];
                    bitArr1.CopyTo(bytArr2, -1);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentOutOfRangeException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_563twsgt! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bitArr1.CopyTo(null, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentNullException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_674tgdfgbdg! Unexpected exception thrown, " + ex);					
                }
                strLoc = "Loc_28453sf!";
                iCountTestcases++;
                intArr1 = new Int32[iNumOfElements];
                for(int i=0; i<iNumOfElements; i++)
                {
                    intArr1[i] = i;
                }
                bitArr1 = new BitArray(intArr1);
                intArr2 = new Int32[iNumOfElements];
                bitArr1.CopyTo(intArr2, 0);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(intArr1[i] != intArr2[i])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_753qn_" + i + "! Wrong value returned, " + bolArr1[i] + " " + bolArr1[2]);
                    }
                }
                intArr2 = new Int32[iNumOfElements + 20];
                bitArr1.CopyTo(intArr2, 20);
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(intArr1[i] != intArr2[i+20])
                    {
                        iCountErrors++;
                        Console.WriteLine("Err_763dfsf! Wrong value returned");
                    }
                }
                try
                {
                    intArr2 = new Int32[iNumOfElements];
                    bitArr1.CopyTo(intArr2, 5);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_563rtfzgs! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    intArr2 = new Int32[iNumOfElements-1];
                    bitArr1.CopyTo(intArr2, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_764wdsgsf! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    intArr2 = new Int32[iNumOfElements];
                    bitArr1.CopyTo(intArr2, -1);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentOutOfRangeException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_43wsdg! Unexpected exception thrown, " + ex);					
                }
                try
                {
                    bitArr1.CopyTo(null, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentNullException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_94720dnsl! Unexpected exception thrown, " + ex);					
                }
                strLoc = "Loc_4782eaf!";
                iCountTestcases++;
                try
                {
                    sbtArr1 = new SByte[100];
                    bitArr1.CopyTo(sbtArr1, 0);
                    iCountErrors++;
                    Console.WriteLine("Err_97452dsaf! No exception thrown");					
                }
                catch(ArgumentException)
                {
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_435324sg! Unexpected exception thrown, " + ex);					
                }
                strLoc = "Loc_275sfdsgf!";
                iCountTestcases++;
                bolArr1 = new Boolean[iNumOfElements];
                for(int i=0; i<iNumOfElements; i++)
                {
                    if(rnd1.Next(10)>5)
                        bolArr1[i] = true;
                    else
                        bolArr1[i] = false;
                }
                bitArr1 = new BitArray(bolArr1);
                intArr2 = new Int32[iNumOfElements];
                try
                {
                    bitArr1.CopyTo(intArr2, 0);
                }
                catch(Exception ex)
                {
                    iCountErrors++;
                    Console.WriteLine("Err_019472eaf! Unexpected exception thrown, " + ex);					
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
    public static void Main(String[] args)
    {
        bool bResult = false;
        Co3968CopyTo_ArrInt cbA = new Co3968CopyTo_ArrInt();
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
