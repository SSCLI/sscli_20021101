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
//-----------------------------------------------------------------------------
// Various resource Holders
//
// General idea is to have a templatized class who's ctor and dtor call
// allocation management functions. This makes the holders type-safe, and
// the compiler can inline most/all of the holder code.
//-----------------------------------------------------------------------------

#ifndef __HOLDER_H_
#define __HOLDER_H_

//-----------------------------------------------------------------------------
// Smart Pointer
//-----------------------------------------------------------------------------
template <class TYPE>
class ComWrap
{
  private:
    TYPE *m_value;
  public:
    ComWrap<TYPE>() : m_value(NULL) {}
    ComWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~ComWrap<TYPE>() { if (m_value != NULL) m_value->Release(); }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
    TYPE* operator->() { return m_value; }
    const TYPE* operator->() const { return m_value; }
    void Release() { if (m_value != NULL) m_value->Release(); m_value = NULL; }
};

//-----------------------------------------------------------------------------
// wrap new & delete
//-----------------------------------------------------------------------------
template <class TYPE>
class NewWrap
{
  private:
    TYPE *m_value;
  public:
    NewWrap<TYPE>() : m_value(NULL) {}
    NewWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~NewWrap<TYPE>() { if (m_value != NULL) delete m_value; }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
    TYPE* operator->() { return m_value; }
    const TYPE* operator->() const { return m_value; }
};

//-----------------------------------------------------------------------------
// wrap new[] & delete []
//-----------------------------------------------------------------------------
template <class TYPE>
class NewArrayWrap
{
  private:
    TYPE *m_value;
  public:
    NewArrayWrap<TYPE>() : m_value(NULL) {}
    NewArrayWrap<TYPE>(TYPE *value) : m_value(value) {}
    ~NewArrayWrap<TYPE>() { if (m_value != NULL) delete [] m_value; }
    operator TYPE*() { return m_value; }
    TYPE** operator&() { return &m_value; }
    void operator=(TYPE *value) { m_value = value; }
    int operator==(TYPE *value) { return value == m_value; }
    int operator!=(TYPE *value) { return value != m_value; }
};


//-----------------------------------------------------------------------------
// Wrapper, dtor calls a non-member function to cleanup
//----------------------------------------------------------------------------- 
template <class TYPE, void (*DESTROY)(TYPE), TYPE NULLVALUE>
class Wrap
{
  private:
    TYPE m_value;
  public:
    Wrap<TYPE, DESTROY, NULLVALUE>() : m_value(NULLVALUE) {}
    Wrap<TYPE, DESTROY, NULLVALUE>(TYPE value) : m_value(value) {}

    ~Wrap<TYPE, DESTROY, NULLVALUE>() 
    { 
        if (m_value != NULLVALUE) 
            DESTROY(m_value);
    }

    operator TYPE() { return m_value; }
    TYPE* operator&() { return &m_value; }
    void operator=(TYPE value) { m_value = value; }
    int operator==(TYPE value) { return value == m_value; }
    int operator!=(TYPE value) { return value != m_value; }
};


//-----------------------------------------------------------------------------
// Wrapper. Dtor calls a member function for exit
//-----------------------------------------------------------------------------
template <class TYPE>
class ExitWrap
{
public:
    typedef void (TYPE::*FUNCPTR)(void);

    template<FUNCPTR funcExit>
    class Funcs
    {
        TYPE *m_ptr;
    public:
        inline Funcs(TYPE * ptr) : m_ptr(ptr) {  }; 
        inline ~Funcs () { (m_ptr->*funcExit)(); }
    };
};
 

#define EXIT_HOLDER_CLASS(c, f) ExitWrap<c>::Funcs<&c::f>


//-----------------------------------------------------------------------------
// Wrapper, ctor calls an member-function on enter, dtor calls a 
// member-function on exit.
//-----------------------------------------------------------------------------
template <class TYPE>
class EnterExitWrap
{
public:
    typedef void (TYPE::*FUNCPTR)(void);

    template<FUNCPTR funcEnter, FUNCPTR funcExit>
    class Funcs
    {
        TYPE *m_ptr;
    public:
        inline Funcs(TYPE * ptr) : m_ptr(ptr) { (m_ptr->*funcEnter)(); }; 
        inline ~Funcs () { (m_ptr->*funcExit)(); }
    };
};

#define ENTEREXIT_HOLDER_CLASS(c, fEnter, fExit) EnterExitWrap<c>::Funcs<&c::fEnter, &c::fExit>

#endif  // __HOLDER_H_
