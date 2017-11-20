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
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
namespace System.Runtime.InteropServices {

	using System;


	/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum"]/*' />
	[Serializable]
	public enum VarEnum
	{	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_EMPTY"]/*' />
		VT_EMPTY			= 0,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_NULL"]/*' />
		VT_NULL				= 1,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I2"]/*' />
		VT_I2				= 2,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I4"]/*' />
		VT_I4				= 3,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_R4"]/*' />
		VT_R4				= 4,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_R8"]/*' />
		VT_R8				= 5,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CY"]/*' />
		VT_CY				= 6,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_DATE"]/*' />
		VT_DATE				= 7,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BSTR"]/*' />
		VT_BSTR				= 8,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_ERROR"]/*' />
		VT_ERROR			= 10,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BOOL"]/*' />
		VT_BOOL				= 11,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VARIANT"]/*' />
		VT_VARIANT			= 12,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UNKNOWN"]/*' />
		VT_UNKNOWN			= 13,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_DECIMAL"]/*' />
		VT_DECIMAL			= 14,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I1"]/*' />
		VT_I1				= 16,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI1"]/*' />
		VT_UI1				= 17,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI2"]/*' />
		VT_UI2				= 18,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI4"]/*' />
		VT_UI4				= 19,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_I8"]/*' />
		VT_I8				= 20,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UI8"]/*' />
		VT_UI8				= 21,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_INT"]/*' />
		VT_INT				= 22,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_UINT"]/*' />
		VT_UINT				= 23,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VOID"]/*' />
		VT_VOID				= 24,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_HRESULT"]/*' />
		VT_HRESULT			= 25,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_PTR"]/*' />
		VT_PTR				= 26,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_SAFEARRAY"]/*' />
		VT_SAFEARRAY		= 27,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CARRAY"]/*' />
		VT_CARRAY			= 28,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_USERDEFINED"]/*' />
		VT_USERDEFINED		= 29,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_LPSTR"]/*' />
		VT_LPSTR			= 30,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_LPWSTR"]/*' />
		VT_LPWSTR			= 31,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_RECORD"]/*' />
		VT_RECORD			= 36,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_FILETIME"]/*' />
		VT_FILETIME			= 64,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BLOB"]/*' />
		VT_BLOB				= 65,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STREAM"]/*' />
		VT_STREAM			= 66,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STORAGE"]/*' />
		VT_STORAGE			= 67,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STREAMED_OBJECT"]/*' />
		VT_STREAMED_OBJECT	= 68,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_STORED_OBJECT"]/*' />
		VT_STORED_OBJECT	= 69,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BLOB_OBJECT"]/*' />
		VT_BLOB_OBJECT		= 70,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CF"]/*' />
		VT_CF				= 71,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_CLSID"]/*' />
		VT_CLSID			= 72,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_VECTOR"]/*' />
		VT_VECTOR			= 0x1000,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_ARRAY"]/*' />
		VT_ARRAY			= 0x2000,
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="VarEnum.VT_BYREF"]/*' />
		VT_BYREF			= 0x4000
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType"]/*' />
	[Serializable()]
	public enum UnmanagedType
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Bool"]/*' />
		Bool			 = 0x2,			// 4 byte boolean value (true != 0, false == 0)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I1"]/*' />
		I1				 = 0x3,			// 1 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U1"]/*' />
		U1				 = 0x4,			// 1 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I2"]/*' />
		I2				 = 0x5,			// 2 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U2"]/*' />
		U2				 = 0x6,			// 2 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I4"]/*' />
		I4				 = 0x7,			// 4 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U4"]/*' />
		U4				 = 0x8,			// 4 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.I8"]/*' />
		I8				 = 0x9,			// 8 byte signed value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.U8"]/*' />
		U8				 = 0xa,			// 8 byte unsigned value

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.R4"]/*' />
		R4				 = 0xb,			// 4 byte floating point

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.R8"]/*' />
		R8				 = 0xc,			// 8 byte floating point

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Currency"]/*' />
		Currency		 = 0xf,			// A currency


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPStr"]/*' />
		LPStr			 = 0x14,		// Ptr to SBCS string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPWStr"]/*' />
		LPWStr			 = 0x15,		// Ptr to Unicode string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPTStr"]/*' />
		LPTStr			 = 0x16,		// Ptr to OS preferred (SBCS/Unicode) string

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.ByValTStr"]/*' />
		ByValTStr		 = 0x17,		// OS preferred (SBCS/Unicode) inline string (only valid in structs)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.IUnknown"]/*' />
		IUnknown		 = 0x19,		// COM IUnknown pointer. 


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Struct"]/*' />
		Struct			 = 0x1b,		// Structure


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.ByValArray"]/*' />
		ByValArray		 = 0x1e,		// Array of fixed size (only valid in structs)

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.SysInt"]/*' />
		SysInt			 = 0x1f,		// Hardware natural sized signed integer

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.SysUInt"]/*' />
		SysUInt			 = 0x20,		 


		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.FunctionPtr"]/*' />
		FunctionPtr		 = 0x26,		// Function pointer

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.AsAny"]/*' />
		AsAny			 = 0x28,		// Paired with Object type and does runtime marshalling determination

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPArray"]/*' />
		LPArray			 = 0x2a,		// C style array

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.LPStruct"]/*' />
		LPStruct		 = 0x2b,		// Pointer to a structure

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.CustomMarshaler"]/*' />
		CustomMarshaler	 = 0x2c,		

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="UnmanagedType.Error"]/*' />
		Error			 = 0x2d,		
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter | AttributeTargets.Field | AttributeTargets.ReturnValue, Inherited = false)] 
	public sealed class MarshalAsAttribute : Attribute	 
	{ 
		internal UnmanagedType _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalAsAttribute"]/*' />
		public MarshalAsAttribute(UnmanagedType unmanagedType)
		{
			_val = unmanagedType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalAsAttribute1"]/*' />
		public MarshalAsAttribute(short unmanagedType)
		{
			_val = (UnmanagedType)unmanagedType;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.Value"]/*' />
		public UnmanagedType Value { get {return _val;} }	


		// Fields used with SubType = ByValArray and LPArray.
		// Array size =	 parameter(PI) * PM + C
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.ArraySubType"]/*' />
		public UnmanagedType	  ArraySubType;	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SizeParamIndex"]/*' />
		public short			  SizeParamIndex;			// param index PI
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.SizeConst"]/*' />
		public int				  SizeConst;				// constant C

		// Fields used with SubType = CustomMarshaler
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalType"]/*' />
		public String			  MarshalType;				// Name of marshaler class
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalTypeRef"]/*' />
		public Type 			  MarshalTypeRef;			// Type of marshaler class
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="MarshalAsAttribute.MarshalCookie"]/*' />
		public String			  MarshalCookie;			// cookie to pass to marshaler
	}


	/// <include file='doc\Attributes.uex' path='docs/doc[@for="PreserveSigAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class PreserveSigAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="PreserveSigAttribute.PreserveSigAttribute"]/*' />
		public PreserveSigAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="InAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class InAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="InAttribute.InAttribute"]/*' />
		public InAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="OutAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class OutAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="OutAttribute.OutAttribute"]/*' />
		public OutAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="OptionalAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Parameter, Inherited = false)] 
	public sealed class OptionalAttribute : Attribute
	{
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="OptionalAttribute.OptionalAttribute"]/*' />
		public OptionalAttribute()
		{
		}
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Method, Inherited = false)] 
	public sealed class DllImportAttribute : Attribute
	{
		internal String _val;

        /// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.DllImportAttribute"]/*' />
		public DllImportAttribute(String dllName)
		{
			_val = dllName;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.Value"]/*' />
		public String Value { get {return _val;} }	

		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.EntryPoint"]/*' />
		public String			  EntryPoint;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.CharSet"]/*' />
		public CharSet			  CharSet;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.SetLastError"]/*' />
		public bool				  SetLastError;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.ExactSpelling"]/*' />
		public bool				  ExactSpelling;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.PreserveSig"]/*' />
		public bool				  PreserveSig;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="DllImportAttribute.CallingConvention"]/*' />
		public CallingConvention  CallingConvention;
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)] 
	public sealed class StructLayoutAttribute : Attribute
	{
		internal LayoutKind _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.StructLayoutAttribute"]/*' />
		public StructLayoutAttribute(LayoutKind layoutKind)
		{
			_val = layoutKind;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.StructLayoutAttribute1"]/*' />
		public StructLayoutAttribute(short layoutKind)
		{
			_val = (LayoutKind)layoutKind;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Value"]/*' />
		public LayoutKind Value { get {return _val;} }	
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Pack"]/*' />
		public int				  Pack;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.Size"]/*' />
		public int 			  Size;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="StructLayoutAttribute.CharSet"]/*' />
		public CharSet			  CharSet;
	}

	/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute"]/*' />
	[AttributeUsage(AttributeTargets.Field, Inherited = false)] 
	public sealed class FieldOffsetAttribute : Attribute
	{
		internal int _val;
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute.FieldOffsetAttribute"]/*' />
		public FieldOffsetAttribute(int offset)
		{
			_val = offset;
		}
		/// <include file='doc\Attributes.uex' path='docs/doc[@for="FieldOffsetAttribute.Value"]/*' />
		public int Value { get {return _val;} }	
	}

}
