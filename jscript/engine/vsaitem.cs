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
    using System.Collections;
    using System.Globalization;
    using Microsoft.Vsa;

    public abstract class VsaItem : IVsaItem{
      protected string name;
      internal string codebase;
      internal VsaEngine engine;
      protected VsaItemType type;
      protected VsaItemFlag flag;
      protected bool isDirty;

      internal VsaItem(VsaEngine engine, string itemName, VsaItemType type, VsaItemFlag flag){ 
        this.engine = engine;
        this.type = type;
        this.name = itemName;
        this.flag = flag;
        this.codebase = null;
        this.isDirty = true;
      }

      internal virtual void CheckForErrors(){
      }

      internal virtual void Close(){
        this.engine = null;
      }

      internal virtual void Compile(){}

	  	
      internal virtual Type GetCompiledType(){
        // only code items have a compiled type
        return null;
      }

      public virtual bool IsDirty{
        get{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          return this.isDirty;
        }
        set{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          this.isDirty = value;
        }
      }

      public virtual Object GetOption(String name){
        if (this.engine == null)
          throw new VsaException(VsaError.EngineClosed);
        if (0 == String.Compare(name, "codebase", true, CultureInfo.InvariantCulture))
          return this.codebase;
        throw new VsaException(VsaError.OptionNotSupported);
      }

      public virtual String Name{ 
        get{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          return this.name;
        }

        set{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          if (this.name == value)
            return;
          if (!this.engine.IsValidIdentifier(value))
            throw new VsaException(VsaError.ItemNameInvalid);
          foreach (IVsaItem item in this.engine.Items){
            if (item.Name.Equals(value))
              throw new VsaException(VsaError.ItemNameInUse);
          }
          this.name = value;
          this.isDirty = true;
          this.engine.IsDirty = true;
        }
      }

      internal virtual void Remove(){
        this.engine = null;
      }

      internal virtual void Reset(){}

      internal virtual void Run(){}

      public virtual void SetOption(String name, Object value){
        if (this.engine == null)
          throw new VsaException(VsaError.EngineClosed);
        if (0 == String.Compare(name, "codebase", true, CultureInfo.InvariantCulture))
          this.codebase = (string)value;
        else
          throw new VsaException(VsaError.OptionNotSupported);
        this.isDirty = true;
        this.engine.IsDirty = true;
      }

      public VsaItemType ItemType{ 
        get{
          if (this.engine == null)
            throw new VsaException(VsaError.EngineClosed);
          return this.type;
        }
      }

    }

}
