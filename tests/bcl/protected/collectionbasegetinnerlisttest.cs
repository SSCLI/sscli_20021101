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
using System.Collections;
using System.IO;
class MyCollection : CollectionBase {
 public bool UseInnerList(ArrayList arrList) {
 if(this.InnerList.Count == 0) {
 return true;
 }
 return false;
 }
}
class Test {
 public static void Main() {
 int errors = 0;
 int testcases = 0;
 testcases++;
 ArrayList arrList = new ArrayList();
 MyCollection MyColl = new MyCollection();
 if(! MyColl.UseInnerList(arrList))
   errors++;
 Environment.ExitCode = errors;
 }
}
