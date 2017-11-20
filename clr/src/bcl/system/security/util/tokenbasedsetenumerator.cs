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
// TokenBasedSetEnumerator.cs
//
namespace System.Security.Util {
        
    using System;
    using System.Collections;
    
    internal class TokenBasedSetEnumerator : IEnumerator
    {
        protected TokenBasedSet   m_set;
        protected int             m_currentIndex;
        
        public TokenBasedSetEnumerator(TokenBasedSet set)
        {
            SetData(set);
        }
        
        protected void SetData(TokenBasedSet set)
        {
            m_set = set;
            m_currentIndex = -1;
        }
        
        private bool EndConditionReached()
        {
           return m_set == null || (m_currentIndex > m_set.GetMaxUsedIndex());
        }
        
        // Advances the enumerator to the next element of the enumeration and
        // returns a boolean indicating whether an element is available. Upon
        // creation, an enumerator is conceptually positioned before the first
        // element of the enumeration, and the first call to GetNext brings
        // the first element of the enumeration into view.
        // 
        public virtual bool MoveNext()
        {
            Object perm = null;
            
            while (!EndConditionReached())
            {
                ++m_currentIndex;
                perm = m_set.GetItem(m_currentIndex);
                if (perm != null)
                    return true;
            }
                    
            return false;
        }
        
        // Returns the current element of the enumeration. The returned value is
        // undefined before the first call to GetNext and following a call
        // to GetNext that returned false. Multiple calls to
        // GetObject with no intervening calls to GetNext will return
        // the same object.
        // 
        public virtual Object Current {
            get {
                if (m_currentIndex==-1)
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumNotStarted"));
                if (EndConditionReached())
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumEnded"));
                
                return m_set.GetItem(m_currentIndex);
            }
        }
        
        // Removes the current element from the underlying set of objects. This
        // method will throw a NotSupportedException if the underlying set of
        // objects cannot be modified.
        // 
        public virtual void Remove()
        {
            if (m_currentIndex==-1)
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumNotStarted"));
            if (EndConditionReached())
                throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_EnumEnded"));
            
            m_set.SetItem(m_currentIndex, null);
        }
        
        public int GetCurrentIndex()
        {
            return m_currentIndex;
        }

        public virtual void Reset() {
            m_currentIndex = -1;
        }
    }
}
