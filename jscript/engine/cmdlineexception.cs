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
namespace Microsoft.JScript
{

    using System;
    using System.Globalization;
    using Microsoft.JScript;

    public class CmdLineException: Exception{
      private CmdLineError errorCode;
      private string context;
      private CultureInfo culture;

      public CmdLineException(CmdLineError errorCode, CultureInfo culture){
        this.culture = culture;
        this.errorCode = errorCode;
      }

      public CmdLineException(CmdLineError errorCode, string context, CultureInfo culture){
        this.culture = culture;
        this.errorCode = errorCode;
        if (context != "")
          this.context = context;
      }

      public override string Message{
        get{
          string key = this.ResourceKey(this.errorCode);
          string msg;
          msg = JScriptException.Localize(key, this.context, this.culture);
          // convert errorCode to a 4-digit string
          string errorNum = (10000 + (int)this.errorCode).ToString().Substring(1);
          return "fatal error JS" + errorNum + ": " + msg;
        }
      }

      public string ResourceKey(CmdLineError errorCode){
        switch (errorCode){
          case CmdLineError.AssemblyNotFound: return "Assembly not found";
          case CmdLineError.CannotCreateEngine: return "Cannot create JScript engine";
          case CmdLineError.CompilerConstant: return "Compiler constant";
          case CmdLineError.DuplicateFileAsSourceAndAssembly: return "Duplicate file as source and assembly";
          case CmdLineError.DuplicateResourceFile: return "Duplicate resource file";
          case CmdLineError.DuplicateResourceName: return "Duplicate resource name";
          case CmdLineError.DuplicateSourceFile: return "Duplicate source file";
          case CmdLineError.ErrorSavingCompiledState: return "Error saving compiled state";
          case CmdLineError.IncompatibleTargets: return "Incompatible targets";
          case CmdLineError.InvalidAssembly: return "Invalid assembly";
          case CmdLineError.InvalidCharacters: return "Invalid characters";
          case CmdLineError.InvalidCodePage: return "Invalid code page";
          case CmdLineError.InvalidDefinition: return "Invalid definition";
          case CmdLineError.InvalidForCompilerOptions: return "Invalid for CompilerOptions";
          case CmdLineError.InvalidLocaleID: return "Invalid Locale ID";
          case CmdLineError.InvalidTarget: return "Invalid target";
          case CmdLineError.InvalidSourceFile: return "Invalid source file";
          case CmdLineError.InvalidVersion: return "Invalid version";
          case CmdLineError.InvalidWarningLevel: return "Invalid warning level";
          case CmdLineError.MultipleOutputNames: return "Multiple output filenames";
          case CmdLineError.MultipleTargets: return "Multiple targets";
          case CmdLineError.MultipleWin32Resources: return "Multiple win32resources";
          case CmdLineError.MissingDefineArgument: return "Missing define argument";
          case CmdLineError.MissingExtension: return "Missing extension";
          case CmdLineError.MissingLibArgument: return "Missing lib argument";
          case CmdLineError.MissingReference: return "Missing reference";
          case CmdLineError.ManagedResourceNotFound: return "Managed resource not found";
          case CmdLineError.NestedResponseFiles: return "Nested response files";
          case CmdLineError.NoCodePage: return "No code page";
          case CmdLineError.NoFileName: return "No filename";
          case CmdLineError.NoInputSourcesSpecified: return "No input sources specified";
          case CmdLineError.NoLocaleID: return "No Locale ID";
          case CmdLineError.NoWarningLevel: return "No warning level";
          case CmdLineError.ResourceNotFound: return "Resource not found";
          case CmdLineError.SourceFileTooBig: return "Source file too big";
          case CmdLineError.SourceNotFound: return "Source not found";
          case CmdLineError.UnknownOption: return "Unknown option";
        }
        return "No description available";
      }
    }
}
