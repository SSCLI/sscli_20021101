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
using System.Threading;

class TestClass {
   public struct MyStruct {
       public int x;
       public int y;
   };
   
   public struct Struct1 {
      public byte x;
   };

   public static int Sum(int a, double b, int c, float d, __arglist)
   {
      ArgIterator args = new ArgIterator(__arglist);
      int S = a + (int)b + c + (int)d;
      while(args.GetRemainingCount() > 0)
        {
          if (args.GetNextArgType().Equals(typeof(int).TypeHandle))
          {
              int N = __refvalue(args.GetNextArg(), int);
              Console.WriteLine("int value is "+N);
              S = S + N;
          }
          else if (args.GetNextArgType().Equals(typeof(string).TypeHandle))
          {
              string s = __refvalue(args.GetNextArg(), string);
              Console.WriteLine("string value is "+s);
          }
          else if (args.GetNextArgType().Equals(typeof(MyStruct).TypeHandle))
          {
              MyStruct st = __refvalue(args.GetNextArg(), MyStruct);
              Console.WriteLine("MyStruct value is "+st.x+" "+st.y);
              S = S + 2*st.x + 3*st.y;
          }
          else
             return -1;
        }
      return S;
   }

   public int Sub(int a, int b, __arglist)
   {
     Console.WriteLine("This type: " + this.GetType().ToString());
     return (a  - b);
   }

   public MyStruct SumAndCount(int a, int b, __arglist)
   {
     Console.WriteLine("This type: " + this.GetType().ToString());
     MyStruct t = new MyStruct();
     
    ArgIterator args = new ArgIterator(__arglist);     
    t.x += (a + b);
    t.y += args.GetRemainingCount();

    while(args.GetRemainingCount() > 0)
    {
        if (args.GetNextArgType().Equals(typeof(int).TypeHandle))
        {
            int N = __refvalue(args.GetNextArg(), int);
            Console.WriteLine("int value is "+N);
            t.x = t.x + N;
        }
        else
        if (args.GetNextArgType().Equals(typeof(byte).TypeHandle))
        {
            byte N = __refvalue(args.GetNextArg(), byte);
            Console.WriteLine("byte value is "+N);
            t.x = t.x + N;
        }
        else
        if (args.GetNextArgType().Equals(typeof(short).TypeHandle))
        {
            short N = __refvalue(args.GetNextArg(), short);
            Console.WriteLine("short value is "+N);
            t.x = t.x + N;
        }
        else
        if (args.GetNextArgType().Equals(typeof(long).TypeHandle))
        {
            TypedReference tr = args.GetNextArg();
            Interlocked.Increment(ref __refvalue(tr, long));
            long N = __refvalue(tr, long);
            Console.WriteLine("long value is "+N);
            t.x = t.x + (int)(N % 57);
        }
        else
        if (args.GetNextArgType().Equals(typeof(float).TypeHandle))
        {
            float N = __refvalue(args.GetNextArg(), float);
            Console.WriteLine("float value is "+N);
            t.x = t.x + (int)N;
        }
        else
        if (args.GetNextArgType().Equals(typeof(double).TypeHandle))
        {
            double N = __refvalue(args.GetNextArg(), double);
            Console.WriteLine("double value is "+N);
            t.x = t.x + (int)N;
        }                            
        else
        if (args.GetNextArgType().Equals(typeof(Struct1).TypeHandle))
        {
            Struct1 N = __refvalue(args.GetNextArg(), Struct1);
            Console.WriteLine("Struct1 value is "+N.x);
            t.x = t.x + N.x;
        }                            
        else
            throw new Exception();
     }
     
     return t;
   }

   public static int Main(string[] a) {
     try
     {
       int S =  Sum( 5, 12.0, 6, 13.0f, __arglist());
       if ( S == 36 )
         Console.WriteLine("Test 1 passed");
       else
       {
         Console.WriteLine("Test 1 failed !! Ret: " + S);
         return 1;
       }
     } catch(Exception e)
     {
       Console.WriteLine("Test failed "+e);
       return 1; 
     }
     try
     {
       MyStruct st;
       st.x = 123;
       st.y = 434;
       int S = Sum( 3, 1.0, 2, 8.0f, __arglist(5,st,"aaa",6) );
       
       if ( S == 1573 )
         Console.WriteLine("Test 2 passed");
       else
       {
         Console.WriteLine("Test 2 failed !! Ret: " + S);
         return 1;
       }
     } catch(Exception e)
     {
       Console.WriteLine("Test failed "+e);
       return 1; 
     }
     try
     {
       TestClass A = new TestClass();
       int S = A.Sub( 9,7, __arglist() );
       
       if ( S == 2 )
         Console.WriteLine("Test 3 passed");
       else
       {
         Console.WriteLine("Test 3 failed !! Ret: " + S);
         return 1;
       }
     } catch(Exception e)
     {
       Console.WriteLine("Test failed "+e);
       return 1; 
     }

     try
     {
       TestClass A = new TestClass();
       Struct1 s1;
       s1.x = 43;
       MyStruct S = A.SumAndCount( 9,7, __arglist(3,4,5,(byte)34,(short)45,
          (long)123456789123456789L,(int)1,(long)123456789123456789L,(float)432,s1,(double)42) );

       if ( S.x == 627 && S.y == 11 )
         Console.WriteLine("Test 4 passed");
       else
       {
         Console.WriteLine("Test 4 failed !! Ret: " + S.x + " " + S.y);
         return 1;
       }
     } catch(Exception e)
     {
       Console.WriteLine("Test failed "+e);
       return 1; 
     }
     return 0;
   }
}
