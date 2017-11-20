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
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Http;

public interface IHello
{
  string message(string s, Object o);
}


public class Hello : MarshalByRefObject, IHello
{
  public Hello()
  {
  }
  public string message(string s, Object o)
  {
      return s + " " + o.ToString() + "!";
  }
}

class My
{
    static void Main(string[] args)
    {
        ChannelServices.RegisterChannel(new HttpChannel(4398));
        RemotingConfiguration.RegisterWellKnownServiceType(
            typeof(Hello),"hello",WellKnownObjectMode.Singleton);

        Object obj = RemotingServices.Connect(
               typeof(IHello),"http://localhost:4398/hello");

        Console.WriteLine(((IHello)obj).message("Hello","world"));
    }
}

