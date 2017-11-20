//------------------------------------------------------------------------------
// <copyright file="keyconfig.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>
//------------------------------------------------------------------------------


using System;
using System.IO;
using System.Text;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

/// <summary>
/// This class contains the platform invoke declarations for calling the assembly key functions.
/// It also contains some helper methods for working with assembly keys.
/// </summary>
class StrongName 
{

#if !PLATFORM_UNIX
    private const String DLLPREFIX = "";
    private const String DLLSUFFIX = ".dll";
#else // !PLATFORM_UNIX
#if __APPLE__
    private const String DLLPREFIX = "lib";
    private const String DLLSUFFIX = ".dylib";
#else
    private const String DLLPREFIX = "lib";
    private const String DLLSUFFIX = ".so";
#endif
#endif // !PLATFORM_UNIX

    private const String MSCORSN = DLLPREFIX + "mscorsn" + DLLSUFFIX;

    [DllImport(MSCORSN, CharSet=CharSet.Unicode)]
    private static extern int StrongNameErrorInfo();

    [DllImport(MSCORSN, CharSet=CharSet.Unicode)]
    private static extern void StrongNameFreeBuffer(IntPtr pbMemory);

    [DllImport(MSCORSN, CharSet=CharSet.Unicode)]
    private static extern byte StrongNameGetPublicKey(
        IntPtr      keyContainer,
        byte[]      pbKeyBlob,
        int         cbKeyBlob,
        ref IntPtr  pbPublicKeyBlob,
        ref int     cbPublicKeyBlob);

    [DllImport(MSCORSN, CharSet=CharSet.Unicode)]
    private static extern byte StrongNameTokenFromPublicKey(
        byte[]      pbPublicKeyBlob,
        int         cbPublicKeyBlob,
        ref IntPtr  pbStrongNameToken,
        ref int     pcbStrongNameToken);

    /// <summary>
    /// Gets the public portion of the key from a key pair.
    /// </summary>
    /// <param name="key"></param>
    /// <returns></returns>
    public static byte[] GetPublicKey(byte[] key) 
    {
        IntPtr buf = IntPtr.Zero;
        int cb = 0;
        byte[] ret;
        try 
        {
            if (StrongNameGetPublicKey(IntPtr.Zero, key, key.Length, ref buf, ref cb) == 0) 
            {
                throw new Win32Exception(StrongNameErrorInfo());
            }

            ret = new byte[cb];
            Marshal.Copy(buf, ret, 0, cb);
        }
        finally 
        {
            StrongNameFreeBuffer(buf);
        }

        return ret;
    }

    /// <summary>
    /// Gets a tokenized representation of the public key from the full public key.
    /// </summary>
    /// <param name="publicKey"></param>
    /// <returns></returns>
    public static byte[] GetToken(byte[] publicKey) 
    {
        IntPtr buf = IntPtr.Zero;
        int cb = 0;
        byte[] ret;
        try 
        {
            if (StrongNameTokenFromPublicKey(publicKey, publicKey.Length, ref buf, ref cb) == 0) 
            {
                throw new Win32Exception(StrongNameErrorInfo());
            }

            ret = new byte[cb];
            Marshal.Copy(buf, ret, 0, cb);
        }
        finally 
        {
            StrongNameFreeBuffer(buf);
        }

        return ret;
    }
};

/// <summary>
/// This class handles the key configuration process in the SSCLI directory.
/// It also contains the main entry point for the sample application.
/// </summary>
class KeyConfig 
{
    /// <summary>
    /// Outputs usage text to console.
    /// </summary>
    static void Usage() 
    {
        Console.WriteLine("Usage:");
        Console.WriteLine();
        Console.WriteLine("keyconfig sign <filename>");
        Console.WriteLine("    Sets the key used to fully sign the SSCLI binaries.");
        Console.WriteLine("    <filename> must be a valid assembly key pair.");
        Console.WriteLine();
        Console.WriteLine("keyconfig delaysign <filename>");
        Console.WriteLine("    Sets the key used to delay sign the SSCLI binaries.");
        Console.WriteLine("    <filename> must be a valid public assembly key or key pair.");
        Console.WriteLine();
        Console.WriteLine("keyconfig rollback");
        Console.WriteLine("    Rolls back SSCLI assembly key modifications.");
        Console.WriteLine();
        Console.WriteLine("buildall -c is required for any changes to take effect");
    }

    enum ConfigFiles 
    {
        Build,
        Mscorsn
    };

    static void ReportProgress(string message) 
    {
        Console.WriteLine(message + "...");
    }

    /// <summary>
    /// Archives existing keyfiles so they can be restored later.
    /// Also serves to roll back files from archived versions.
    /// </summary>
    /// <param name="files"></param>
    /// <param name="rollback">If true then roll back to archived versions of files.</param>
    static void Archive(string[] files, bool rollback) 
    {
        for (int i = 0; i < files.Length; i++) 
        {
            string file = files[i];
            string backupfile = file + "_";

            if (rollback) 
            {
                if (File.Exists(backupfile)) 
                {
                    ReportProgress("Rolling back " + file);
                    File.Copy(backupfile, file, true);
                    ReportProgress("Remember to use the sn tool to remove any non-essential entries");
                    ReportProgress("in assembly key verification skip list.");
                }
                else 
                {
                    Console.WriteLine("Can't rollback, backup file does not exist " + file);
                }
            }
            else 
            {
                if (!File.Exists(backupfile)) 
                {
                    ReportProgress("Backing up " + file);
                    File.Copy(file, backupfile, false);
                }
            }
        }
    }

    /// <summary>
    /// Converts a byte array containng a key to a string representation of the key.
    /// </summary>
    /// <param name="bytes"></param>
    /// <param name="cppFormat"></param>
    /// <returns></returns>
    static string BytesToString(byte[] bytes, bool cppFormat) 
    {
        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < bytes.Length; i++) 
        {
            if (cppFormat) 
            {
                builder.Append("0x");
            }
            builder.Append(bytes[i].ToString("x02"));
            if (cppFormat && i != bytes.Length-1) 
            {
                builder.Append(",");
            }
        }
        return builder.ToString();
    }

    /// <summary>
    /// Main entry point of sample application.
    /// </summary>
    /// <param name="args"></param>
    /// <returns></returns>
    static int Main(string[] args) 
    {

        try 
        {
            Console.WriteLine("Microsoft (R) Shared Source CLI Strong Name Configuration Sample");
            Console.WriteLine("Copyright (C) Microsoft Corporation 1998-2002. All rights reserved.");
            Console.WriteLine();

            if (args.Length == 0) 
            {
                Usage();
                return 1;
            }

            string RotorDir = Environment.GetEnvironmentVariable("ROTOR_DIR");
            if (RotorDir == null) 
            {
                Console.WriteLine("This utility has to be run from within the Rotor environment.");
                return 1;
            }

            string[] files = {
                                 /* Build */   @"rotorenv\bin\strongnamekey.inc",
                                 /* Mscorsn */ @"clr\src\dlls\mscorsn\thekey.h",
            };

            for (int i = 0; i < files.Length; i++) 
            {
                files[i] = Path.GetFullPath(Path.Combine(RotorDir, files[i]));
            }

            bool delaysign = false;

            switch (args[0]) 
            {
                case "delaysign":
                    delaysign = true;
                    goto case "sign";

                case "sign":
                    if (args.Length != 2) 
                    {
                        goto default;
                    }

                    // Archive the files before modifying them.
                    Archive(files, false);

                    string keyFile = Path.GetFullPath(args[1]);

                    ReportProgress("Reading " + keyFile);

                    FileStream keyStream = new FileStream(keyFile, FileMode.Open, FileAccess.Read, FileShare.Read);
                    byte[] key = new byte[keyStream.Length];
                    if (keyStream.Read(key, 0, key.Length) != key.Length)
                        throw new IOException();
                    keyStream.Close();

                    byte[] publicKey;
                    if (!delaysign) 
                    {
                        ReportProgress("Getting the public key.");
                        publicKey = StrongName.GetPublicKey(key);
                    }
                    else 
                    {
                        publicKey = key;
                    }

                    ReportProgress("Getting the key token.");
                    byte[] keyToken = StrongName.GetToken(publicKey);

                    // Modify sscli/rotorenv/bin/strongnamekey.inc to contain the new key information.
                    string filename;
                    StreamWriter writer;

                    filename = files[(int)ConfigFiles.Build];
                    ReportProgress("Writing " + filename);
                    writer = new StreamWriter(
                        new FileStream(filename, FileMode.Create, FileAccess.Write, FileShare.None));
                    writer.WriteLine("MICROSOFT_KEY_FILE = " + keyFile);
                    writer.WriteLine("MICROSOFT_KEY_TOKEN = " + BytesToString(keyToken, false));
                    writer.WriteLine("MICROSOFT_KEY_FULL = " + BytesToString(publicKey, false));
                    writer.WriteLine("DELAY_SIGN = " + (delaysign ? "1" : "0"));
                    writer.Close();

                    // Modify sscli/clr/src/dlls/mscorsn/thekey.h to contain the new key information.
                    filename = files[(int)ConfigFiles.Mscorsn];
                    ReportProgress("Writing " + files[(int)ConfigFiles.Mscorsn]);
                    writer = new StreamWriter(
                        new FileStream(filename, FileMode.Create, FileAccess.Write, FileShare.None));
                    writer.WriteLine("static const BYTE g_rbTheKey[] = {" + BytesToString(publicKey, true) + "};");
                    writer.WriteLine("static const BYTE g_rbTheKeyToken[] = {" + BytesToString(keyToken, true) + "};");
                    writer.Close();

                    // If delay signed then add key to verification skip list so that errors will not occur on load.
                    if (delaysign) 
                    {
                        string snpath = Path.GetFullPath(Path.Combine(
                            Environment.GetEnvironmentVariable("TARGETCOMPLUS"), @"sdk\bin\sn"));

                        string snargs = "-Vr *," + BytesToString(keyToken, false);

                        ReportProgress("Executing \"sn " + snargs + "\" to add the key to the skip list.");

                        Process p = Process.Start(snpath, snargs);
                        if (p.WaitForExit(Timeout.Infinite) != true) 
                        {
                            Console.WriteLine("sn returned failure code");
                            return 1;
                        }
                        p.Close();

                        Console.WriteLine();
                    }
                    break;

                case "rollback":
                    if (args.Length != 1) 
                    {
                        goto default;
                    }

                    Archive(files, true);
                    break;

                default:
                    Console.WriteLine("Invalid options");
                    return 1;
            }

            Console.WriteLine("Succeeded.");
            Console.WriteLine();
            Console.WriteLine("buildall -c is required for the changes to take effect.");

            return 0;
        }
        catch(Exception e) 
        {
            Console.WriteLine("FATAL ERROR");
            Console.WriteLine(e);
            return 1;
        }
    }
}

///////////////////////////////// End of File /////////////////////////////////
