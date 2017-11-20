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
// This class represents the Ole Automation binder.

// #define DISPLAY_DEBUG_INFO

namespace System {

	using System;
	using System.Runtime.InteropServices;
    using System.Reflection;
	using CultureInfo = System.Globalization.CultureInfo;    

    // Made serializable in anticipation of this class eventually having state.
	[Serializable()] 
    internal class OleAutBinder : DefaultBinder
    {
    	// ChangeType
    	// This binder uses OLEAUT to change the type of the variant.
      	public override Object ChangeType(Object value, Type type, CultureInfo cultureInfo)
      	{
			Variant myValue = new Variant(value);
    		if (cultureInfo == null)
    			cultureInfo = CultureInfo.CurrentCulture;
    			
    #if DISPLAY_DEBUG_INFO		
    		Console.Write("In OleAutBinder::ChangeType converting variant of type: ");
    		Console.Write(myValue.VariantType);
    		Console.Write(" to type: ");
    		Console.WriteLine(type.Name);
    #endif		
    		
    		// If we are trying to convert to variant then there is nothing to do.
    		if (type == typeof(Variant))
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Variant being changed to type variant is always legal");
    #endif		
    			return value;
    		}
	
			if (type.IsByRef)
			{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Striping byref from the type to convert to.");
    #endif		
				type = type.GetElementType();
			}

			// If we are trying to convert from an object to another type then we don't
			// need the OLEAUT change type, we can just use the normal COM+ mechanisms.
    		if (!type.IsPrimitive && type.IsInstanceOfType(value))
    		{
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Source variant can be assigned to destination type");
    #endif		
    			return value;
    		}

            // Handle converting primitives to enums.
		    if (type.IsEnum && value.GetType().IsPrimitive)
		    {
    #if DISPLAY_DEBUG_INFO		
    			Console.WriteLine("Converting primitive to enum");
    #endif		
                return Enum.Parse(type, value.ToString());
		    }


            return base.ChangeType(value, type, cultureInfo);

      	}
      	
      	
        // CanChangeType
        public override bool CanChangeType(Object value, Type type, CultureInfo cultureInfo)
        {
            Variant myValue = new Variant(value);
            if (cultureInfo == null)
                cultureInfo = CultureInfo.CurrentCulture;
                
    #if DISPLAY_DEBUG_INFO      
            Console.Write("In OleAutBinder::CanChangeType converting variant of type: ");
            Console.Write(myValue.VariantType);
            Console.Write(" to type: ");
            Console.WriteLine(type.Name);
    #endif      
            
            // If we are trying to convert to variant then there is nothing to do.
            if (type == typeof(Variant))
            {
    #if DISPLAY_DEBUG_INFO      
                Console.WriteLine("Variant being changed to type variant is always legal");
    #endif      
                return true;
            }
    
            if (type.IsByRef)
            {
    #if DISPLAY_DEBUG_INFO      
                Console.WriteLine("Striping byref from the type to convert to.");
    #endif      
                type = type.GetElementType();
            }

            // If we are trying to convert from an object to another type then we don't
            // need the OLEAUT change type, we can just use the normal COM+ mechanisms.
            if (!type.IsPrimitive && type.IsInstanceOfType(value))
            {
    #if DISPLAY_DEBUG_INFO      
                Console.WriteLine("Source variant can be assigned to destination type");
    #endif      
                return true;
            }

            // Handle converting primitives to enums.
            if (type.IsEnum && value.GetType().IsPrimitive)
            {
    #if DISPLAY_DEBUG_INFO      
                Console.WriteLine("Converting primitive to enum");
    #endif      
                return true;
            }

            return base.CanChangeType(value, type, cultureInfo);
        }
    }
}
