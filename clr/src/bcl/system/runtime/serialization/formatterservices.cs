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
/*============================================================
**
** Class: FormatterServices
**
**                                
**
** Purpose: Provides some static methods to aid with the implementation
**          of a Formatter for Serialization.
**
** Date: September 27, 1999
**
============================================================*/
namespace System.Runtime.Serialization {
    
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Security.Permissions;
    using System.Runtime.Remoting;
	using System.Runtime.CompilerServices;
    using StackCrawlMark = System.Threading.StackCrawlMark;
    using System.IO;
    using System.Text;

    /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices"]/*' />
    public sealed class FormatterServices {
    
        internal static Hashtable m_MemberInfoTable = new Hashtable(32);
        private  static bool      m_checkedRegistry       = false;
        private  static bool      m_ignoreSerializationBit = false;
        private  static bool      m_logNonSerializable    = false;
    
        private FormatterServices() {
            throw new NotSupportedException();
        }
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern MemberInfo[] nativeGetSerializableMembers(RuntimeType type, bool bFilterTransient);
        
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern void GetSerializationRegistryValues(out bool checkBit, out bool logNonSerializable);

        private static void WriteNonSerializableMember(Type nonSer) {
            try {
#if !PLATFORM_UNIX
                FileStream stream = new FileStream("C:\\Temp\\NonSerialized", FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
#else
                FileStream stream = new FileStream("/tmp/NonSerialized", FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
#endif // !PLATFORM_UNIX
                StreamWriter writer = new StreamWriter(stream);
                writer.WriteLine("Unable to serialize " + nonSer.FullName + " from Module " + nonSer.Module);
                writer.Close();
            } catch (Exception e) {
                //                Console.WriteLine("[WriteNonSerializableMember]Failed with exception ", e)
                BCLDebug.Trace("SER", "[WriteNonSerializableMember]Failed with exception ", e);
            }
        }

        private static bool CheckSerializable(RuntimeType type) {
            if (type.IsSerializable) {
                return true;
            }
            return false;
        }

        private static MemberInfo[] InternalGetSerializableMembers(RuntimeType type, bool excludeNonSerializable) {
            ArrayList allMembers=null;
            MemberInfo[] typeMembers;
            FieldInfo [] typeFields;
            RuntimeType parentType;

            BCLDebug.Assert(type!=null, "[GetAllSerializableMembers]type!=null");
            
            if (type.IsInterface) {
                return new MemberInfo[0];
            }

            if (!m_checkedRegistry) {
                GetSerializationRegistryValues(out m_ignoreSerializationBit, out m_logNonSerializable);
            }

            if (!(CheckSerializable(type))) {
                if (m_logNonSerializable) {
                    WriteNonSerializableMember(type);
                }
                if (!m_ignoreSerializationBit) {
                    throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_NonSerType"), type.FullName, type.Module.Assembly.FullName));
                }
            }
          
            //Get all of the serializable members in the class to be serialized.
            typeMembers = nativeGetSerializableMembers(type, excludeNonSerializable);

            //If this class doesn't extend directly from object, walk its hierarchy and 
            //get all of the private and assembly-access fields (e.g. all fields that aren't
            //virtual) and include them in the list of things to be serialized.  
            parentType = (RuntimeType)(type.BaseType);
            if (parentType!=null && parentType!=typeof(Object)) {
                allMembers = new ArrayList();
                while (parentType!=null && parentType!=typeof(Object)) {
                    if (parentType.IsInterface) {
                        continue;
                    }

                    if (!CheckSerializable(parentType)) {
                        if (m_logNonSerializable) {
                            WriteNonSerializableMember(parentType);
                        }
                        if (!m_ignoreSerializationBit) {
                            throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_NonSerType"), parentType.FullName, parentType.Module.Assembly.FullName));
                        }
                    }

                    typeFields = ((RuntimeType)parentType).InternalGetFields(BindingFlags.Instance | BindingFlags.NonPublic, false);
                    String typeName = parentType.Name;
                    foreach (FieldInfo field in typeFields) {
                        if (!excludeNonSerializable || !field.IsNotSerialized) {
                            allMembers.Add(new SerializationFieldInfo((RuntimeFieldInfo)field, typeName));
                        }
                    }
                    parentType = (RuntimeType)(parentType.BaseType);
                }
            }
            
            //If we actually found any new MemberInfo's, we need to create a new MemberInfo array and
            //copy all of the members which we've found so far into that.
            if (allMembers!=null && allMembers.Count>0) {
                MemberInfo[] membersTemp = new MemberInfo[allMembers.Count + typeMembers.Length];
                Array.Copy(typeMembers, membersTemp, typeMembers.Length);
                allMembers.CopyTo(membersTemp, typeMembers.Length);
                typeMembers = membersTemp;
            }

            return typeMembers;
        }

        // Get all of the Serializable members for a particular class.  For all practical intents and
        // purposes, this is the non-transient, non-static members (fields and properties).  In order to
        // be included, properties must have both a getter and a setter.  N.B.: A class
        // which implements ISerializable or has a serialization surrogate may not use all of these members
        // (or may have additional members).
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.GetSerializableMembers"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public static MemberInfo[] GetSerializableMembers(Type type) {
            return GetSerializableMembers(type, new StreamingContext(StreamingContextStates.All));
        }

        // Get all of the Serializable Members for a particular class.  If we're not cloning, this is all
        // non-transient, non-static fields.  If we are cloning, include the transient fields as well since
        // we know that we're going to live inside of the same context.
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.GetSerializableMembers1"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public static MemberInfo[] GetSerializableMembers(Type type, StreamingContext context) {
            MemberInfo[] members;
    
            if (type==null) {
                throw new ArgumentNullException("type");
            }

            if (!(type is RuntimeType)) {
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_InvalidType"), type.ToString()));
            }
    
            MemberHolder mh = new MemberHolder(type, context);
    
            //If we've already gathered the members for this type, just return them.
            if ((members=(MemberInfo[])m_MemberInfoTable[mh])!=null) {
                return members;
            }
            
            lock (typeof(FormatterServices)) {
                //If we've already gathered the members for this type, just return them.
                if ((members=(MemberInfo[])m_MemberInfoTable[mh])!=null) {
                    return members;
                }

                //If we're not cloning, get all of the non-transient members.  If we are cloning, then we know that
                //we're going to live inside of the same context, so copy the transient members as well.
                if (context.State==StreamingContextStates.Clone) {
                    members = InternalGetSerializableMembers((RuntimeType)type, false);
                } else {
                    members = InternalGetSerializableMembers((RuntimeType)type, true);
                }
            
                m_MemberInfoTable[mh] = members;
            }
    
            return members;
        }
    
    
        // Gets a new instance of the object.  The entire object is initalized to 0 and no 
        // constructors have been run. **THIS MEANS THAT THE OBJECT MAY NOT BE IN A STATE
        // CONSISTENT WITH ITS INTERNAL REQUIREMENTS** This method should only be used for
        // deserialization when the user intends to immediately populate all fields.  This method
        // will not create an unitialized string because it is non-sensical to create an empty
        // instance of an immutable type.
        //
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.GetUninitializedObject"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public static Object GetUninitializedObject(Type type) {
            if (type==null) {
                throw new ArgumentNullException("type");
            }
    
            if (!(type is RuntimeType)) {
                throw new SerializationException(String.Format(Environment.GetResourceString("Serialization_InvalidType"), type.ToString()));
            }

            return nativeGetUninitializedObject((RuntimeType)type);
        }
    
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern Object nativeGetUninitializedObject(RuntimeType type);

        internal static void SerializationSetValue(MemberInfo fi, Object target, Object value) {
            BCLDebug.Assert(fi is RuntimeFieldInfo || fi is SerializationFieldInfo, 
                            "[SerializationSetValue]fi is RuntimeFieldInfo || fi is SerializationFieldInfo.");

            RuntimeFieldInfo rfi = fi as RuntimeFieldInfo;
            if (rfi != null) {
                rfi.InternalSetValue(target, value, (BindingFlags)0, Type.DefaultBinder, null, false, true);
            } else {
                ((SerializationFieldInfo)fi).InternalSetValue(target, value, (BindingFlags)0, Type.DefaultBinder, null, false, true);
            }
        }

        internal static void SerializationSetValueDirect(FieldInfo fi, TypedReference target, Object value) {
            BCLDebug.Assert(fi is RuntimeFieldInfo, "[SerializationSetValue]fi is RuntimeFieldInfo.");
            ((RuntimeFieldInfo)fi).SetValueDirectImpl(target, value, false);
        }
    
        // Fill in the members of obj with the data contained in data.
        // Returns the number of members populated.
        //
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.PopulateObjectMembers"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public static Object PopulateObjectMembers(Object obj, MemberInfo[] members, Object[] data) {
            MemberInfo mi;
    
            BCLDebug.Trace("SER", "[PopulateObjectMembers]Enter.");
            

            if (obj==null) {
                throw new ArgumentNullException("obj");
            }

            if (members==null) {
                throw new ArgumentNullException("members");
            }

            if (data==null) {
                throw new ArgumentNullException("data");
            }

            if (members.Length!=data.Length) {
                throw new ArgumentException(Environment.GetResourceString("Argument_DataLengthDifferent"));
            }

            for (int i=0; i<members.Length; i++) {
                mi = members[i];
    
                if (mi==null) {
                    throw new ArgumentNullException("members", String.Format(Environment.GetResourceString("ArgumentNull_NullMember"), i));
                }
    
    
                //If we find an empty, it means that the value was never set during deserialization.
                //This is either a forward reference or a null.  In either case, this may break some of the
                //invariants mantained by the setter, so we'll do nothing with it for right now.
                if (data[i]!=null) {
                    if (mi.MemberType==MemberTypes.Field) {
                        SerializationSetValue(mi, obj, data[i]);
                    } else {
                        throw new SerializationException(Environment.GetResourceString("Serialization_UnknownMemberInfo"));
                    }

                    BCLDebug.Trace("SER", "[PopulateObjectMembers]\tType:", obj.GetType(), "\tMember:", 
                                   members[i].Name, " with member type: ", ((FieldInfo)members[i]).FieldType);
                }
                //Console.WriteLine("X");
            }
            
            BCLDebug.Trace("SER", "[PopulateObjectMembers]Leave.");

            return obj;
        }
    
        // Extracts the data from obj.  members is the array of members which we wish to
        // extract (must be FieldInfos or PropertyInfos).  For each supplied member, extract the matching value and
        // return it in a Object[] of the same size.
        //
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.GetObjectData"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        public static Object[] GetObjectData(Object obj, MemberInfo[] members) {
    
            if (obj==null) {
                throw new ArgumentNullException("obj");
            }
    
            if (members==null) {
                throw new ArgumentNullException("members");
            }
            
            int numberOfMembers = members.Length;
    
            Object[] data = new Object[numberOfMembers];
            MemberInfo mi;
    
            for (int i=0; i<numberOfMembers; i++) {
                mi=members[i];
    
                if (mi==null) {
                    throw new ArgumentNullException("members", String.Format(Environment.GetResourceString("ArgumentNull_NullMember"), i));
                }
    
                if (mi.MemberType==MemberTypes.Field) {
                    BCLDebug.Assert(mi is RuntimeFieldInfo || mi is SerializationFieldInfo,
                                    "[FormatterServices.GetObjectData]mi is RuntimeFieldInfo || mi is SerializationFieldInfo.");

                    RuntimeFieldInfo rfi = mi as RuntimeFieldInfo;
                    if (rfi != null) {
                        data[i] = rfi.InternalGetValue(obj, false);
                    } else {
                        data[i] = ((SerializationFieldInfo)mi).InternalGetValue(obj, false);
                    }
                } else {
                    throw new SerializationException(Environment.GetResourceString("Serialization_UnknownMemberInfo"));
                }
            }
    
            return data;
        }

        
        /*=============================GetTypeFromAssembly==============================
        **Action:
        **Returns:
        **Arguments:
        **Exceptions:
        ==============================================================================*/
        /// <include file='doc\FormatterServices.uex' path='docs/doc[@for="FormatterServices.GetTypeFromAssembly"]/*' />
        [SecurityPermissionAttribute(SecurityAction.LinkDemand, Flags=SecurityPermissionFlag.SerializationFormatter)]
        [ReflectionPermissionAttribute(SecurityAction.LinkDemand, TypeInformation=true)]
        public static Type GetTypeFromAssembly(Assembly assem, String name) {
            if (assem==null)
                throw new ArgumentNullException("assem");
            return assem.GetTypeInternal(name, false, false, false);
        }
    
        /*============================LoadAssemblyFromString============================
        **Action: Loads an assembly from a given string.  The current assembly loading story
        **        is quite confusing.  If the assembly is in the fusion cache, we can load it
        **        using the stringized-name which we transmitted over the wire.  If that fails,
        **        we try for a lookup of the assembly using the simple name which is the first
        **        part of the assembly name.  If we can't find it that way, we'll return null
        **        as our failure result.
        **Returns: The loaded assembly or null if it can't be found.
        **Arguments: assemblyName -- The stringized assembly name.
        **Exceptions: None
        ==============================================================================*/
        internal static Assembly LoadAssemblyFromString(String assemblyName) {
            //
            // Try using the stringized assembly name to load from the fusion cache.
            //
            BCLDebug.Trace("SER", "[LoadAssemblyFromString]Looking for assembly: ", assemblyName);
            Assembly found = Assembly.Load(assemblyName);
            return found;
        }
    }
}





