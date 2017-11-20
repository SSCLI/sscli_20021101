using System;

namespace Variety
{
    /// <summary>
    /// Summary description for Class1.
    /// </summary>
    class Variety
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        static void Main(string[] args)
        {
            FooDerived fd;
            Object o;
            String s;
            int i = 28;
            long l= 123456789 * 4;
            Int64 h;
            float f;
            double d;

            string[] a = new string[3];
            // call object ctor
            fd = new FooDerived();

            // initialize array
            a[0] = "OH, ";
            a[1] = "Hello ";
            a[2] = "World";

            // miscellaneous initializations
            s = a[1];
            o = s;
            f = 100.0F;
            d = f + 50.0;
            h = 1234567890 + (long)f + (long)d;

            // undeclared initializations;
            int unused1 = 6;
            string unused2 = "WHAT";
                
            short L1; 
            short L2;
            short L3;

            // iteration (nested loops)       
            for(L1 = 1;L1 < 2; L1++)
            {
                for(L2 = 1; L2 < 3; L2++)
                {	           
                    // control flow
                    if (L2 == 3)
                    {
                        break;
                    }
                    else
                    {
                        for (L3 = 1; L3 < 4; L3++)
                        {
                            int x;
                            x = (L1 * L2 * L3);
                            if (6 % x == 0)
                            {
                                DoSomething( x );
                            }
                        }
                    }
                }
            }


            // set public static member 
            FooDerived.SharedMember = 200 + (int)f;
		
            // call to member subroutine
            fd.SetNonSharedMember( fd.Return_Integer( a[0] + a[1] + a[2] ) + (int)d );

            // recursive function call
            h = fd.F( 17 );

            // call to regular member subroutine    
            fd.ComputeMatrix( 5 );
	 
            // call subroutine with various types;
            fd.Mondo( fd, o, s, i, l, h, f, d ); 

            // generate and handle an exception;
            DoExceptionStuff();
        }

        //
        // function that does nested exception stuff
        //
        static void DoExceptionStuff()
        {
            int c;
            int i = 0;
            try 
            {
                //div by 0 exception
                c = 6/i;
                Console.WriteLine( "**** Error: Should NOT Reach This Code ****" );
            }
            catch (Exception Outer)
            {
                Console.WriteLine( "**** Outer Exception was Handled ****" );
                Console.WriteLine( Outer.ToString() );
                Console.WriteLine();
                try 
                {
                    throw new ArgumentException("Bad Arguments");
                    Console.WriteLine( "**** Error: Should NOT Reach This Code ****" );
                }
                catch (ArgumentException Inner)
                {
                    Console.WriteLine( "**** Inner Exception was Handled ****" );
                    Console.WriteLine( Inner.ToString() );
                    Console.WriteLine();     
                }
            }
            finally
            {
                Console.WriteLine( "**** Leaving Finally ****" );
                Console.WriteLine();
            }
        }
        static FooDerived Return_FooDerived()
        {
            return new FooDerived();
        }
        //
        // function that contains a variety of locals and has control flow
        //
        static String DoSomething( int n )
        {
            FooDerived fd;
            FooBase fb;
            Object o;
            String s;
            int i;
            long l;
            Int64 h;
            float f;
            double d;
            fd = Return_FooDerived();
            fb = fd;
            // control flow 
            switch (n)
            {
                case 1:
                    o = fb.Return_Object();
                    return "One is a Factor";
                case 2:
                    s = fd.Return_String();
                    i = fd.Return_Integer( s );
                    return "Two is a Factor";
                case 3:
                    l = fd.Return_Long();
                    h = fb.Return_Int64();
                    return "Three is a Factor";
                case 6:
                    f = fb.Return_Single();
                    d = fd.Return_Double();
                    return "Six is a Factor";
                default:
                    return "Not a Factor";
            }
        }
    }
    class FooDerived: FooBase
    {
        private int _NonSharedMember;
        public static int SharedMember;

        //
        // ctor
        //
        public FooDerived() : base()
        {
            _NonSharedMember = 0;
        }
        // Getter function
        public int GetNonSharedMember()
        {
            return _NonSharedMember;
        }
        // Setter function
        public void SetNonSharedMember(int i)
        {
            _NonSharedMember = i;
        }

        //
        // recursive function
        //
        public Int64 F( int n )
        {
            if (n < 2)
            {
                return n;
            }
            else 
            {
                return F( n-1 ) + F( n-2 );
            }
        }

        //
        // overridden functions that return various types
        //    
        new public Object Return_Object()
        {
            return "FooDerived";
        }

        new public String Return_String()
        {
            return "FooDerived";
        }

        new public int Return_Integer( String s)
        {
            return base.Return_Integer( s );
        }

        new public double Return_Double()
        {
            return 9876543210.9876543210;
        }
        //
        // subroutine using arrays, loops, and control flow
        //
        public void ComputeMatrix( int rank) 
        {
            long i;
            long j;
            long[] q;
            long count;
            long[] dims;

            // allocate space for arrays
            q = new long[rank];
            dims = new long[rank];

            // create the dimensions
            count = 1;
            for (i = 0; i < rank; i++)
            {
                q[i] = 0;
                dims[i] = (long)System.Math.Pow(2, i);
                count *= dims[i];
            }

            // Initialize Coordinates to locate each 
            // element of the above matrix        
            for (j = 1; j <= count; j++)
            {
                int temp;
                int carry;

                temp = rank-1;
                if (j > 1)
                {
                    q[temp] += 1;
                    if (q[temp] % dims[temp] == 0)
                    {
                        carry = 1;
                    }
                    else 
                    {
                        carry = 0;
                    }

                    // do a carry if necessary
                    do 
                    {
                        q[temp] = 0;
                        temp -= 1;
                                       
                        q[temp] += 1;
                        if (q[temp] % dims[temp] == 0)
                        {
                            carry = 1;
                        }
                        else
                        {
                            carry = 0;
                        }
                    } while (carry > 0 && temp > 0);
                }
            }
        }
    }


    class FooBase
    {

        //
        // ctor
        //
        public FooBase()
        {
        }


        //
        // functions that return various types
        //    
        public object Return_Object()
        {
            return "FooBase";
        }

        public string Return_String()
        {
            return "FooBase";
        }

        public int Return_Integer( string s )
        {
            return s.Length;
        }
        public long Return_Long()
        {
            return 999999;
        }
        public Int64 Return_Int64()
        {
            return 1234567890;
        }

        public float Return_Single()
        {
            return 12345.67F;
        }
        public double Return_Double()
        {
            return 1234567890.0987654321;
        }

        // subroutine containing various types as parameters
        public void Mondo(FooBase fb,
            object o,
            string s,
            int i,
            long l,
            Int64 h,
            float f,
            double d)
        {
            object o1;
            o1 = o;
            o1 = s;
            o1 = i;
            o1 = l;
            o1 = h;
            o1 = f;
            o1 = d;
            o1 = fb;
        }
    }

}


            
// End of File
