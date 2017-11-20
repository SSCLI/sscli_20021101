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

// This is performance test for raw xml parsing speed. This is a realworld benchmark
// for the quality of the jitted code

using System;
using System.Xml;

class MainApp {

   public static void DoStuff() {
      XmlTextReader r = new XmlTextReader( "hamlet.xml" );
      while ( r.Read());
   }

   public static void Main(string[] args) {

      int iterations = (args.Length!=0)?Int32.Parse(args[0]):100;

      Console.WriteLine("Doing " + iterations.ToString() + " iterations");

      int tstart = Environment.TickCount;

      for (int i = 0; i < iterations; i++) {

          int start = Environment.TickCount;
          DoStuff();
          int end = Environment.TickCount;

          Console.WriteLine(i.ToString() + ". iteration [ms]: " + (end - start).ToString());
      }

      int tend = Environment.TickCount;

      Console.WriteLine("Average [ms]: " + ((double)(tend - tstart) / iterations).ToString());
   }
}
