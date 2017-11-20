//------------------------------------------------------------------------------
// <copyright file="StateMachine.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    internal class StateMachine {
        // Constants for the state table
        private  const int Init         = 0x000000;       // Initial state
        private  const int Elem         = 0x000001;       // Element was output
        private  const int AttrN        = 0x000002;       // Attribute name was output
        private  const int AttrV        = 0x000003;       // Attribute value was output (some more can follow)
        private  const int Attr         = 0x000004;       // Attribute was output
        private  const int InElm        = 0x000005;       // Filling in element, general state text
        private  const int EndEm        = 0x000006;       // After end element event - next end element doesn't generate token
        private  const int InCmt        = 0x000007;       // Adding text to a comment
        private  const int InPI         = 0x000008;       // Adding text to a processing instruction
        private  const int InCDT        = 0x000009;       // Adding text to a CDATA section
        private  const int InDcl        = 0x00000A;       // Adding attributes to an XMLDECL
        private  const int InDTp        = 0x00000B;       // Adding text to a DocType
        private  const int InERf        = 0x00000C;       // Adding text to an entity ref

        private  const int StateMask    = 0x00000F;       // State mask

        internal const int Error        = 0x000010;       // Invalid XML state

        private  const int Ignor        = 0x000020;       // Ignore this transition
        private  const int Assrt        = 0x000030;       // Assrt

        private  const int U            = 0x000100;       // Depth up
        private  const int D            = 0x000200;       // Depth down

        internal const int DepthMask    = 0x000300;       // Depth mask

        internal const int DepthUp      = U;
        internal const int DepthDown    = D;

        private  const int C            = 0x000400;       // BeginChild
        private  const int H            = 0x000800;       // HadChild
        private  const int M            = 0x001000;       // EmptyTag

        internal const int BeginChild   = C;
        internal const int HadChild     = H;
        internal const int EmptyTag     = M;

        private  const int B            = 0x002000;       // Begin Record
        private  const int E            = 0x004000;       // Record finished

        internal const int BeginRecord  = B;
        internal const int EndRecord    = E;

        private  const int S            = 0x008000;       // Push namespace scope
        private  const int P            = 0x010000;       // Pop current namepsace scope

        internal const int PushScope    = S;
        internal const int PopScope     = P;              // Next event must pop scope

        //
        // Runtime state
        //

        private int _State;

        internal StateMachine() {
            _State = Init;
        }

        internal int State {
            get {
                return _State;
            }

            set {
                // Hope you know what you are doing ...
                _State = value;
            }
        }

        internal void Reset() {
            _State = Init;
        }

        internal static int StateOnly(int state) {
            return state & StateMask;
        }

        private static readonly int[] token = {
            /* Root                    */ (int)XmlNodeType.Document,
            /* Element                 */ (int)XmlNodeType.Element,
            /* Attribute               */ (int)XmlNodeType.Attribute,
            /* Namespace               */ (int)XmlNodeType.Attribute,
            /* Text                    */ (int)XmlNodeType.Text,
            /* SignificantWhitespace   */ (int)XmlNodeType.SignificantWhitespace,
            /* Whitespace              */ (int)XmlNodeType.Whitespace,
            /* ProcessingInstruction   */ (int)XmlNodeType.ProcessingInstruction,
            /* Comment                 */ (int)XmlNodeType.Comment,
        };

        internal int BeginOutlook(XPathNodeType nodeType) {
            int newState = s_BeginTransitions[token[(int)nodeType], _State];
            Debug.Assert(newState != Assrt);
            return newState;
        }

        internal int Begin(XPathNodeType nodeType) {
            int newState = s_BeginTransitions[token[(int)nodeType], _State];
            Debug.Assert(newState != Assrt);

            if (newState != Error && newState != Ignor) {
                _State = newState & StateMask;
            }
            return newState;
        }

        internal int EndOutlook(XPathNodeType nodeType) {
            int newState = s_EndTransitions[token[(int)nodeType], _State];
            Debug.Assert(newState != Assrt);
            return newState;
        }

        internal int End(XPathNodeType nodeType) {
            int newState = s_EndTransitions[token[(int)nodeType], _State];
            Debug.Assert(newState != Assrt);

            if (newState != Error && newState != Ignor) {
                _State = newState & StateMask;
            }
            return newState;
        }

        private static readonly int [,] s_BeginTransitions =
        {
            /*                             { Init,            Elem,             AttrN,            AttrV,            Attr,             InElm,            EndEm,            InCmt,            InPI,             InCDT,            InDcl,            InDTp,            InERf             }, */
            /* ========================================================================================================================================================================================================================================================================  */
            /* None                     */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* Element                  */ { Elem |B|S,       Elem |U|C|B|S,    Error,            Error,            Elem |C|B|S,      Elem |B|S,        Elem |B|P|S,      Error,            Error,            Error,            Error,            Error,            Error},
            /* Attribute                */ { Error,           AttrN|U,          Error,            Error,            AttrN,            Error,            Error,            Error,            Error,            Error,            AttrN|U,          AttrN|U,            Error},
            /* Text                     */ { InElm|B,         InElm|U|C|B,      AttrV|U,          AttrV,            InElm|C|B,        InElm,            InElm|B|P,        InCmt,            InPI,             InCDT,            Error,            InDTp,            Error},
            /* CDATA                    */ { InCDT|B,         InCDT|U|C|B,      Error,            Error,            InCDT|C|B,        InCDT|B,          InCDT|B|P,        Error,            Error,            Error,            Error,            Error,            Error},
            /* EntityReference          */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* Entity                   */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* ProcessingInstruction    */ { InPI |B,         InPI |U|C|B,      Error,            Error,            InPI |C|B,        InPI |B,          InPI |B|P,        Error,            Error,            Error,            Error,            Error,            Error},
            /* Comment                  */ { InCmt|B,         InCmt|U|C|B,      Error,            Error,            InCmt|C|B,        InCmt|B,          InCmt|B|P,        Error,            Error,            Error,            Error,            Error,            Error},
            /* Document                 */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* DocumentType             */ { InDTp,           InDTp|U|C|B,      Error,            Error,            InDTp|C|B,        InDTp|B,          InDTp|B|P,        Error,            Error,            Error,            Error,            Error,            Error},
            /* DocumentFragment         */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* Notation                 */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* Whitespace               */ { InElm|B,         InElm|U|C|B,      AttrV|U,          AttrV,            InElm|C|B,        InElm,            InElm|B|P,        InCmt,            InPI,             InCDT,            Error,            InDTp,            Error},
            /* SignificantWhitespace    */ { InElm|B,         InElm|U|C|B,      AttrV|U,          AttrV,            InElm|C|B,        InElm,            InElm|B|P,        InCmt,            InPI,             InCDT,            Error,            InDTp,            Error},
            /* EndElement               */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* EndEntity                */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* CharacterEntity          */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
            /* XmlDeclaration           */ { InDcl |B,        InDcl |U|C|B,     Error,            Error,            InDcl |C|B,       InDcl |B,         InDcl |B|P,       Error,            Error,            Error,            Error,            Error,            Error},
            /* All                      */ { Error,           Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error,            Error},
        };

        private static readonly int [,] s_EndTransitions =
        {
            /*                             { Init,            Elem,             AttrN,            AttrV,            Attr,             InElm,            EndEm,            InCmt,            InPI,             InCDT,            InDcl,            InDTp,            InERf             }, */
            /* ========================================================================================================================================================================================================================================================================  */
            /* None                     */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Element                  */ { Assrt,           EndEm|B|E|P|M,    Assrt,            Assrt,            EndEm|D|B|E|P|M,  EndEm|D|H|B|E|P,  EndEm|D|H|B|E|P,  Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Attribute                */ { Assrt,           Assrt,            Attr,             Attr |D,          Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Text                     */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* CDATA                    */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            EndEm|E,          Assrt,            Assrt,            Assrt},
            /* EntityReference          */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Entity                   */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* ProcessingInstruction    */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            EndEm|E,          Assrt,            Assrt,            Assrt,            Assrt},
            /* Comment                  */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            EndEm|E,          Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Document                 */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* DocumentType             */ { Assrt,           Assrt,            Assrt,            Assrt,            EndEm|E,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            EndEm|E,          Assrt},
            /* DocumentFragment         */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Notation                 */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* Whitespace               */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* SignificantWhitespace    */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* EndElement               */ { Assrt,           EndEm|B|E|P|M,    Assrt,            Assrt,            EndEm|D|B|E|P|M,  EndEm|D|H|B|E|P,  EndEm|D|H|B|E|P,  Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* EndEntity                */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* CharacterEntity          */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
            /* XmlDeclaration           */ { Assrt,           Assrt,            Assrt,            Assrt,            EndEm|E|D,        Assrt,            Assrt,            Assrt,            EndEm|E,          Assrt,            Assrt,            Assrt,            Assrt},
            /* All                      */ { Assrt,           Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt,            Assrt},
        };
    }

}
