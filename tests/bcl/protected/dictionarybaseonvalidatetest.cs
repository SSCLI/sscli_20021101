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
class MyCollection : DictionaryBase {
 public void UseOnValidate(object key, object Val) {
 this.OnValidate(key, Val);
 }
}
class Test {
 public static void Main() {
 int errors = 0;
 int testcases = 0;
 Environment.ExitCode = 100;
 MyCollection MyColl = new MyCollection();
 try {
 testcases++;
 MyColl.UseOnValidate((object)0, (object)5);
 } catch(Exception e) {
 errors++;
 }
 try {
 testcases++;
 MyColl = new MyCollection();
 MyColl.UseOnValidate((object)5, (object)(-2));
 }  catch(Exception e) {
 errors++;
 }
 try {
 testcases++;
 MyColl = new MyCollection();
 MyColl.UseOnValidate((object)0, (object)5);
 } catch(Exception e) {
 errors++;
 }
 try {
 testcases++;
 MyColl = new MyCollection();
 MyColl.UseOnValidate((object)(-1), (object)5);
 Environment.ExitCode = errors;
 } catch(Exception e) {
 errors++;
 }
 }
}
