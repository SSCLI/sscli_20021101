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

    using Microsoft.JScript.Vsa;
    using System;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using Microsoft.Vsa;

    class VsaReference : VsaItem, IVsaReferenceItem{
      
      private String assemblyName;
      private Assembly assembly;
      bool loadFailed;

      internal VsaReference(VsaEngine engine, string itemName)
        : base(engine, itemName, VsaItemType.Reference, VsaItemFlag.None){
        this.assemblyName = itemName; // default to item name
        this.assembly = null;
        this.loadFailed = false;
      }

      public String AssemblyName{
        get{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          return this.assemblyName;
        } 

        set{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          this.assemblyName = value;
          this.isDirty = true;
          this.engine.IsDirty = true;
        }
      }

      internal Assembly Assembly{
        get{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          return this.assembly;
        }
      }

      internal Type GetType(String typeName){
        if (this.assembly == null){
          if (!loadFailed) {
            try {
              this.Load();
            }catch(Exception) {
              loadFailed = true;
            }
          }
          if (this.assembly == null)
            return null;
        }
        Type result = this.assembly.GetType(typeName, false);
        if (result != null && (!result.IsPublic || result.IsDefined(typeof(System.Runtime.CompilerServices.RequiredAttributeAttribute), true)))
          result = null; //Suppress the type if it is not public or if it is a funky C++ type.
        return result;
      }

      internal override void Compile(){
        Compile(true);		
      }
      
      internal bool Compile(bool throwOnFileNotFound){
        try{
          String assemblyFileName = Path.GetFileName(this.assemblyName);
          String alternateName = assemblyFileName + ".dll";
          if (String.Compare(assemblyFileName, "mscorlib.dll", true, CultureInfo.InvariantCulture) == 0 ||
              String.Compare(alternateName, "mscorlib.dll", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(Object).Module.Assembly;
          else if (String.Compare(assemblyFileName, "microsoft.jscript.dll", true, CultureInfo.InvariantCulture) == 0 ||
                   String.Compare(alternateName, "microsoft.jscript.dll", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(VsaEngine).Module.Assembly;
          else if (String.Compare(assemblyFileName, "microsoft.vsa.dll", true, CultureInfo.InvariantCulture) == 0 ||
                   String.Compare(alternateName, "microsoft.vsa.dll", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(IVsaEngine).Module.Assembly;
          else if (String.Compare(assemblyFileName, "system.dll", true, CultureInfo.InvariantCulture) == 0 ||
                   String.Compare(alternateName, "system.dll", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(System.Text.RegularExpressions.Regex).Module.Assembly;
          else{
            String path = this.engine.FindAssembly(this.assemblyName);
            // if not found, look for the file with ".dll" appended to the assembly name
            if (path == null){
              // check for duplicates before we add the ".dll" part
              alternateName = this.assemblyName + ".dll";
              bool fDuplicate = false;
              foreach (Object item in this.engine.Items){
                if (item is VsaReference && String.Compare(((VsaReference)item).AssemblyName, alternateName, true, CultureInfo.InvariantCulture) == 0){
                  fDuplicate = true;
                  break;
                }
              }
              if (!fDuplicate){
                path = this.engine.FindAssembly(alternateName);
                if (path != null)
                  this.assemblyName = alternateName;
              }
            }
            if (path == null){
			        if (throwOnFileNotFound)
  				      throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, new System.IO.FileNotFoundException());
			        else
	  	          return false;
		      	}
            this.assembly = Assembly.LoadFrom(path);
          }
        }catch(VsaException){
          throw;
        }catch(System.BadImageFormatException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(System.IO.FileNotFoundException e){
		      if (throwOnFileNotFound)
  			    throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
		      else
		        return false;
        }catch(System.IO.FileLoadException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(System.ArgumentException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(Exception e){
          throw new VsaException(VsaError.InternalCompilerError, e.ToString(), e);
        }
        if (this.assembly == null){
		      if (throwOnFileNotFound)
  			    throw new VsaException(VsaError.AssemblyExpected, this.assemblyName);
		      else
		        return false;
        }
	    	return true;
      }


      // This method is called at runtime. When this is called this.assemblyName is the
      // actual assembly name ( eg., mscorlib ) rather than the file name ( eg., mscorlib.dll)
      private void Load(){
        try{
          if (String.Compare(this.assemblyName, "mscorlib", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(Object).Module.Assembly;
          else if (String.Compare(this.assemblyName, "Microsoft.JScript", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(VsaEngine).Module.Assembly;
          else if (String.Compare(this.assemblyName, "Microsoft.Vsa", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(IVsaEngine).Module.Assembly;
          else if (String.Compare(this.assemblyName, "System", true, CultureInfo.InvariantCulture) == 0)
            this.assembly = typeof(System.Text.RegularExpressions.Regex).Module.Assembly;
          else{
            this.assembly = Assembly.Load(this.assemblyName);
          }
        }catch(System.BadImageFormatException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(System.IO.FileNotFoundException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(System.ArgumentException e){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName, e);
        }catch(Exception e){
          throw new VsaException(VsaError.InternalCompilerError, e.ToString(), e);
        }
        if (this.assembly == null){
          throw new VsaException(VsaError.AssemblyExpected, this.assemblyName);
        }
      }
    }
}

