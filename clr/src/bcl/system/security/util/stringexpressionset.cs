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
// StringExpressionSet
//
 
namespace System.Security.Util {    
    using System.Text;
    using System;
    using System.Collections;
    using System.Runtime.CompilerServices;
    using System.Globalization;

    [Serializable]
    internal class StringExpressionSet
    {
        protected static ArrayList m_emptyList = new ArrayList( 1 );
        
        protected ArrayList m_list;
        protected bool m_ignoreCase;
        protected String m_expressions;
        protected String[] m_expressionsArray;
        protected bool m_throwOnRelative;
        
        protected static readonly char[] m_separators = { ';' };
        protected static readonly char[] m_trimChars = { ' ' };
#if !PLATFORM_UNIX
        protected static readonly char m_directorySeparator = '\\';
        protected static readonly char m_alternateDirectorySeparator = '/';
#else
        protected static readonly char m_directorySeparator = '/';
        protected static readonly char m_alternateDirectorySeparator = '\\';
#endif // !PLATFORM_UNIX
        
        public StringExpressionSet()
            : this( true, null, false )
        {
        }
        
        public StringExpressionSet( String str )
            : this( true, str, false )
        {
        }
        
        public StringExpressionSet( bool ignoreCase )
            : this( ignoreCase, null, false )
        {
        }
        
        public StringExpressionSet( bool ignoreCase, bool throwOnRelative )
            : this( ignoreCase, null, throwOnRelative )
        {
        }
        
        public StringExpressionSet( bool ignoreCase, String str )
            : this( ignoreCase, str, false )
        {
        }

        public StringExpressionSet( bool ignoreCase, String str, bool throwOnRelative )
        {
            m_list = null;
            m_ignoreCase = ignoreCase;
            m_throwOnRelative = throwOnRelative;
            if (str == null)
                m_expressions = null;
            else
            AddExpressions( str );
        }

        protected virtual StringExpressionSet CreateNewEmpty()
        {
            return new StringExpressionSet();
        }
        
        public virtual StringExpressionSet Copy()
        {
            StringExpressionSet copy = CreateNewEmpty();
            if (this.m_list != null)
                copy.m_list = new ArrayList( this.m_list );
            copy.m_expressions = this.m_expressions;
            copy.m_ignoreCase = this.m_ignoreCase;
            copy.m_throwOnRelative = this.m_throwOnRelative;
            return copy;
        }
        
        public void SetIgnoreCase( bool ignoreCase )
        {
            this.m_ignoreCase = ignoreCase;
        }
        
        public void SetThrowOnRelative( bool throwOnRelative )
        {
            this.m_throwOnRelative = throwOnRelative;
        }

        public void SetExpressions( String str )
        {
            m_list = null;
            AddExpressions( str );
        }

        protected virtual String ProcessWholeString( String str )
        {
            return str.Replace( m_alternateDirectorySeparator, m_directorySeparator );
        }

        protected virtual String ProcessSingleString( String str )
        {
            return str.Trim( m_trimChars );
        }
        
        public void AddExpressions( String str )
        {
            if (str == null)
            {
                throw new ArgumentNullException( "str" );
            }

            str = ProcessWholeString( str );
           
            if (m_expressions == null)
            {
                m_expressions = str;
            }
            else
            {
                m_expressions = m_expressions + m_separators[0] + str;
            }

            m_expressionsArray = null;
            
            if (m_list != null || m_throwOnRelative)
            {
                String[] arystr = Split( str );
            
                m_list = new ArrayList();
            
                for (int index = 0; index < arystr.Length; ++index)
                {
                    if (arystr[index] != null && !arystr[index].Equals( "" ))
                    {
                        String temp = ProcessSingleString( arystr[index] );
                        int indexOfNull = temp.IndexOf( '\0' );

                        if (indexOfNull != -1)
                            temp = temp.Substring( 0, indexOfNull );
    
                        if (temp != null && !temp.Equals( "" ))
                        {
                            if (m_throwOnRelative)
                            {
#if !PLATFORM_UNIX
                                if (!((temp.Length >= 3 && temp[1] == ':' && temp[2] == '\\' &&
                                       ((temp[0] >= 'a' && temp[0] <= 'z') || (temp[0] >= 'A' && temp[0] <= 'Z'))) ||
                                      (temp.Length >= 2 && temp[0] == '\\' && temp[1] == '\\')))
#else
                                if(!(temp.Length >= 1 && temp[0] == m_directorySeparator))
#endif // !PLATFORM_UNIX
                                {
                                    throw new ArgumentException( Environment.GetResourceString( "Argument_AbsolutePathRequired" ) );
                                }

                                temp = CanonicalizePath( temp );
                            }

                            m_list.Add( temp );
                        }
                    }
                }
                
                Reduce();
           }
                
        }

        public void AddExpressions( String[] str )
        {
            AddExpressions( str, true, true );
        }

        public void AddExpressions( String[] str, bool checkForDuplicates, bool needFullPath )
        {
            BCLDebug.Assert( m_throwOnRelative, "This should only be called when throw on relative is set" );

            if (str == null)
            {
                throw new ArgumentNullException( "str" );
            }

            m_expressionsArray = null;
            m_expressions = null;

            String[] expressionsArray = new String[str.Length];

            if (m_list == null)
                m_list = new ArrayList();
        
            for (int index = 0; index < expressionsArray.Length; ++index)
            {
                if (str[index] == null)
                    throw new ArgumentNullException( "str" );

                expressionsArray[index] = ProcessWholeString( str[index] );

                if (expressionsArray[index] != null && !expressionsArray[index].Equals( "" ))
                {
                    String temp = ProcessSingleString( expressionsArray[index] );

                    int indexOfNull = temp.IndexOf( '\0' );

                    if (indexOfNull != -1)
                        temp = temp.Substring( 0, indexOfNull );

                    if (temp != null && !temp.Equals( "" ))
                    {
                        if (m_throwOnRelative)
                        {
#if !PLATFORM_UNIX                            
                            if (!((temp.Length >= 3 && temp[1] == ':' && temp[2] == '\\' && 
                                   ((temp[0] >= 'a' && temp[0] <= 'z') || (temp[0] >= 'A' && temp[0] <= 'Z'))) ||
                                  (temp.Length >= 2 && temp[0] == '\\' && temp[1] == '\\')))
#else
                            if(!(temp.Length >= 1 && temp[0] == m_directorySeparator))
#endif // !PLATFORM_UNIX
                            {
                                throw new ArgumentException( Environment.GetResourceString( "Argument_AbsolutePathRequired" ) );
                            }

                            temp = CanonicalizePath( temp, needFullPath );

                        }

                        m_list.Add( temp );
                    }
                }
            }
            
            if (checkForDuplicates)
                Reduce();
                
        }
            
        protected void CheckList()
        {
            if (m_list == null && m_expressions != null)
            {
                CreateList();
            }
        }
        
        protected String[] Split( String expressions )
        {
            if (m_throwOnRelative)
            {
                ArrayList tempList = new ArrayList();

                String[] quoteSplit = expressions.Split( '\"' );

                for (int i = 0; i < quoteSplit.Length; ++i)
                {
                    if (i % 2 == 0)
                    {
                        String[] semiSplit = quoteSplit[i].Split( ';' );

                        for (int j = 0; j < semiSplit.Length; ++j)
                        {
                            if (semiSplit[j] != null && !semiSplit[j].Equals( "" ))
                                tempList.Add( semiSplit[j] );
                        }
                    }
                    else
                    {
                        tempList.Add( quoteSplit[i] );
                    }
                }

                String[] finalArray = new String[tempList.Count];

                IEnumerator enumerator = tempList.GetEnumerator();

                int index = 0;
                while (enumerator.MoveNext())
                {
                    finalArray[index++] = (String)enumerator.Current;
                }

                return finalArray;
            }
            else
            {
                return expressions.Split( m_separators );
            }
        }

        
        protected void CreateList()
        {
            String[] expressionsArray = Split( m_expressions );
            
            m_list = new ArrayList();
            
            for (int index = 0; index < expressionsArray.Length; ++index)
            {
                if (expressionsArray[index] != null && !expressionsArray[index].Equals( "" ))
                {
                    String temp = ProcessSingleString( expressionsArray[index] );

                    int indexOfNull = temp.IndexOf( '\0' );

                    if (indexOfNull != -1)
                        temp = temp.Substring( 0, indexOfNull );

                    if (temp != null && !temp.Equals( "" ))
                    {
                        if (m_throwOnRelative)
                        {
#if !PLATFORM_UNIX
                            if (!((temp.Length >= 3 && temp[1] == ':' && temp[2] == '\\' && 
                                   ((temp[0] >= 'a' && temp[0] <= 'z') || (temp[0] >= 'A' && temp[0] <= 'Z'))) ||
                                  (temp.Length >= 2 && temp[0] == '\\' && temp[1] == '\\')))
#else
                            if(!(temp.Length >= 1 && temp[0] == m_directorySeparator))
#endif // !PLATFORM_UNIX
                            {
                                throw new ArgumentException( Environment.GetResourceString( "Argument_AbsolutePathRequired" ) );
                            }

                            temp = CanonicalizePath( temp );
                        }
                        
                        m_list.Add( temp );
                    }
                }
            }
        }
        
        public bool IsEmpty()
        {
            if (m_list == null)
            {
                return m_expressions == null;
            }
            else
            {
                return m_list.Count == 0;
            }
        }
        
        public bool IsSubsetOf( StringExpressionSet ses )
        {
            if (this.IsEmpty())
                return true;
            
            if (ses == null || ses.IsEmpty())
                return false;
            
            CheckList();
            ses.CheckList();
            
            for (int index = 0; index < this.m_list.Count; ++index)
            {
                if (!StringSubsetStringExpression( (String)this.m_list[index], ses, m_ignoreCase ))
                {
                    return false;
                }
            }
            return true;
        }
        
        public bool IsSubsetOfPathDiscovery( StringExpressionSet ses )
        {
            if (this.IsEmpty())
                return true;
            
            if (ses == null || ses.IsEmpty())
                return false;
            
            CheckList();
            ses.CheckList();
            
            for (int index = 0; index < this.m_list.Count; ++index)
            {
                if (!StringSubsetStringExpressionPathDiscovery( (String)this.m_list[index], ses, m_ignoreCase ))
                {
                    return false;
                }
            }
            return true;
        }

        
        public StringExpressionSet Union( StringExpressionSet ses )
        {
            // If either set is empty, the union represents a copy of the other.
            
            if (ses == null || ses.IsEmpty())
                return this.Copy();
    
            if (this.IsEmpty())
                return ses.Copy();
            
            CheckList();
            ses.CheckList();
            
            // Perform the union
            // note: insert smaller set into bigger set to reduce needed comparisons
            
            StringExpressionSet bigger = ses.m_list.Count > this.m_list.Count ? ses : this;
            StringExpressionSet smaller = ses.m_list.Count <= this.m_list.Count ? ses : this;
    
            StringExpressionSet unionSet = bigger.Copy();
            
            unionSet.Reduce();
            
            for (int index = 0; index < smaller.m_list.Count; ++index)
            {
                unionSet.AddSingleExpressionNoDuplicates( (String)smaller.m_list[index] );
            }
            
            unionSet.GenerateString();
            
            return unionSet;
        }
            
        
        public StringExpressionSet Intersect( StringExpressionSet ses )
        {
            // If either set is empty, the intersection is empty
            
            if (this.IsEmpty() || ses == null || ses.IsEmpty())
                return CreateNewEmpty();
            
            CheckList();
            ses.CheckList();
            
            // Do the intersection for real
            
            StringExpressionSet intersectSet = CreateNewEmpty();
            
            for (int this_index = 0; this_index < this.m_list.Count; ++this_index)
            {
                for (int ses_index = 0; ses_index < ses.m_list.Count; ++ses_index)
                {
                    if (StringSubsetString( (String)this.m_list[this_index], (String)ses.m_list[ses_index], m_ignoreCase ))
                    {
                        if (intersectSet.m_list == null)
                        {
                            intersectSet.m_list = new ArrayList();
                        }
                        intersectSet.m_list.Add( this.m_list[this_index] );
                    }
                    else if (StringSubsetString( (String)ses.m_list[ses_index], (String)this.m_list[this_index], m_ignoreCase ))
                    {
                        if (intersectSet.m_list == null)
                        {
                            intersectSet.m_list = new ArrayList();
                        }
                        intersectSet.m_list.Add( ses.m_list[ses_index] );
                    }
                }
            }
            
            intersectSet.GenerateString();
            
            return intersectSet;
        }
        
        protected void GenerateString()
        {
            if (m_list != null)
            {
                StringBuilder sb = new StringBuilder();
            
                IEnumerator enumerator = this.m_list.GetEnumerator();
                bool first = true;
            
                while (enumerator.MoveNext())
                {
                    if (!first)
                        sb.Append( m_separators[0] );
                    else
                        first = false;
                            
                    String currentString = (String)enumerator.Current;
                    int indexOfSeparator = currentString.IndexOf( m_separators[0] );

                    if (indexOfSeparator != -1)
                        sb.Append( '\"' );

                    sb.Append( currentString );

                    if (indexOfSeparator != -1)
                        sb.Append( '\"' );
                }
            
                m_expressions = sb.ToString();
            }
            else
            {
                m_expressions = null;
            }
        }            
        
        public override String ToString()
        {
            CheckList();
        
            Reduce();
        
            GenerateString();
                            
            return m_expressions;
        }

        public String[] ToStringArray()
        {
            if (m_expressionsArray == null && m_list != null)
            {
                m_expressionsArray = new String[m_list.Count];

                IEnumerator enumerator = m_list.GetEnumerator();

                int count = 0;
                while (enumerator.MoveNext())
                {
                    m_expressionsArray[count++] = (String)enumerator.Current;
                }
            }

            return m_expressionsArray;
        }
                
        
        //-------------------------------
        // protected static helper functions
        //-------------------------------
        
        
        protected bool StringSubsetStringExpression( String left, StringExpressionSet right, bool ignoreCase )
        {
            for (int index = 0; index < right.m_list.Count; ++index)
            {
                if (StringSubsetString( left, (String)right.m_list[index], ignoreCase ))
                {
                    return true;
                }
            }
            return false;
        }
        
        protected static bool StringSubsetStringExpressionPathDiscovery( String left, StringExpressionSet right, bool ignoreCase )
        {
            for (int index = 0; index < right.m_list.Count; ++index)
            {
                if (StringSubsetStringPathDiscovery( left, (String)right.m_list[index], ignoreCase ))
                {
                    return true;
                }
            }
            return false;
        }

        
        protected virtual bool StringSubsetString( String left, String right, bool ignoreCase )
        {
            if (right == null || left == null || right.Length == 0 || left.Length == 0 ||
                right.Length > left.Length)
            {
                return false;
            }
            else if (right.Length == left.Length)
            {
                // if they are equal in length, just do a normal compare
                return String.Compare( right, left, ignoreCase, CultureInfo.InvariantCulture) == 0;
            }
            else if (left.Length - right.Length == 1 && left[left.Length-1] == m_directorySeparator)
            {
                return String.Compare( left, 0, right, 0, right.Length, ignoreCase, CultureInfo.InvariantCulture) == 0;
            }
            else if (right[right.Length-1] == m_directorySeparator)
            {
                // right is definitely a directory, just do a substring compare
                return String.Compare( right, 0, left, 0, right.Length-1, ignoreCase, CultureInfo.InvariantCulture) == 0;
            }
            else if (left[right.Length] == m_directorySeparator)
            {
                // left is hinting at being a subdirectory on right, do substring compare to make find out
                return String.Compare( right, 0, left, 0, right.Length, ignoreCase, CultureInfo.InvariantCulture ) == 0;
            }
            else
            {
                return false;
            }
        }

        protected static bool StringSubsetStringPathDiscovery( String left, String right, bool ignoreCase )
        {
            if (right == null || left == null || right.Length == 0 || left.Length == 0)
            {
                return false;
            }
            else if (right.Length == left.Length)
            {
                // if they are equal in length, just do a normal compare
                return String.Compare( right, left, ignoreCase, CultureInfo.InvariantCulture) == 0;
            }
            else
            {
                String shortString, longString;

                if (right.Length < left.Length)
                {
                    shortString = right;
                    longString = left;
                }
                else
                {
                    shortString = left;
                    longString = right;
                }

                if (String.Compare( shortString, 0, longString, 0, shortString.Length, ignoreCase, CultureInfo.InvariantCulture) != 0)
                {
                    return false;
                }

#if !PLATFORM_UNIX
                if (shortString.Length == 3 &&
                    shortString.EndsWith( ":\\" ) &&
                    ((shortString[0] >= 'A' && shortString[0] <= 'Z') ||
                    (shortString[0] >= 'a' && shortString[0] <= 'z')))
#else
                if (shortString.Length == 1 && shortString[0]== m_directorySeparator)
#endif // !PLATFORM_UNIX
                     return true;

                return longString[shortString.Length] == m_directorySeparator;
            }
        }

        
        //-------------------------------
        // protected helper functions
        //-------------------------------
        
        protected void AddSingleExpressionNoDuplicates( String expression )
        {
            int index = 0;
            
            m_expressionsArray = null;
            m_expressions = null;

            if (this.m_list == null)
                this.m_list = new ArrayList();

            while (index < this.m_list.Count)
            {
                if (StringSubsetString( (String)this.m_list[index], expression, m_ignoreCase ))
                {
                    this.m_list.RemoveAt( index );
                }
                else if (StringSubsetString( expression, (String)this.m_list[index], m_ignoreCase ))
                {
                    return;
                }
                else
                {
                    index++;
                }
            }
            this.m_list.Add( expression );
        }
    
        protected void Reduce()
        {
            CheckList();
            
            if (this.m_list == null)
                return;
            
            int j;

            for (int i = 0; i < this.m_list.Count - 1; i++)
            {
                j = i + 1;
                
                while (j < this.m_list.Count)
                {
                    if (StringSubsetString( (String)this.m_list[j], (String)this.m_list[i], m_ignoreCase ))
                    {
                        this.m_list.RemoveAt( j );
                    }
                    else if (StringSubsetString( (String)this.m_list[i], (String)this.m_list[j], m_ignoreCase ))
                    {
                        // write the value at j into position i, delete the value at position j and keep going.
                        this.m_list[i] = this.m_list[j];
                        this.m_list.RemoveAt( j );
                        j = i + 1;
                    }
                    else
                    {
                        j++;
                    }
                }
            }
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetLongPathName( String path );

        internal static String CanonicalizePath( String path )
        {
            return CanonicalizePath( path, true );
        }

        internal static String CanonicalizePath( String path, bool needFullPath )
        {

#if !PLATFORM_UNIX
            if (path.IndexOf( '~' ) != -1)
                path = GetLongPathName( path );

            if (path.IndexOf( ':', 2 ) != -1)
                throw new NotSupportedException( Environment.GetResourceString( "Argument_PathFormatNotSupported" ) );
#endif // !PLATFORM_UNIX               

            if (needFullPath)
            {
                String newPath = System.IO.Path.GetFullPathInternal( path );
                if (path.EndsWith( m_directorySeparator + "." ))
                {
                    if (newPath.EndsWith( m_directorySeparator ))
                    {
                        newPath += ".";
                    }
                    else
                    {
                        newPath += m_directorySeparator + ".";
                    }
                }                
                return newPath;
            }
            else
                return path;
        }


    }
}
