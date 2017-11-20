//------------------------------------------------------------------------------
// <copyright file="cleanbuild.cs" company="Microsoft">
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
using System.Diagnostics;
using System.Collections;
using System.Collections.Specialized;
using System.Xml.Serialization;
using System.Threading;

public enum OSTypeEnum {Windows, Unix};

/// <summary>
/// CleanBuild utility sample.  
/// Requires tar utility in path.
/// Demonstrates the following functionality:
/// - Console IO
/// - Argument parsing.
/// - XML serialization to store settings.  An alternative to this is to use Isolated Storage.
/// - Using the Process class to run another console application and 
///   redirect standard output and error.
/// - Thread creation and management via ThreadPool and WaitHandles.
/// </summary>
public class CleanBuild
{
    const string m_SettingsFileName = @"cb.xml";
    const string m_WindowsDeleteScriptName = @"deletefiles.bat";
    const string m_UnixDeleteScriptName = @"deletefiles";
    const string m_CommandArgument = "-tf ";

    // Start with gnutar.  Fall back to tar.  
    // Tar may have issues with long file names on Mac OS X.
    static string m_CommandName = "gnutar";
    public static string m_nl = Environment.NewLine;
    public static string m_sep = Path.DirectorySeparatorChar.ToString();

    public static string m_DeleteScriptName = null;

    /// <summary>
    /// Main entry point.
    /// - Gets current arguments and stores in CurrentArgs.
    /// - Gets current settings and stores in CurrentSettings.
    /// - Runs application to get data on files.
    /// - Fills FileTable with list of files in .tar file.
    /// - Checks files in the directory to see if in .tar archive.  If not then added to deletion script.
    /// </summary>
    /// <param name="args"></param>
    static void Main(string[] args)
    {
        try
        {
            switch (GetOSType())
            {
                case OSTypeEnum.Windows:
                {
                    m_DeleteScriptName = m_WindowsDeleteScriptName;
                    break;
                } //case
                case OSTypeEnum.Unix:
                {
                    m_DeleteScriptName = m_UnixDeleteScriptName;
                    break;
                } //case
                default:
                {
                    throw new System.InvalidOperationException("Invalid OSTypeEnum value.");
                } //case
            } //switch

            Args CurrentArgs = GetArgs(args);

            if (CurrentArgs.Usage || CurrentArgs.KillSettings)
            {
                System.Environment.ExitCode = 0;
                return;  //User choose argument that terminates normally.
            }

            if (CurrentArgs.Error)
            {
                System.Environment.ExitCode = 1;
                return; //Error in arguments.
            }

            Settings CurrentSettings = null;
            CurrentSettings = GetSettings(CurrentArgs);

            if (CurrentSettings.Error)
            {
                System.Environment.ExitCode = 1;
                return;
            } //if

            switch (CurrentSettings.OSType)
            {
                case OSTypeEnum.Windows:
                {
                    m_DeleteScriptName = m_WindowsDeleteScriptName;
                    break;
                } //case
                case OSTypeEnum.Unix:
                {
                    m_DeleteScriptName = m_UnixDeleteScriptName;
                    break;
                } //case
                default:
                {
                    throw new System.InvalidOperationException("Invalid OSTypeEnum value.");
                } //case
            } //switch
      
            RunCommand(CurrentSettings);
            System.Environment.ExitCode = 0;
            //Console.ReadLine();	
        } //try
        catch (Exception e)
        {
            Console.WriteLine("Exception in CleanBuild application: {0}", e.ToString());
            System.Environment.ExitCode = 1;
        } //catch
    } //Main

    /// <summary>
    /// Invokes tar and builds the list of files in the archive which are not to be deleted.
    /// </summary>
    /// <param name="CurrentSettings"></param>
    public static void RunCommand(Settings CurrentSettings)
    {
        try
        {
            StringDictionary FileTable = new StringDictionary();
            ArrayList DeleteList = new ArrayList();
            string DeleteCommand = null;
            string sep = Path.DirectorySeparatorChar.ToString();

            StreamWriter OutFile = new StreamWriter(CurrentSettings.DeletionScriptPath + 
                Path.DirectorySeparatorChar + 
                m_DeleteScriptName);

            switch (CurrentSettings.OSType)
            {
                case OSTypeEnum.Windows:
                {
                    DeleteCommand = "@del /f ";
                    break;
                } //case
                case OSTypeEnum.Unix:
                {
                    OutFile.WriteLine("#!/bin/sh");
                    DeleteCommand = "rm -f -v ";
                    break;
                } //case
                default:
                {
                    throw new System.InvalidOperationException("Invalid OSTypeEnum value.");
                } //case
            } //switch

            string fullCommand = m_CommandArgument + CurrentSettings.TarFileName;
            // Check to see that tar is in path.

            Console.WriteLine();
            // tar fails on the Mac.  Try gnutar first.
            if (ToolInstalled("gnutar"))
            {
                m_CommandName = "gnutar";
                Console.WriteLine("Found utility named: {0}", m_CommandName);
            } //if
            else
            {
                if (ToolInstalled("tar"))
                {
                    m_CommandName = "tar";
                    Console.WriteLine("Found utility named: {0}", m_CommandName);
                    Console.WriteLine("Tar utility may truncate file names on Mac OS X.");
                } //if
                else  //No tar installed.
                {
                    Console.WriteLine("No tar utility found. Exiting...");
                    System.InvalidOperationException ioe = new System.InvalidOperationException("No tar utility found.");
                    throw ioe;
                } //else
            }

            ConsoleProcess toolProc = new ConsoleProcess(m_CommandName, fullCommand);
            Console.WriteLine(m_nl + "Starting command {0} {1}", m_CommandName, fullCommand);
            toolProc.Start();

            // Wait for all IO to complete.
            toolProc.WaitForOutput();

            // Get standard output and error (if any).
            string toolStdOut = toolProc.StandardOutputString;
            string toolStdError = toolProc.StandardErrorString;
    
            // If there is output to stdErr or a bad command exit code output warning.
            if (toolStdError != null || toolProc.BaseProcess.ExitCode != 0)
            {
                Console.WriteLine(m_nl + 
                    "*************************** Tool Error ***************************");
                Console.WriteLine(m_nl +
                    "Exit code: {0}", toolProc.BaseProcess.ExitCode);
                Console.WriteLine(m_nl + 
                    "Error in tool operation: {0}", toolStdError);
                System.Environment.ExitCode = toolProc.BaseProcess.ExitCode;
                return;
            } //if
    
            if (toolStdOut == null || toolStdOut.Length < 1)
            {
                Console.WriteLine(m_nl + "No file list generated, exiting");
                System.Environment.ExitCode = 1;
                return;
            } //if

            Console.WriteLine(m_nl + "Finished {0} {1}, searching for files to delete ...", 
                m_CommandName, 
                m_CommandArgument);
		
            StringReader outputList = new StringReader(toolStdOut);
            string fname = null;
            string line = null;
      
            while (outputList.Peek() > -1)
            {
                line = outputList.ReadLine();

                // Tar always outputs using forward slashes as the separator char.
                if (CurrentSettings.OSType == OSTypeEnum.Windows)
                {
                    fname = CurrentSettings.SSCLIRootDirectory + sep + line.Replace("/", sep);
                } //if
                else
                {
                    fname = CurrentSettings.SSCLIRootDirectory + sep + line;
                } //else

                if (!Directory.Exists(fname)) // filter out directory names
                {
                    // There is a rare case where the table already contains the name.
                    if (!FileTable.ContainsKey(fname.ToLower()))
                    {
                        FileTable.Add(fname.ToLower(), fname.ToLower());
                    } //if
                } //if
            } //while
    
            CreateDeletionFile(new DirectoryInfo(CurrentSettings.SSCLIRootDirectory), 
                OutFile, 
                FileTable, 
                DeleteList, 
                DeleteCommand, 
                CurrentSettings);
            OutFile.Flush();
            OutFile.Close();

            // Make script executable on Unix
            if (CurrentSettings.OSType == OSTypeEnum.Unix)
            {
                System.Diagnostics.ProcessStartInfo si = new System.Diagnostics.ProcessStartInfo();
                si.FileName = "chmod";
                si.Arguments = "+x " + 
                    CurrentSettings.DeletionScriptPath + 
                    Path.DirectorySeparatorChar + 
                    m_DeleteScriptName;
                si.UseShellExecute = false;
                System.Diagnostics.Process chmodproc= System.Diagnostics.Process.Start(si);
                chmodproc.WaitForExit();
            } //if

            Console.WriteLine(m_nl +
                "*********************************************************");
            Console.WriteLine("Deletion script file created at: {0}", 
                CurrentSettings.DeletionScriptPath +
                Path.DirectorySeparatorChar +
                m_DeleteScriptName);
        } //try
        catch (Exception e)
        {
            Console.WriteLine("Exception in GenerateFile: {0}", e.ToString());
        } //catch
    } //GenerateFile()

    /// <summary>
    /// This method simply runs the tool with no command line to see if it executes.
    /// </summary>
    /// <param name="commandName"></param>
    /// <returns></returns>
    public static bool ToolInstalled(string commandName)
    {
        try
        {
            ProcessStartInfo psi = new ProcessStartInfo(commandName);
            // Redirect to avoid output to console.
            psi.RedirectStandardError = true;
            psi.RedirectStandardOutput = true;
            Process proc = Process.Start(psi); 
            return true;
        } //try

        // We'll assume that any exception is that the command isn't found.
        // Any exception means the tool isn't working correctly so this is valid. 
        catch 
        {
            return false;
        } //catch

    } // IsTarInstalled

    /// <summary>
    /// Generates the list of files to be deleted.
    /// Uses recursion to walk the entire directory tree.
    /// </summary>
    /// <param name="RemoveDir"></param>
    /// <param name="OutFile"></param>
    /// <param name="FileTable"></param>
    /// <param name="DeleteList"></param>
    public static void CreateDeletionFile(DirectoryInfo RemoveDir,
        StreamWriter OutFile, 
        StringDictionary FileTable, 
        ArrayList DeleteList, 
        string DeleteCommand, 
        Settings CurrentSettings)
    {
        try
        {
            // Iterate over all files

            foreach (DirectoryInfo di in RemoveDir.GetDirectories())
            {
                CreateDeletionFile(di , OutFile, FileTable, DeleteList, DeleteCommand, CurrentSettings);  //Recurse
            }

            foreach (FileInfo fi in RemoveDir.GetFiles())
            {
                string FileName = fi.FullName;
                // If file isn't in list, add for deletion.  
                // Don't delete the .tar/.tgz file if it's in the directory.
                if (!FileTable.ContainsKey(FileName.ToLower()) && 
                    !FileName.ToLower().EndsWith(".tar") &&
                    !FileName.ToLower().EndsWith(".tgz"))
                {
                    DeleteList.Add(FileName);
                    if (CurrentSettings.Verbose)
                    {
                        Console.WriteLine("Added for deletion: {0}", FileName);
                    } //if
                    OutFile.WriteLine(DeleteCommand + FileName);
                } //if
            } //foreach
        } //try
        catch (Exception e)
        {
            Console.WriteLine("Exception in CreateDeletionFile: {0}", e.ToString());
        } //catch
    } //CreateDeletionFile()

    /// <summary>
    /// Gets command-line arguments and returns them in instance of Args class.
    /// </summary>
    /// <param name="args"></param>
    /// <returns></returns>
    public static Args GetArgs(string[] args)
    {
        Args CurrentArgs = new Args();
        try
        {
            if (args.Length > 0 ) //have arguments
            {
                CurrentArgs.Empty = false;
                for (int i=0; i < args.Length; i++) 
                {
                    switch (args[i].Substring(0,2))
                    {
                        case "-?":
                            CurrentArgs.Usage = true;
                            break;

                        case "-d":
                        {
                            if (args[i].Length < 5)  //Invalid path on -d:
                            {
                                Console.WriteLine("Invalid path");
                                CurrentArgs.Error = true;
                            }
                            else
                            {
                                string ScriptPath = args[i].Substring(3, args[i].Length - 3);
                                if (Directory.Exists(ScriptPath))
                                {
                                    // Get rid of trailing directory separator chars.
                                    CurrentArgs.DeletionScriptPath = Path.GetFullPath(ScriptPath);
                                } //if
                                else //bad directory path
                                {
                                    Console.WriteLine("Invalid path");
                                    CurrentArgs.Error = true;
                                }
                            } //else
                            break;
                        } //-d case

                        case "-e":
                        {
                            if (args[i].Length < 5)  //Invalid path on -e=
                            {
                                Console.WriteLine("Invalid path");
                                CurrentArgs.Error = true;
                            } //if
                            else
                            {
                                string DirPath = args[i].Substring(3, args[i].Length - 3);
                                string FullPath = Path.GetFullPath(DirPath);
                                if (FullPath != null)
                                {
                                    string ReturnedFullPath = GetSSCLIRoot(FullPath);
                                    if (ReturnedFullPath == null)
                                    {
                                        CurrentArgs.Error = true;
                                    } //if
                                    else
                                    {
                                        CurrentArgs.SSCLIRootDirectory = ReturnedFullPath;
                                    }
                                } //if
                                else //bad directory path
                                {
                                    Console.WriteLine("Invalid path or filename in -e option");
                                    CurrentArgs.Error = true;
                                } //else
                            } //else
                            break;
                        } //-e case

                        case "-k":
                            CurrentArgs.KillSettings = true;
                            break;

                        case "-t":
                            if (args[i].Length < 5)  //Invalid path or filename on -t:
                            {
                                Console.WriteLine("Invalid path or filename in -t option");
                                CurrentArgs.Error = true;
                            } //if
                            else // Could be a valid path.
                            {
                                string TarFileName = GetTarFileFullPath(args[i].Substring(3, args[i].Length - 3));
                                if (TarFileName == null || !File.Exists(TarFileName))
                                {
                                    Console.WriteLine("Invalid .tar file location.");
                                    CurrentArgs.Error = true;
                                } //if
                                else
                                {
                                    CurrentArgs.TarFileName = TarFileName;
                                } //else
                            } //else
                            break;

                        case "-v":
                            CurrentArgs.Verbose = true;
                            break;

                        default:
                            CurrentArgs.Error = true;
                            Console.WriteLine("Error in command-line: invalid argument");
                            break;
                    } //switch
                }
            } //if
            else //empty args
            {
                CurrentArgs.Empty = true;
            } //else

            if (CurrentArgs.Usage)
            {
                OutputUsage();
            } //if
            else
            {
                if (CurrentArgs.KillSettings)
                {
                    if (File.Exists(m_SettingsFileName))
                    {
                        File.Delete(m_SettingsFileName);
                        Console.WriteLine(m_nl + 
                            "*********************************************************");
                        Console.WriteLine("Deleted settings file." + m_nl +  
                            "Execute cleanbuild again to create a new one.");
                    }
                    else
                    {
                        Console.WriteLine(m_nl +
                            "*********************************************************");
                        Console.WriteLine("No settings file." + m_nl + 
                            "Execute cleanbuild with no options to create one.");
                    }
                    Console.WriteLine("*********************************************************");
                    CurrentArgs.Error = true; //set to Error to exit app
                }
            } //else  

            return CurrentArgs;
        } //try
        catch (Exception e)
        {
            Console.WriteLine("Error in argument parsing {0}", e.ToString());
            CurrentArgs.Error = true;
            return CurrentArgs;
        } //catch
    } //GetArgs

    /// <summary>
    /// Gets the current program settings.
    /// Uses XML serialization to store and read properties
    /// </summary>
    /// <param name="CurrentArgs"></param>
    /// <returns></returns>
    public static Settings GetSettings(Args CurrentArgs)
    {
        XmlSerializer SettingsSerializer = new XmlSerializer(typeof(Settings));
        Settings CurrentSettings = null;
        try
        {
            if (File.Exists(m_SettingsFileName))
            {
                Stream XMLStream = new FileStream(m_SettingsFileName, FileMode.Open);
                CurrentSettings = (Settings) SettingsSerializer.Deserialize(XMLStream);

                if (CurrentSettings.OSType != GetOSType())
                {
                    Console.WriteLine("!!! OS type in settings doesn't match current OS type !!!" + m_nl +
                        "        Deleting settings file and exiting" + m_nl +
                        "          Please re-run this application");
                    XMLStream.Close();
                    File.Delete(m_SettingsFileName);
                    throw new NotSupportedException("OS type in settings doesn't match current OS type");
                } //if

                // if args then override file settings
                if (!CurrentArgs.Empty)
                {
                    if (CurrentArgs.SSCLIRootDirectory != null)
                    {
                        CurrentSettings.SSCLIRootDirectory = CurrentArgs.SSCLIRootDirectory;
                    } //if
                    if (CurrentArgs.DeletionScriptPath != null)
                    {
                        CurrentSettings.DeletionScriptPath = CurrentArgs.DeletionScriptPath;
                    } //if
                    if (CurrentArgs.Verbose == true)
                    {
                        CurrentSettings.Verbose = true;
                    } //if
                    if (CurrentArgs.TarFileName != null)
                    {
                        CurrentSettings.TarFileName = CurrentArgs.TarFileName;
                    } //if
                } //if
        
                Console.WriteLine("Settings file found at {0}", 
                    Environment.CurrentDirectory + 
                    m_sep + 
                    m_SettingsFileName);
                Console.WriteLine(m_nl+ 
                    "Current program settings are:");
                Console.WriteLine("   Build directory path: {0}", CurrentSettings.SSCLIRootDirectory);
                Console.WriteLine("   Tar file location: {0}", CurrentSettings.TarFileName);
                Console.WriteLine("   Deletion script path: {0}", 
                    CurrentSettings.DeletionScriptPath + 
                    m_sep + 
                    m_DeleteScriptName);
                Console.WriteLine("   OS type: {0}", CurrentSettings.OSType);
                Console.WriteLine("   Verbose: {0}" + m_nl, CurrentSettings.Verbose);
                Console.Write("Continue with generating deletion list? 'y' or 'n' ");
                string Reply = Console.ReadLine();
                if (Reply.ToLower() == "n") 
                {
                    CurrentSettings.Error = true;
                    Console.Write(m_nl + "Do you want to erase the settings file? 'y' or 'n' ");
                    Reply = Console.ReadLine();
                    if (Reply.ToLower() == "y")
                    {
                        Console.WriteLine(m_nl + "Deleting settings file {0} and exiting...", m_SettingsFileName);
                        XMLStream.Close();
                        File.Delete(m_SettingsFileName);
                    }
                    Console.WriteLine(m_nl + "Exiting cleanbuild ...");
                }
            } //if
            else  // no settings file
            {
                CurrentSettings = new Settings();
                Console.WriteLine("*********************************************************" + m_nl +
                    "******************* No Settings File ********************" + m_nl +
                    "************ First-time User Input Required *************" + m_nl +
                    "*********************************************************" + m_nl);
                Console.Write("Please input full path to SSCLI root directory: ");
                string response = GetUserInput(true);
                CurrentSettings.SSCLIRootDirectory = GetSSCLIRoot(response);

                while (true)
                {
                    Console.Write(m_nl + "Please input full path (or directory) to SSCLI tar file: ");
                    response = Console.ReadLine();
                    string TarFileName = GetTarFileFullPath(response);
                    if (TarFileName != null)
                    {
                        CurrentSettings.TarFileName = TarFileName;
                        break;
                    } //if
                    else
                    {
                        continue;
                    } //else
                } //while (true)

                Console.Write(m_nl + "Please input full path for deletion batch file: ");
                CurrentSettings.DeletionScriptPath = GetUserInput(true);

                Console.WriteLine("Deletion batch file will be named: {0}", 
                    CurrentSettings.DeletionScriptPath + 
                    Path.DirectorySeparatorChar + 
                    m_DeleteScriptName);
                Console.Write(m_nl + "Verbose output mode? 'y' or 'n' ");
                string Reply = Console.ReadLine();
                if (Reply.ToLower() == "y")
                {
                    CurrentSettings.Verbose = true;
                } //if
                else
                {
                    CurrentSettings.Verbose = false;
                } //else
                CurrentSettings.OSType = GetOSType();
                Stream XMLStream = new FileStream(m_SettingsFileName, FileMode.OpenOrCreate);
                SettingsSerializer.Serialize(XMLStream, CurrentSettings);
                Console.WriteLine("Settings file serialized to {0}", 
                    Environment.CurrentDirectory + 
                    Path.DirectorySeparatorChar + 
                    m_SettingsFileName);
            } //else
            return CurrentSettings;
        }
        catch (Exception e)
        {
            Console.WriteLine("Exception occured in GetSettings() {0}", e.ToString());
            return null;
        } //catch
    } //GetSettings


    /// <summary>
    /// Outputs usage info.
    /// </summary>
    static void OutputUsage()
    {
        Console.WriteLine("Usage: " + m_nl + 
            "       -?                       Output usage" + m_nl +
            "       -d:<deletionfilepath>    Override deletion script path" + m_nl +
            "       -e:<SSCLIrootdirectory>  Override SSCLI root directory path" + m_nl +
            "       -k                       Kill current settings file" + m_nl +
            "       -t:<TarFileName>         override tar file path" + m_nl +
            "       -v                       Verbose output mode");
    } //OutputUsage()

	
    /// <summary>
    /// Get user input.  Will cycle until valid input is entered.
    /// </summary>
    /// CheckDirectory = true if you want directory validity to be checked.  False if not.
    /// <param name="CheckDirectory"></param>
    /// <returns></returns>
    static string GetUserInput(bool CheckDirectory)
    {
        // Loop until user inputs a valid directory or file location.
        while (true)
        {
            string InputStr = Console.ReadLine();
            if (CheckDirectory)
            {
                if (Directory.Exists(InputStr) || File.Exists(InputStr))
                    // Strip off trailing directory separator chars from paths.
                    if (InputStr.EndsWith(m_sep.ToString()))
                    {
                        return InputStr.Substring(0, InputStr.Length - 1);
                    } //if
                    else // directory string okay
                    {
                        return InputStr;
                    } //else
                else
                {
                    Console.Write("Invalid file or directory: {0}, please try again: ", InputStr);
                    continue;
                }
            } //if
            else 
                return InputStr;
        } //while (true)
    } //GetUserInput()

    /// <summary>
    /// Used to find directory above the sscli directory.
    /// </summary>
    /// <param name="InputStr"></param>
    /// <returns></returns>
    static string GetSSCLIRoot(string InputStr)
    {
        // Check for sscli directory.
        if (Directory.Exists(InputStr + m_sep + "sscli") && 
            File.Exists(InputStr + m_sep + "sscli" + m_sep + "env.bat"))
        {
            return InputStr;
        }
        else
        {
            // If the input path ends with sscli then try the parent.
            if (InputStr.ToLower().EndsWith(m_sep + "sscli"))
            {
                return GetSSCLIRoot(Directory.GetParent(InputStr).FullName);
            } //if
            else  // Have a bad directory.
            {
                Console.WriteLine("Invalid sscli directory");
                return null;
            } //else
        } //else
    } //GetSSCLIRoot()
	

    /// <summary>
    /// Gets the full path to a .tar file.  Input can be a file path or a directory which will be searched.
    /// </summary>
    /// <param name="InputStr"></param>
    /// <returns></returns>
    public static string GetTarFileFullPath(string InputStr)
    {
        if (File.Exists(InputStr) || Directory.Exists(InputStr))
        {
            if (InputStr.EndsWith(".tar"))
            {
                return InputStr;  //Assume the full path and file name was input.
            } //if
            else
            {
                DirectoryInfo di = new DirectoryInfo(InputStr);
                FileInfo [] fileArr = di.GetFiles("*.tar");
                if (fileArr.Length > 1)
                {
                    Console.WriteLine(
                        "Too many .tar files found. " + m_nl + 
                        "Please specify exact file name or remove extra .tar files from directory."
                        );
                    return null;
                } //if
                else
                {
                    if (fileArr.Length < 1)
                    {
                        Console.WriteLine("No .tar file found, please try again.");
                        return null;
                    } //if
                } //else
                Console.WriteLine(m_nl + "Found .tar file at: {0}", fileArr[0].FullName);
                return fileArr[0].FullName;
            } //else
        } //if
        else
        {
            Console.WriteLine("Specified path not valid file or directory.");
            return null;
        } //else
    } //GetTarFilePath()

    /// <summary>
    /// Get the OS type
    /// </summary>
    /// <returns>string</returns>
    static OSTypeEnum GetOSType()
    {
        if (Environment.CurrentDirectory.IndexOf(":") > 0 )
        {
            return OSTypeEnum.Windows;
        } //if
        else
        {
            return OSTypeEnum.Unix;
        } //else
    } //GetOSType

} //class

  /// <summary>
  /// Type used to store command-line arguments in parsed form.
  /// </summary>
public class Args
{
    private string m_DeletionScriptPath = null;
    private bool m_Empty = false;
    private bool m_Error = false;
    private bool m_KillSettings = false;
    private string m_SSCLIRootDirectory = null;
    private string m_TarFileName = null;
    private bool m_Usage = false;
    private bool m_Verbose = false;

    public string DeletionScriptPath
    {
        get 
        {
            return m_DeletionScriptPath;
        } //get
        set
        {
            m_DeletionScriptPath = value;
        }
    } //DeletionScriptPath property


    public bool Empty
    {
        get 
        {
            return m_Empty;
        } //get
        set
        {
            m_Empty = value;
        }
    } //Empty property

    public bool Error
    {
        get 
        {
            return m_Error;
        } //get
        set
        {
            m_Error = value;
        }
    } //Error property

    public bool KillSettings
    {
        get 
        {
            return m_KillSettings;
        } //get
        set
        {
            m_KillSettings = value;
        }
    } //KillSettings property


    public string TarFileName
    {
        get 
        {
            return m_TarFileName;
        } //get
        set
        {
            m_TarFileName = value;
        }
    } //TarFileName property

    public string SSCLIRootDirectory
    {
        get 
        {
            return m_SSCLIRootDirectory;
        } //get
        set
        {
            m_SSCLIRootDirectory = value;
        }
    } //SSCLIRootDirectory property

    public bool Usage
    {
        get 
        {
            return m_Usage;
        } //get
        set
        {
            m_Usage = value;
        }
    } //Usage property

    public bool Verbose
    {
        get 
        {
            return m_Verbose;
        } //get
        set
        {
            m_Verbose = value;
        }
    } //Verbose property
} //class

  /// <summary>
  /// Type used to store program settings.
  /// </summary>
public class Settings
{
    private bool m_Error = false;
    private string m_DeletionScriptPath = null;
    private OSTypeEnum m_OSType;
    private string m_SSCLIRootDirectory = null;
    private string m_TarFileName = null;
    private bool m_Verbose = false;

    public string DeletionScriptPath
    {
        get
        {
            return m_DeletionScriptPath;
        } //get
        set
        {
            m_DeletionScriptPath = value;
        } //set
    } //DeletionScriptPath property

    public bool Error
    {
        get 
        {
            return m_Error;
        } //get
        set
        {
            m_Error = value;
        }
    } //Error property

    public OSTypeEnum OSType
    {
        get
        {
            return m_OSType;
        } //get
        set
        {
            m_OSType = value;
        } //set
    } //OSType property

    public string TarFileName
    {
        get 
        {
            return m_TarFileName;
        } //get
        set
        {
            m_TarFileName = value;
        }
    } //TarFileName property


    public string SSCLIRootDirectory
    {
        get 
        {
            return m_SSCLIRootDirectory;
        } //get
        set
        {
            m_SSCLIRootDirectory = value;
        }
    } //SSCLIRootDirectory property

    public bool Verbose
    {
        get 
        {
            return m_Verbose;
        } //get
        set
        {
            m_Verbose = value;
        }
    } //Verbose property
} //class Settings

/// =============================================================================================
/// <summary>
/// ConsoleProcess is a class that allows getting stdout and stderr without deadlocking.
/// This class uses WaitHandles and ThreadPool objects to synchronize access to the std streams.
/// Access to the underlying process is provided by the BaseProcess property.
/// 
/// Currently this is limited to returning all the output at once from a console app that
/// terminates.  It would have to be modified to get interactive console IO.
/// </summary>
/// =============================================================================================
public class ConsoleProcess
{
    /// <summary>
    /// Enum used to specify which type of output is requested.
    /// </summary>
    private enum IOType {Output = 1, Error};

    private string m_ProcessName = null;
    private string m_Arguments = null;
  
    private System.Diagnostics.Process m_Process = null;

    protected string m_StdErr = null;
    protected string m_StdOut = null;
    protected bool m_Started = false;

    StandardIO m_StdOutWorker = null;
    StandardIO m_StdErrWorker = null;

    System.Threading.ManualResetEvent m_StdOutWaitHandle = null;
    System.Threading.ManualResetEvent m_StdErrWaitHandle = null;

    /// <summary>
    /// ConsoleProcess Constructor
    /// </summary>
    /// <param name="ProcessName"></param>
    /// <param name="Arguments"></param>
    public ConsoleProcess(string ProcessName, string Arguments)
    {
        m_ProcessName = ProcessName;
        m_Arguments = Arguments;
        m_Process = new System.Diagnostics.Process();
        m_Process.StartInfo.FileName = m_ProcessName;
        m_Process.StartInfo.Arguments = m_Arguments;
        m_Process.StartInfo.UseShellExecute = false;
        m_Process.StartInfo.RedirectStandardOutput = true;
        m_Process.StartInfo.RedirectStandardError = true;
    } //constructor

    /// <summary>
    /// Starts the process and creates  output worker threads.
    /// Returns the underlying process object which can also be obtained by
    /// BaseProcess property.
    /// </summary>
    /// <returns></returns>
    public System.Diagnostics.Process Start()
    {
        m_StdOutWaitHandle = new ManualResetEvent(false);
        m_StdErrWaitHandle = new ManualResetEvent(false);
    
        //Create objects for worker threads.  
        //These will call back on the this pointer to return the output.
        m_StdOutWorker = new StandardIO(m_Process, IOType.Output, m_StdOutWaitHandle, this);
        m_StdErrWorker = new StandardIO(m_Process, IOType.Error, m_StdErrWaitHandle, this);
        m_Process.Start();
    
        ThreadPool.QueueUserWorkItem(new System.Threading.WaitCallback(m_StdOutWorker.GetOutput));
        ThreadPool.QueueUserWorkItem(new System.Threading.WaitCallback(m_StdErrWorker.GetOutput));
    
        m_Started = true;
        return m_Process;

    } //Execute()

    /// <summary>
    /// Waits until the output worker threads have terminated and set the WaitHandles.
    /// </summary>
    public void WaitForOutput()
    {
        m_Process.WaitForExit();  //Wait for the base process to terminate.
        System.Threading.ManualResetEvent[] HandleArray = {m_StdOutWaitHandle, m_StdErrWaitHandle};
        System.Threading.WaitHandle.WaitAll(HandleArray, 1000000, true);
    } //WaitForOutput()
  
    /// <summary>
    /// Provides access to the underlying process object.
    /// </summary>
    public System.Diagnostics.Process BaseProcess
    {
        get 
        {
            return m_Process;
        } //get
    } //BaseProcess property

    /// <summary>
    /// Used to obtain the standard output content.
    /// Will wait until the worker thread is finished running.
    /// </summary>
    public string StandardOutputString
    {
        get 
        {
            if (!m_Started) 
                throw new System.NotSupportedException("ConsoleProcess error:  not started");
            m_StdOutWaitHandle.WaitOne();
            return m_StdOut;
        } //get
    } //StandardOutput property

    /// <summary>
    /// Used to obtain the standard error content.
    /// Will wait until the worker thread is finished running.
    /// </summary>
    public string StandardErrorString
    {
        get 
        {
            if (!m_Started) 
                throw new System.NotSupportedException("ConsoleProcess error: Process not started");
            m_StdErrWaitHandle.WaitOne();
            return m_StdErr;
        } //get
    } //StandardError property

    /// <summary>
    /// Nested class that is used to create objects to run worker threads.
    /// </summary>
    private class StandardIO
    {
        private System.Diagnostics.Process m_Process = null;
        private const long m_LOOPMAX = 10;
        private const int m_SLEEPTIME = 100;

        private IOType m_IOType;
        private ConsoleProcess m_CallingProcess;

        private System.Threading.ManualResetEvent m_WaitHandle = null;

        /// <summary>
        /// Constructor for StandardIO type.
        /// </summary>
        /// <param name="Process"></param>
        /// <param name="iotype"></param>
        /// <param name="ManResetEvent"></param>
        /// <param name="CallingProcess"></param>
        public StandardIO(System.Diagnostics.Process Process, 
            IOType iotype, 
            System.Threading.ManualResetEvent ManResetEvent,
            ConsoleProcess CallingProcess)
        {
            m_Process = Process;
            m_IOType = iotype;
            m_WaitHandle = ManResetEvent;
            m_CallingProcess = CallingProcess;
        } //constructor

        /// <summary>
        /// GetOutput is the core worker thread method.
        /// It peeks at the process streams and if there is output then exits and returns it all.
        /// If the process exits then it exits after a the loop iterations set in m_LOOPMAX.
        /// </summary>
        /// <param name="InObj"></param>
        public void GetOutput(Object InObj)
        {
            try
            {
                StreamReader IOStream = null;
                if (m_IOType == IOType.Output) //Standard Output
                {
                    lock(m_Process)
                    {
                        IOStream = m_Process.StandardOutput;
                    } //lock
                }
                else
                {
                    if (m_IOType == IOType.Error)  //Standard Error
                    {
                        lock(m_Process)
                        {
                            IOStream = m_Process.StandardError;
                        } //lock
                    } //if
                    else  //bad input
                    {
                        throw new NotSupportedException("Error in StandardIO class: IO type not found");
                    }
                } //else

                int count = 0;
                int test = -1;
                //Main loop to get console output.  Wait for any input or timeout once process has exited.
                while (true)
                {
                    lock(IOStream)
                    {
                        test = IOStream.Peek();
                    } //lock

                    if (test > -1)  //have output, exit loop
                    {
                        break;
                    } //if
                    else
                    {
                        lock(m_Process)
                        {
                            if (m_Process.HasExited) //start the countdown to exit the loop
                                count++;
                        } //lock
                        if (count > m_LOOPMAX)
                        {
                            break;
                        } //if
                        else
                        {
                            Thread.Sleep(m_SLEEPTIME);
                        } //else
                    } //else
                } //for

                if (test > -1)  //exited loop with output
                {
                    lock(m_Process)
                    {
                        if (m_IOType == IOType.Output) //Standard Output
                        {
                            lock(m_CallingProcess)
                            {
                                m_CallingProcess.m_StdOut = m_Process.StandardOutput.ReadToEnd();
                            } //lock
                        } //if
                        else
                        {
                            if (m_IOType == IOType.Error) //Standard Error
                                lock(m_CallingProcess)
                                {
                                    m_CallingProcess.m_StdErr = m_Process.StandardError.ReadToEnd();
                                } //lock
                        } //else
                    } //lock
                } //if
                else  //exited loop through timeout
                {
                    if (m_IOType == IOType.Output) //Standard Output
                        m_CallingProcess.m_StdOut = null;
                    else
                        if (m_IOType == IOType.Error) //Standard Error
                        m_CallingProcess.m_StdErr = null;
                } //else

            } //try
            catch (Exception e)
            {
                Console.WriteLine("Exception in worker thread: {0}", e.ToString());
            } //catch
            finally
            {
                m_WaitHandle.Set();  //Always signal on WaitHandle  to avoid deadlocking.
            }
        } //GetOutput()
    } //class StandardIO
} //class ConsoleProcess

