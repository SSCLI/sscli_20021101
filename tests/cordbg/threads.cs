namespace ThreadTest
{
    using System;
	using System.Threading;
    using System.Globalization;

    //
    // MBase
    //
    abstract class MBase
    {
        public MBase( long number )
        {             
            number_ = number; 

        } // ctor


        public abstract void Run();
    

        public double Result()
        {        
            lock ( this ) 
            {
                return result_;
            }        

        } // Result


        // data members        
        protected long number_;
        protected double result_ = 0.0; 

    }; // MBase


    //
    // FFunction
    //
    class FFunction : MBase
    { 

        public FFunction( long number ) : base( number )
        {            
        } // ctor


        public override void Run()
        {
            lock ( this )
            {
                result_ += _Compute( number_ );
            }

        } // Run


        private double _Compute( long arg )
        {
            if ( (arg == 0) || (arg == 1) )
                return (double)arg;

            else
                return (_Compute( arg - 1 ) + _Compute( arg - 2 ));

        } // _Compute

    } ;// FFunction


    //
    // GFunction
    //
    class GFunction : MBase
    { 
        public GFunction( long number ): base ( number )
        {
        } // ctor


        public override void Run()
        {
            lock( this )
            {
                result_ += _Compute( number_ );
            }
        
        } // Run


        private double _Compute( long arg )
        {
            if ( (arg == 0) || (arg == 1) )
                return (double)arg;

            else
                return (_Compute( arg - 1 ) * _Compute( arg - 2 ));       
    
        } // _Compute

    } // GFunction


    //
    // HFunction
    //
    class HFunction : MBase
    { 
        public HFunction( long number ) : base( number )
        {            
        } // ctor


        public override void Run()
        {
            lock( this )
            {
                result_ += _Compute( number_ );
            }

        } // Run


        private double _Compute( long arg )
        {
            if ( (arg == 0) || (arg == 1) )
                return (double)arg;

            else
                return (_Compute( arg - 1 ) / (_Compute( arg - 2 ) + 1));       
    
        } // _Compute

    } // HFunction


    //
    // Threads (main entry point)
    //
    public class Threads
    {
        public Threads() : this( 250, true ) {}
        
        public Threads( int numthreads, bool exceptionthread )
        {
            _number = 13;
            _initialized = false;
            _numthreads = numthreads;
            _exceptionthread = exceptionthread;
                            
            _f = new FFunction( _number );
            _g = new GFunction( _number );
            _h = new HFunction( _number );
        
            _InitializeThreads();
    
        } // ctor


        public static void Main( String[] args )
        {        
            Threads worker;
        
       
            if ( args.Length > 0 )
            {
                int threads = 250;
                bool exception = true;


                // command-line options:
                //   -t<number> -- number of threads (default is 250)
                //   -n -- no exception thread (default is true)
                for ( int i = 0; i < args.Length; i++ )
                {
                    if ( args[i].Length >= 2 && args[i][0] == '-' )
                    {
                        switch ( args[i][1] )
                        {
                            case 't':
                                threads = Int32.Parse( args[i].Substring( 2, args[i].Length - 2 ), CultureInfo.InvariantCulture );
                                break;
                                
                            case 'n':
                                exception = false;
                                break;
                                
                            default:
                                throw new Exception( "Invalid Command-Line Arguments" );
                        }
                    }
                    else
                           throw new Exception( "Invalid Command-Line Arguments" );
                }
                worker = new Threads( threads, exception );
            }
            else
                worker = new Threads();
                
                   
            worker.DoWork();        
                
        } // main


        public void DoWork()
        {                         
            Thread jthread = null;
            
            if ( _initialized == true )
            {
                double result;
        
              
                try
                {                                          
                    for ( int i = 0; i < _numthreads; i++ )
                    {
                        _t1[i].Start(); 
                        _t2[i].Start(); 
                        _t3[i].Start();
                    }

                    // wait for all the threads to finish...
                    for ( int j = 0; j < _numthreads; j++ )
                    {
                        _t1[j].Join(); 
                        _t2[j].Join(); 
                        _t3[j].Join();
                    }

                    result = (_f.Result() + _g.Result() + _h.Result());
                    Console.Write( "The result is " ); Console.WriteLine( result );                                                                               
                }
                catch( Exception e0 )
                {
                    Console.WriteLine( "Thread(s) FAILED to Start" );
                }            

                // start and run a thread that throws an exception
                if ( _exceptionthread )
                {
                    MyException.MyException jexception;
                    
                    
                    jexception = new MyException.MyException( 16 );
                    jthread = new Thread( new ThreadStart( jexception.Run ) );
                     
                    if ( jthread != null )
                    {
                        jthread.Start();
                        jthread.Join(); 
                    }
                }
            }
            else
                throw new Exception( "Threads FAILED to Initialize" );

        } // DoWork


        private void _InitializeThreads()
        {        
            try
            {
                _t1 = new Thread[_numthreads];
                _t2 = new Thread[_numthreads];
                _t3 = new Thread[_numthreads];
                
                for ( int i = 0; i < _numthreads; i++ )
                {
                    _t1[i] = new Thread( new ThreadStart( _f.Run ) );
                     _t2[i] = new Thread( new ThreadStart( _g.Run ) );    
                    _t3[i] = new Thread( new ThreadStart( _h.Run ) );
            
                } // for
            
                _initialized = true;
            }
            catch ( Exception e0 )
            {           
            }
    
        } // _InitializeThreads
    
        
        // data members
        private long _number;
        private int _numthreads;
        private bool _initialized;
        private bool _exceptionthread;
    
        private FFunction _f;
        private GFunction _g;
        private HFunction _h;
    
        private Thread[] _t1;
        private Thread[] _t2;
        private Thread[] _t3;

    } // Threads

} // ThreadTest

namespace MyException
{
    using System;

    //
    // MyException
    //
    
    class MyException
    {
        public MyException( int entries )
        {
            _entries = entries;
            _Initialize();

        } // ctor
        
    
        public void Run()
        {
            try
            {
                try
                {
                    // generate but dont' handle exception 
                    _Generate_Exception0();
                }
                catch ( Exception exception )
                {
                    Console.WriteLine( "exception caught in MyException::Run()" );
            
                    // generate and handle exception
                    _Generate_Exception1(); 
                }
            }
            finally
            {           
                try
                {
                    // generate an uncaught exception
                    throw new ApplicationException( "Blah" ); 
                }
                catch ( ArithmeticException exception )
                {
                    // shouldn't land here...   
                }
            }

        } // Run
   

        public void _Generate_Exception0()
        {
            _iArray[_entries] = 100;
          
        } // _Generate_Exception0
    
    
        public void _Generate_Exception1()
        {
            // generate and handle exception
            try
            {
                int i = 0;
            
            
                int j = i / i;
            }
            catch ( Exception exception )
            {
                Console.WriteLine( "exception caught in MyException::_Generate_Exception1()" );
            }
                  
        } // _Generate_Exception1


        private void _Initialize()
        {
            _iArray = new int[_entries];
        
        } // _Initialize


        // data members
        private int _entries;    
        private int[] _iArray;

    } // MyException

} // MyException  

