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
** Class: ValueTypeFixupInfo
**
**                                
**
** Purpose: Information about an object required to do a value-type
** fixup.  This includes the id of the containing object and the
** member info (if the containing body is an object or value type) 
** or the array indices (if the containing body is an array.)
**
** Date: August 30, 2000
**
============================================================*/
namespace System.Runtime.Serialization {
    using System.Reflection;

    internal class ValueTypeFixupInfo {

        //The id of the containing body.  This could be a regular
        //object, another value type, or an array.  For obvious reasons,
        //the containing body can never have both a FieldInfo and 
        //an array index.
        private long m_containerID;

        //The FieldInfo into the containing body.  This will only 
        //apply if the containing body is a field info or another
        //value type.
        private FieldInfo m_parentField;
        
        //The array index of the index into the parent.  This will only
        //apply if the containing body is an array.
        private int[] m_parentIndex;
        
        public ValueTypeFixupInfo(long containerID, FieldInfo member, int[] parentIndex) {
            
            BCLDebug.Trace("SER", "[ValueTypeFixupInfo.ctor]Creating a VTFI with Container ID: ", containerID, " and MemberInfo ", member);

            if (containerID==0 && member==null) {
                m_containerID = containerID;
                m_parentField = member;
                m_parentIndex = parentIndex;
            }

            //If both member and arrayIndex are null, we don't have enough information to create
            //a tunnel to do the fixup.
            if (member==null && parentIndex==null) {
                throw new ArgumentException(Environment.GetResourceString("Argument_MustSupplyParent"));
            }

            //If the member isn't null, we know that they supplied a MemberInfo as the parent.  This means
            //that the arrayIndex must be null because we can't have a FieldInfo into an array. 
            if (member!=null) {
                if (parentIndex!=null) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_MemberAndArray"));
                }
                       
                if (((((FieldInfo)member).FieldType).IsValueType) && containerID==0) {
                    throw new ArgumentException(Environment.GetResourceString("Argument_MustSupplyContainer"));
                }
            } 

            m_containerID=containerID;
            m_parentField = (FieldInfo)member;
            m_parentIndex = parentIndex;
        }

        public long ContainerID { 
            get {
                return m_containerID;
            }
        }

        public FieldInfo ParentField {
            get {
                return m_parentField;
            }
        }

        public int[] ParentIndex {
            get {
                return m_parentIndex;
            }
        }
    }
}
