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
namespace System.Tools {
    using System;
    using System.IO; 
    using System.Collections; 
    using System.Resources; 
    using System.Text;
    using System.Diagnostics;
    using System.Globalization;
    
    // .NET Development Platform Resource file Generator
    // 
    // This program will read in text files or ResX files of name-value pairs and 
    // produces a .NET .resources file.  Additionally ResGen can change data from
    // any of these three formats to any of the other formats (though text files
    // only support strings).
    //
    // The text files must have lines of the form name=value, and comments are 
    // allowed ('#' at the beginning of the line).  
    // 
    /// <include file='doc\ResGen.uex' path='docs/doc[@for="ResGen"]/*' />
    /// <devdoc>
    ///    <para>                              </para>
    /// </devdoc>
    public sealed class ResGen {
        private const int errorCode = unchecked((int)0xbaadbaad);
    
        private static int errors = 0;
    
        // We use a list to preserve the resource ordering (primarily for easier testing),
        // but also use a hash table to check for duplicate names.
        private static ArrayList resources = new ArrayList();
        private static Hashtable resourcesHashTable = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
    
        private static void AddResource(string name, object value) {
            Entry entry = new Entry(name, value);
    
            if (resourcesHashTable.ContainsKey(name.ToString())) {
                Error(SR.GetString(SR.DuplicateResourceKey, name));
                return;
            }
    
            resources.Add(entry);
            resourcesHashTable.Add(name, value);
        }
    
        private static Boolean EndsWithOrdinal(String value, String suffix) {
            int valueLen = value.Length;
            int suffixLen = suffix.Length;
            if (suffixLen>valueLen) {
                return false;
            }
            return(0==String.CompareOrdinal(value, valueLen-suffixLen, suffix, 0, suffixLen));
        }
    
        private static void Error(string message) {
            Console.Error.WriteLine(SR.GetString(SR.ErrorOutput, message));
            errors++;
        }

        private static void Warning(string message) {
            Console.Error.WriteLine(SR.GetString(SR.WarningOutput, message));
        }
    
        private static Format GetFormat(string filename) {
            string extension = Path.GetExtension(filename);
            if (String.Compare(extension, ".txt", true) == 0)
                return Format.Text;
            else if (String.Compare(extension, ".resources", true) == 0)
                return Format.Binary;
            else {
                Error(SR.GetString(SR.UnknownFileExtension, extension));
                Environment.Exit(errorCode);
                Debug.Fail("Why didn't Environment.Exit exit?");
                return Format.Text; // never reached
            }
        }
    
        /// <include file='doc\ResGen.uex' path='docs/doc[@for="ResGen.Main"]/*' />
        /// <devdoc>
        ///    <para>                              </para>
        /// </devdoc>
        public static void Main(String[] args) {
            // Tell build we had an error, then set this to 0 if we complete successfully.
            Environment.ExitCode = errorCode;
            if (args.Length < 1 || args[0].Equals("-h") || args[0].Equals("-?") ||
                args[0].Equals("/h") || args[0].Equals("/?")) {
                Usage();
                return;
            }
    
            String[] inFiles;
            String[] outFiles;
            if (args[0].Equals("/compile")) {
                inFiles = new String[args.Length - 1];
                outFiles = new String[args.Length - 1];
                for(int i=0; i<inFiles.Length; i++) {
                    inFiles[i] = args[i+1];
                    int index = inFiles[i].IndexOf(",");
                    if (index != -1) {
                        String tmp = inFiles[i];
                        inFiles[i] = tmp.Substring(0, index);
                        if (index == tmp.Length-1) {
                            Error(SR.GetString(SR.MalformedCompileString, tmp));
                            inFiles = new String[0];
                            break;
                        }
                        outFiles[i] = tmp.Substring(index+1);
                    }
                    else {
                        string resourceFileName = GetResourceFileName(inFiles[i]);
                        if (resourceFileName == null) {
                            Usage();
                            return;
                        }
                        outFiles[i] = resourceFileName;
                    }
                }
            }
            else if (args.Length <= 2) {
                inFiles = new String[1];
                inFiles[0] = args[0];
                outFiles = new String[1];
                if (args.Length == 2) {
                    outFiles[0] = args[1];
                }
                else {
                    string resourceFileName = GetResourceFileName(inFiles[0]);
                    if (resourceFileName == null) {
                        Usage();
                        return;
                    }
                    outFiles[0] = resourceFileName;
                }
            }
            else {
                Usage();
                return;
            }            
    
            // Do all the work.
            for(int i=0; i<inFiles.Length; i++) {
                ProcessFile(inFiles[i], outFiles[i]);
            }
    
            // Quit & report errors, if necessary.
            if (errors != 0) {
                Console.Error.WriteLine(SR.GetString(SR.ErrorCount, errors));
            }
            else {
                // Tell build we succeeded.
                Environment.ExitCode = 0;
            }
        }
    
        private static String GetResourceFileName(String inFile) {
            if (inFile == null) {
                return null;
            }
        
            // Note that the naming scheme is basename.[en-US.]resources
            int end = inFile.LastIndexOf('.');
            if (end == -1) {
                return null;
            }
            return inFile.Substring(0, end) + ".resources";
        }
    
        private static void ProcessFile(String inFile, String outFile) {
            //Console.WriteLine("Processing {0} --> {1}", inFile, outFile);
            // Reset state
            resources.Clear();
            resourcesHashTable.Clear();
    
            try {
                // Explicitly handle missing input files here - don't catch a 
                // FileNotFoundException since we can get them from the loader
                // if we try loading an assembly version we can't find.
                if (!File.Exists(inFile)) {
                    Error(SR.GetString(SR.FileNotFound, inFile));
                    return;
                }

                ReadResources(inFile);
            }
            catch (Exception e) {
                Error(e.Message);
                return;
            }
    
            try {
                WriteResources(outFile);
            }
            catch (IOException io) {
                Error(SR.GetString(SR.WriteError, outFile));
                if (io.Message != null)
                    Error(SR.GetString(SR.SpecificError, io.Message));
                if (File.Exists(outFile)) {
                    Error(SR.GetString(SR.CorruptOutput, outFile));
                    try {
                        File.Delete(outFile);
                    }
                    catch (Exception) {
                        Error(SR.GetString(SR.DeleteOutputFileFailed, outFile));
                    }
                }
                return;
            }
            catch (Exception) {
                Error(SR.GetString(SR.GenericWriteError, outFile));
            }
        }
    
        // <doc>
        // <desc>
        //     Reads the resources out of the specified file and populates the
        //     resources hashtable.
        // </desc>
        // <param term='filename'>
        //     Filename to load.
        // </param>
        // </doc>   
        private static void ReadResources(String filename) {
            Format format = GetFormat(filename);
            switch (format) {
                case Format.Text:
                    ReadTextResources(filename);
                    break;
    
    
                case Format.Binary:
                    ReadResources(new ResourceReader(filename)); // closes reader for us
                    break;
    
                default:
                    Debug.Fail("Unknown format " + format.ToString());
                    break;
            }
            Console.WriteLine(SR.GetString(SR.ReadIn, resources.Count, filename));
        }
    
        // closes reader when done
        private static void ReadResources(IResourceReader reader) {
            IDictionaryEnumerator resEnum = reader.GetEnumerator();
            while (resEnum.MoveNext()) {
                string name = (string)resEnum.Key;
                object value = resEnum.Value;
                AddResource(name, value);
            }
            reader.Close();
        }
    
        private static void ReadTextResources(String filename) {
            // Check for byte order marks in the beginning of the input file, but
            // default to UTF-8.
            StreamReader sr = new StreamReader(filename, new UTF8Encoding(true), true);
            StringBuilder name = new StringBuilder(255);
            StringBuilder value = new StringBuilder(2048);
    
            int ch = sr.Read();
            while (ch != -1) {
                if (ch == '\n' || ch == '\r') {
                    ch = sr.Read();
                    continue;
                }
    
                // Skip over commented lines or ones starting with whitespace.
                if (ch == '#' || ch == '\t' || ch == ' ' || ch == ';') {
                    // comment char (or blank line) - skip line.
                    sr.ReadLine();
                    ch = sr.Read();
                    continue;
                }

                if (ch == '[') {
                    String skip = sr.ReadLine();
                    if (skip.Equals("strings]"))
                        Warning(SR.GetString(SR.StringsTagObsolete));
                    else
                        throw new Exception(SR.GetString(SR.INFFileBracket, skip));
                    ch = sr.Read();
                    continue;
                }

                // Read in name
                name.Length = 0;
                while (ch != '=') {
                    if (ch == '\r' || ch == '\n')
                        throw new Exception(SR.GetString(SR.NoEqualsWithNewLine, name.Length, name));
                    
                    name.Append((char)ch);
                    ch = sr.Read();
                    if (ch == -1)
                        break;
                }
                if (name.Length == 0)
                    throw new Exception(SR.GetString(SR.NoEquals));
    
                if (name[name.Length-1] == ' ') {
                    name.Length = name.Length - 1;
                }
                ch = sr.Read(); // move past =
                // If it exists, move past the first space after the equals sign.
                if (ch == ' ')
                    ch = sr.Read();
    
                // Read in value
                value.Length = 0;
    
                for (;;) {
                    if (ch == '\\') {
                        ch = sr.Read();
                        switch (ch) {
                            case '\\':
                                // nothing needed
                                break;
                            case 'n':
                                ch = '\n';
                                break;
                            case 'r':
                                ch = '\r';
                                break;
                            case 't':
                                ch = '\t';
                                break;
                            case '"':
                                ch = '\"';
                                break;
                            default:
                                throw new Exception(SR.GetString(SR.BadEscape, name.ToString()));
                        }
                    }
    
                    value.Append((char)ch);
                    ch = sr.Read();
    
                    // Consume endline...
                    //   Endline can be \r\n or \n.
                    if (ch == '\r') {
                        ch = sr.Read();
                        if (ch == -1) {
                            break;
                        }
                        else if (ch == '\n') {
                            ch = sr.Read();
                        }
                        break;
                    }
                    else if (ch == '\n') {
                        ch = sr.Read();
                        break;
                    }
    
                    if (ch == -1)
                        break;
                }
    
    
                if (value.Length == 0)
                    throw new Exception(SR.GetString(SR.NoName, name));
    
                AddResource(name.ToString(), value.ToString());
            }
            sr.Close();
        }
    
        private static void WriteResources(String filename) {
            Format format = GetFormat(filename);
            switch (format) {
                case Format.Text:
                    WriteTextResources(filename);
                    break;

    
                case Format.Binary:
                    WriteResources(new ResourceWriter(filename)); // closes writer for us
                    break;
    
                default:
                    Debug.Fail("Unknown format " + format.ToString());
                    break;
            }
        }
    
        // closes writer automatically
        private static void WriteResources(IResourceWriter writer) {
            foreach (Entry entry in resources) {
                string key = entry.name;
                object value = entry.value;
                writer.AddResource(key, value);
            }
            Console.Write(SR.GetString(SR.BeginWriting));
            writer.Close();
            Console.WriteLine(SR.GetString(SR.DoneDot));
        }
    
        private static void WriteTextResources(String filename) {
            StreamWriter writer = new StreamWriter(filename);
            foreach (Entry entry in resources) {
                string key = entry.name;
                object value = entry.value;
                if (!(value is string)) {
                    Error(SR.GetString(SR.OnlyString, key, value.GetType().FullName));
                }
                writer.WriteLine("{0}={1}", key, value.ToString());
            }
            writer.Close();
        }
    
        private static void Usage() {
            Console.WriteLine(SR.GetString(SR.Usage_Rotor, Environment.Version, ThisAssembly.Copyright));
        }
    
        private enum Format {
            Text, // .txt
            Binary, // .resources
        }
    
        // name/value pair
        private class Entry {
            public Entry(string name, object value) {
                this.name = name;
                this.value = value;
            }
    
            public string name;
            public object value;
        }
    }
}
