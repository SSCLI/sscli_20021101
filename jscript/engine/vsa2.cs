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
//*************************************************************************************
// Vsa2.cs
//
//  Contain the definitons of all the extra interfaces needed to make IActiveScript work
//*************************************************************************************

namespace Microsoft.JScript
{

  using Microsoft.JScript.Vsa;
  using System;
  using Microsoft.Vsa;
  using System.Reflection;
  using System.Reflection.Emit;
  using System.Runtime.InteropServices;

  //*************************************************************************************
  // IEngine2
  //
  //  Implemented by the VsaEngine, provide the extra functionality needed by an
  //  IActiveScript host
  //*************************************************************************************
  public interface IEngine2{

    System.Reflection.Assembly GetAssembly();

    void Run(System.AppDomain domain);

    bool CompileEmpty();
    void RunEmpty();

    void DisconnectEvents();
    void ConnectEvents();
    void RegisterEventSource(String name);
    void Interrupt();
    void InitVsaEngine(String rootMoniker, IVsaSite site);
    IVsaScriptScope GetGlobalScope();
    Module GetModule();
    IVsaEngine Clone(System.AppDomain domain);
    void Restart();
  }

  //*************************************************************************************
  // ISite2
  //
  //  Implemented by the IActiveScript site, provide the extra functionality needed by
  //  the VsaEngine
  //*************************************************************************************
  public interface ISite2{
    Object[] GetParentChain(Object obj);
  }


  //*************************************************************************************
  // VSAITEMTYPE2
  //
  //  Extendion to the types of Item Type available
  //*************************************************************************************
  public enum VSAITEMTYPE2{
    HOSTOBJECT = 0x10,
    HOSTSCOPE,
    HOSTSCOPEANDOBJECT,
    SCRIPTSCOPE,
  }


  //*************************************************************************************
  // IVsaScriptScope
  //
  //  What was known to be a module in the IActiveScript interface. This functionality
  //  is what behavior in IE will use.
  //*************************************************************************************
  public interface IVsaScriptScope : IVsaItem{
    IVsaScriptScope Parent{ get; }

    IVsaItem AddItem(string itemName, VsaItemType type);
    IVsaItem GetItem(string itemName);
    void RemoveItem(string itemName);
    void RemoveItem(IVsaItem item);

    int GetItemCount();
    IVsaItem GetItemAtIndex(int index);
    void RemoveItemAtIndex(int index);
    Object GetObject();

    IVsaItem CreateDynamicItem(string itemName, VsaItemType type);
  }




}
