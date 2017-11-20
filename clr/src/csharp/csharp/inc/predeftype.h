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
// ===========================================================================
// File: predeftyp.h
//
// Contains a list of the various predefined types.
// ===========================================================================

#if !defined(PREDEFTYPEDEF)
#error Must define PREDEFTYPEDEF macro before including predeftyp.h
#endif

// The "required" column is very important. It means that the compiler requires that type to
// be present in any compilation. It should only be set for types we know will be on any
// platform. If not set, then the compiler may only require the type for certain constructs.


//         id            full type name       required  simple numer  class struct iface  delegat fund type   elementtype,      nice name,    zero,quasi simple numer,  attribute arg size serialization type,  predef attribute, key-sig)
PREDEFTYPEDEF(PT_BYTE,   "System.Byte",         1,            1,      1,     0,   1,   0,       0,   FT_U1,   ELEMENT_TYPE_U1,      L"byte",      0,                 0,      1,      SERIALIZATION_TYPE_U1,      PA_COUNT, "0")
PREDEFTYPEDEF(PT_SHORT,  "System.Int16",        1,            1,      1,     0,   1,   0,       0,   FT_I2,   ELEMENT_TYPE_I2,      L"short",     0,                 0,      2,      SERIALIZATION_TYPE_I2,      PA_COUNT, "1")
PREDEFTYPEDEF(PT_INT,    "System.Int32",        1,            1,      1,     0,   1,   0,       0,   FT_I4,   ELEMENT_TYPE_I4,      L"int",       0,                 0,      4,      SERIALIZATION_TYPE_I4,      PA_COUNT, "2")
PREDEFTYPEDEF(PT_LONG,   "System.Int64",        1,            1,      1,     0,   1,   0,       0,   FT_I8,   ELEMENT_TYPE_I8,      L"long",      &longZero,         0,      8,      SERIALIZATION_TYPE_I8,      PA_COUNT, "3")
PREDEFTYPEDEF(PT_FLOAT,  "System.Single",       1,            1,      1,     0,   1,   0,       0,   FT_R4,   ELEMENT_TYPE_R4,      L"float",     &doubleZero,       0,      4,      SERIALIZATION_TYPE_R4,      PA_COUNT, "4")
PREDEFTYPEDEF(PT_DOUBLE, "System.Double",       1,            1,      1,     0,   1,   0,       0,   FT_R8,   ELEMENT_TYPE_R8,      L"double",    &doubleZero,       0,      8,      SERIALIZATION_TYPE_R8,      PA_COUNT, "5")
PREDEFTYPEDEF(PT_DECIMAL,"System.Decimal",      0,            1,      1,     0,   1,   0,       0,   FT_STRUCT, ELEMENT_TYPE_END,   L"decimal",   &decimalZero,      0,      0,      0,                          PA_COUNT, "6")
PREDEFTYPEDEF(PT_CHAR,   "System.Char",         1,            1,      0,     0,   1,   0,       0,   FT_U2,   ELEMENT_TYPE_CHAR,    L"char",      0,                 0,      2,      SERIALIZATION_TYPE_CHAR,    PA_COUNT, "7")
PREDEFTYPEDEF(PT_BOOL,   "System.Boolean",      1,            1,      0,     0,   1,   0,       0,   FT_I1,   ELEMENT_TYPE_BOOLEAN, L"bool",      0,                 0,      1,      SERIALIZATION_TYPE_BOOLEAN, PA_COUNT, "8")

// "simple" types are certain types that the compiler knows about for conversion and operator purposes.ses.
// Keep these first so that we can build conversion tables on their ordinals... Don't change the orderder
// of the simple types because it will mess up conversion tables.
// The following Quasi-Simple types are considered simple, except they are non-CLS compliant
PREDEFTYPEDEF(PT_SBYTE,  "System.SByte",         1,           1,      1,     0,   1,   0,       0,   FT_I1,   ELEMENT_TYPE_I1,      L"sbyte",     0,                 1,      1,      SERIALIZATION_TYPE_I1,      PA_COUNT, "9")
PREDEFTYPEDEF(PT_USHORT, "System.UInt16",        1,           1,      1,     0,   1,   0,       0,   FT_U2,   ELEMENT_TYPE_U2,      L"ushort",    0,                 1,      2,      SERIALIZATION_TYPE_U2,      PA_COUNT, "a")
PREDEFTYPEDEF(PT_UINT,   "System.UInt32",        1,           1,      1,     0,   1,   0,       0,   FT_U4,   ELEMENT_TYPE_U4,      L"uint",      0,                 1,      4,      SERIALIZATION_TYPE_U4,      PA_COUNT, "b")
PREDEFTYPEDEF(PT_ULONG,  "System.UInt64",        1,           1,      1,     0,   1,   0,       0,   FT_U8,   ELEMENT_TYPE_U8,      L"ulong",     &longZero,         1,      8,      SERIALIZATION_TYPE_U8,      PA_COUNT, "c")


//         id            full type name        required simple numer class struct iface  delegat fund type   elementtype,       nice name,    zero, quasi simple num,  attribute arg size serialization type)
PREDEFTYPEDEF(PT_OBJECT, "System.Object",             1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_OBJECT,   L"object",     0,                 0,      -1,     SERIALIZATION_TYPE_TAGGED_OBJECT,  PA_COUNT, "d")
PREDEFTYPEDEF(PT_STRING, "System.String",             1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_STRING,   L"string",     0,                 0,      -1,     SERIALIZATION_TYPE_STRING,  PA_COUNT, "e")
PREDEFTYPEDEF(PT_DELEGATE,"System.Delegate",          1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "f")
PREDEFTYPEDEF(PT_MULTIDEL,"System.MulticastDelegate", 1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "g")
PREDEFTYPEDEF(PT_ARRAY,   "System.Array",             1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "h")
PREDEFTYPEDEF(PT_EXCEPTION,"System.Exception",        1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "i")
PREDEFTYPEDEF(PT_TYPE,   "System.Type",               1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      -1,     SERIALIZATION_TYPE_TYPE,    PA_COUNT, "j")
PREDEFTYPEDEF(PT_CRITICAL,"System.Threading.Monitor", 0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "k")
PREDEFTYPEDEF(PT_VALUE,   "System.ValueType",         1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "l")
PREDEFTYPEDEF(PT_ENUM,    "System.Enum",              1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "m")


// The special "pointer-sized int" types. Note that this are not considered numeric types from the compiler's point of view --
// they are special only in that they have special signature encodings.
//         id            full type name        required simple numer class struct iface  delegat fund type   elementtype,       nice name,    zero, quasi simple num,  attribute arg size serialization type)
PREDEFTYPEDEF(PT_INTPTR,  "System.IntPtr",            0,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_I,       NULL,          0,                 0,      0,      0,                          PA_COUNT, "n")
PREDEFTYPEDEF(PT_UINTPTR, "System.UIntPtr",           0,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_U,       NULL,          0,                 0,      0,      0,                          PA_COUNT, "o")

// predefined attribute types
PREDEFTYPEDEF(PT_SECURITYATTRIBUTE, "System.Security.Permissions.CodeAccessSecurityAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "p")
PREDEFTYPEDEF(PT_SECURITYPERMATTRIBUTE, "System.Security.Permissions.SecurityPermissionAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "q")
PREDEFTYPEDEF(PT_UNVERIFCODEATTRIBUTE, "System.Security.UnverifiableCodeAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "r")
PREDEFTYPEDEF(PT_DEBUGGABLEATTRIBUTE, "System.Diagnostics.DebuggableAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "s")
PREDEFTYPEDEF(PT_MARSHALBYREF, "System.MarshalByRefObject",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "t")
PREDEFTYPEDEF(PT_CONTEXTBOUND, "System.ContextBoundObject",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "u")
PREDEFTYPEDEF(PT_IN,            "System.Runtime.InteropServices.InAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_IN, "v")
PREDEFTYPEDEF(PT_OUT,           "System.Runtime.InteropServices.OutAttribute",
                                                      1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_OUT, "w")
PREDEFTYPEDEF(PT_ATTRIBUTE, "System.Attribute",       1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "x")
PREDEFTYPEDEF(PT_ATTRIBUTEUSAGE, "System.AttributeUsageAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_ATTRIBUTEUSAGE, "y")
PREDEFTYPEDEF(PT_ATTRIBUTETARGETS, "System.AttributeTargets",
                                                      0,     0,     0,   0,   0,   0,       0,   FT_STRUCT, ELEMENT_TYPE_END,    NULL,          0,                 0,      0,      0,                          PA_COUNT, "z")
PREDEFTYPEDEF(PT_OBSOLETE, "System.ObsoleteAttribute",0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_OBSOLETE, "A")
PREDEFTYPEDEF(PT_CONDITIONAL, "System.Diagnostics.ConditionalAttribute",
						      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_CONDITIONAL, "B")
PREDEFTYPEDEF(PT_CLSCOMPLIANT, "System.CLSCompliantAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_CLSCOMPLIANT, "C")
PREDEFTYPEDEF(PT_GUID, "System.Runtime.InteropServices.GuidAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_GUID, "D")
PREDEFTYPEDEF(PT_DEFAULTMEMBER, "System.Reflection.DefaultMemberAttribute",
                                                      1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "E")
PREDEFTYPEDEF(PT_PARAMS, "System.ParamArrayAttribute",1,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_PARAMARRAY, "F")
PREDEFTYPEDEF(PT_COMIMPORT, "System.Runtime.InteropServices.ComImportAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COMIMPORT, "G")
PREDEFTYPEDEF(PT_FIELDOFFSET, "System.Runtime.InteropServices.FieldOffsetAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_STRUCTOFFSET, "H")
PREDEFTYPEDEF(PT_STRUCTLAYOUT, "System.Runtime.InteropServices.StructLayoutAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_STRUCTLAYOUT, "I")
PREDEFTYPEDEF(PT_LAYOUTKIND, "System.Runtime.InteropServices.LayoutKind",
                                                      0,     0,     0,   0,   0,   0,       0,   FT_STRUCT, ELEMENT_TYPE_END,    NULL,          0,                 0,      0,      0,                          PA_COUNT, "J")
PREDEFTYPEDEF(PT_MARSHALAS, "System.Runtime.InteropServices.MarshalAsAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "K")
PREDEFTYPEDEF(PT_DLLIMPORT, "System.Runtime.InteropServices.DllImportAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_DLLIMPORT, "L")
PREDEFTYPEDEF(PT_INDEXERNAME, "System.Runtime.CompilerServices.IndexerNameAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_NAME, "M")
PREDEFTYPEDEF(PT_DECIMALCONSTANT, "System.Runtime.CompilerServices.DecimalConstantAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "N")
PREDEFTYPEDEF(PT_REQUIRED, "System.Runtime.CompilerServices.RequiredAttributeAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_REQUIRED, "O")

// predefined types for the BCL
PREDEFTYPEDEF(PT_REFANY,  "System.TypedReference",    0,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_TYPEDBYREF, NULL,       0,                 0,      0,      0,                          PA_COUNT, "P")
PREDEFTYPEDEF(PT_ARGITERATOR,   "System.ArgIterator", 0,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "Q")
PREDEFTYPEDEF(PT_TYPEHANDLE, "System.RuntimeTypeHandle",
                                                      1,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "R")
PREDEFTYPEDEF(PT_ARGUMENTHANDLE, "System.RuntimeArgumentHandle",
                                                      0,     0,     0,   0,   1,   0,       0,   FT_STRUCT,ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "S")
PREDEFTYPEDEF(PT_HASHTABLE, "System.Collections.Hashtable",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "T")
PREDEFTYPEDEF(PT_IASYNCRESULT, "System.IAsyncResult", 0,     0,     0,   0,   0,   1,       0,   FT_REF,  ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "U")
PREDEFTYPEDEF(PT_ASYNCCBDEL, "System.AsyncCallback",  0,     0,     0,   0,   0,   0,       1,   FT_REF,  ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "V")
PREDEFTYPEDEF(PT_SECURITYACTION,"System.Security.Permissions.SecurityAction",
                                                      0,     0,     0,   0,   0,   0,       0,   FT_I4,   ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "W")
PREDEFTYPEDEF(PT_IDISPOSABLE, "System.IDisposable",   0,     0,     0,   0,   0,   1,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "X")
PREDEFTYPEDEF(PT_IENUMERABLE, "System.Collections.IEnumerable",
                                                      0,     0,     0,   0,   0,   1,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "Y")
PREDEFTYPEDEF(PT_IENUMERATOR, "System.Collections.IEnumerator",
                                                      0,     0,     0,   0,   0,   1,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "Z")
PREDEFTYPEDEF(PT_SYSTEMVOID, "System.Void",
                                                      0,     0,     0,   0,   1,   0,       0,   FT_STRUCT, ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "VOID")

PREDEFTYPEDEF(PT_RUNTIMEHELPERS, "System.Runtime.CompilerServices.RuntimeHelpers",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,     NULL,          0,                 0,      0,      0,                          PA_COUNT, "RT")

// signature MODIFIER for marking volatile fields
PREDEFTYPEDEF(PT_VOLATILEMOD, "System.Runtime.CompilerServices.IsVolatile",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COUNT, "AA")
// Sets the CoClass for a COM interface wrapper
PREDEFTYPEDEF(PT_COCLASS,     "System.Runtime.InteropServices.CoClassAttribute",
                                                      0,     0,     0,   1,   0,   0,       0,   FT_REF,  ELEMENT_TYPE_END,      NULL,          0,                 0,      0,      0,                          PA_COCLASS, "CC")


