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
using System.IO;
using System;
using System.Collections;
using System.Threading;
public class Co1721Synchronized_Operations
{
    public static String strName = "ArrayList.Syncronized";
    public static String strTest = "Co1721Synchonized_Operations";
    public static String strPath = "";
    public static String strActiveBugs = "";
    public ArrayList arrList     = null;
    public Hashtable hash        = null;    
    public class MyArrrayList
    {
    }
    public Boolean runTest()
    {
        int iCountErrors = 0;
        int iCountTestcases = 0;
        Console.Error.WriteLine( strName + ": " + strTest + " runTest started..." );
        Console.WriteLine( "ACTIVE BUGS " + strActiveBugs );
        Console.WriteLine( "This test might run for 5 minutes on some machines" );
        arrList = new ArrayList();
        arrList = ArrayList.Synchronized( arrList );
        hash = new Hashtable();								
        hash = (Hashtable) hash.SyncRoot;
        hash = Hashtable.Synchronized( hash );
        ThreadStart delegStartMethod = new ThreadStart( AddElems );
        Thread [] workers = new Thread[100];
        for ( int i = 0; i < workers.Length; i++ )
        {
            workers[i] = new Thread( delegStartMethod );
        }
        for ( int i = 0; i < workers.Length; i++ )
        {
            workers[i].Name = "ThreadID " + i.ToString();
            workers[i].Start();
        }
        while( true )
        {
            Boolean fAliveThreads = false;
            for ( int i = 0; i < workers.Length; i++ )
            {
                if ( workers[i].IsAlive )
                {
                    fAliveThreads = true;
                }
            }
            if ( fAliveThreads == false )
            {
                break;
            }
        }
        iCountTestcases++;
        if ( arrList.Count != workers.Length * strHeroes.Length )
        {
            ++iCountErrors;
            Console.WriteLine( "Err_999, arrList.Count should be " + (workers.Length * strHeroes.Length).ToString() + " but it is " + arrList.Count.ToString() );
        }
        Console.Error.Write( strName );
        Console.Error.Write( ": " );
        if ( iCountErrors == 0 )
        {
            Console.Error.WriteLine( strTest + " iCountTestcases==" + iCountTestcases.ToString() + " paSs" );
            return true;
        }
        else
        {
            Console.WriteLine( strTest + " FAiL");
            Console.Error.WriteLine( strTest + " iCountErrors==" + iCountErrors.ToString() );
            return false;
        }
    }
    public void AddElems()
    {
        int iNumTimesThreadUsed = 0;
        for ( int i = 0; i < strHeroes.Length; i++ )
        {
            Thread curThread = Thread.CurrentThread;
            try
            {
                hash.Add( curThread.Name, null );
            }
            catch ( ArgumentException )
            {
                iNumTimesThreadUsed ++;
            }
            if ( arrList == null ) {  Console.WriteLine( "Err_101, arrList is null" ); }
            if ( arrList.IsSynchronized == false ) {  Console.WriteLine( "Err_102, arrList is not synchronized" ); }
            arrList.Add( strHeroes[i] );
        }
        if ( iNumTimesThreadUsed != strHeroes.Length - 1 )
        {
            Console.WriteLine( "Err_102, seems like one thread added more times then others " + iNumTimesThreadUsed.ToString() );
        }
    }
    public String [] strHeroes = new String[]
        {
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
            "Aquaman",
            "Atom",
            "Batman",
            "Black Canary",
            "Captain America",
            "Captain Atom",
            "Catwoman",
            "Cyborg",
            "Flash",
            "Green Arrow",
            "Green Lantern",
            "Hawkman",
            null,
            "Ironman",
            "Nightwing",
            "Robin",
            "SpiderMan",
            "Steel",
            null,
            "Thor",
            "Wildcat",
            null,
    };
    public static void Main( String[] args )
    {
        Boolean bResult = false;	
        Co1721Synchronized_Operations oCbTest = new Co1721Synchronized_Operations();
        try
        {
            bResult = oCbTest.runTest();
        }
        catch( Exception ex )
        {
            bResult = false;
            Console.WriteLine( strTest + " " + strPath );
            Console.WriteLine( strTest + " E_1000000" );
            Console.WriteLine( strTest + "FAiL: Uncaught exception detected in Main()" );
            Console.WriteLine( strTest + " " + ex.ToString() );
        }
        if ( bResult == true ) Environment.ExitCode = 0; else Environment.ExitCode = 1;
    }
} 
