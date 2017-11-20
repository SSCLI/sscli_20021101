//------------------------------------------------------------------------------
// <copyright file="CodeIdentifier.cs" company="Microsoft">
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

namespace System.Xml.Serialization {
    
    using System;
    using System.Text;
    using System.Collections;
    using System.IO;
    using System.Globalization;
    using System.Diagnostics;
    using System.CodeDom.Compiler;

    /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier"]/*' />
    ///<internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class CodeIdentifier {
        static Hashtable cSharpKeywords = null;
    
        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakePascal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakePascal(string identifier) {
            identifier = MakeValid(identifier);
            if (identifier.Length <= 2)
                return identifier.ToUpper(CultureInfo.InvariantCulture);
            else if (char.IsLower(identifier[0]))
                return char.ToUpper(identifier[0], CultureInfo.InvariantCulture).ToString() + identifier.Substring(1);
            else
                return identifier;
        }
        
        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakeCamel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakeCamel(string identifier) {
            identifier = MakeValid(identifier);
            if (identifier.Length <= 2)
                return identifier.ToLower(CultureInfo.InvariantCulture);
            else if (char.IsUpper(identifier[0]))
                return char.ToLower(identifier[0], CultureInfo.InvariantCulture).ToString() + identifier.Substring(1);
            else
                return identifier;
        }
        
        /// <include file='doc\CodeIdentifier.uex' path='docs/doc[@for="CodeIdentifier.MakeValid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MakeValid(string identifier) {
            StringBuilder builder = new StringBuilder();
            for (int i = 0; i < identifier.Length; i++) {
                char c = identifier[i];
                if (IsValid(c)) builder.Append(c);
            }
            if (builder.Length == 0) return "Item";
            if (IsValidStart(builder[0]))
                return builder.ToString();
            else
                return "Item" + builder.ToString();
        }

        static bool IsValidStart(char c) {

            // the given char is already a valid name character
            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                if (!IsValid(c)) throw new ArgumentException(Res.GetString(Res.XmlInternalErrorDetails, "Invalid identifier character " + ((Int16)c).ToString()), "c");
            #endif

            // First char cannot be a number
            if (Char.GetUnicodeCategory(c) == UnicodeCategory.DecimalDigitNumber)
                return false;
            return true;
        }

        static bool IsValid(char c) {
            UnicodeCategory uc = Char.GetUnicodeCategory(c);
            // each char must be Lu, Ll, Lt, Lm, Lo, Nd, Mn, Mc, Pc
            // 
            switch (uc) {
                case UnicodeCategory.UppercaseLetter:        // Lu
                case UnicodeCategory.LowercaseLetter:        // Ll
                case UnicodeCategory.TitlecaseLetter:        // Lt
                case UnicodeCategory.ModifierLetter:         // Lm
                case UnicodeCategory.OtherLetter:            // Lo
                case UnicodeCategory.DecimalDigitNumber:     // Nd
                case UnicodeCategory.NonSpacingMark:         // Mn
                case UnicodeCategory.SpacingCombiningMark:   // Mc
                case UnicodeCategory.ConnectorPunctuation:   // Pc
                    break;
                case UnicodeCategory.LetterNumber:
                case UnicodeCategory.OtherNumber:
                case UnicodeCategory.EnclosingMark:
                case UnicodeCategory.SpaceSeparator:
                case UnicodeCategory.LineSeparator:
                case UnicodeCategory.ParagraphSeparator:
                case UnicodeCategory.Control:
                case UnicodeCategory.Format:
                case UnicodeCategory.Surrogate:
                case UnicodeCategory.PrivateUse:
                case UnicodeCategory.DashPunctuation:
                case UnicodeCategory.OpenPunctuation:
                case UnicodeCategory.ClosePunctuation:
                case UnicodeCategory.InitialQuotePunctuation:
                case UnicodeCategory.FinalQuotePunctuation:
                case UnicodeCategory.OtherPunctuation:
                case UnicodeCategory.MathSymbol:
                case UnicodeCategory.CurrencySymbol:
                case UnicodeCategory.ModifierSymbol:
                case UnicodeCategory.OtherSymbol:
                case UnicodeCategory.OtherNotAssigned:
                    return false;
                default:
                    #if DEBUG
                        // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                        throw new ArgumentException(Res.GetString(Res.XmlInternalErrorDetails, "Unhandled category " + uc), "c");
                    #else
                        return false;
                    #endif
            }
            return true;
        }

        internal static void CheckValidIdentifier(string ident) {
            if (!CodeGenerator.IsValidLanguageIndependentIdentifier(ident))
                throw new ArgumentException(Res.GetString(Res.XmlInvalidIdentifier, ident), "ident");
        }

        internal static string EscapeKeywords(string identifier) {
            if (identifier == null || identifier.Length == 0) return identifier;
            string originalIdentifier = identifier;
            int arrayCount = 0;
            while (identifier.EndsWith("[]")) {
                arrayCount++;
                identifier = identifier.Substring(0, identifier.Length - 2);
            }
            string[] names = identifier.Split(new char[] {'.'});
            bool newIdent = false;
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < names.Length; i++) {
                if (i > 0) sb.Append(".");
                if (IsCSharpKeyword(names[i])) {
                    sb.Append("@");
                    newIdent = true;
                }
                CheckValidIdentifier(names[i]);
                sb.Append(names[i]);
            }

            if (newIdent) {
                for (int i = 0; i < arrayCount; i++)
                    sb.Append("[]");
                return sb.ToString();
            }
            return originalIdentifier;
        }

        static bool IsCSharpKeyword(string value) {
            if (cSharpKeywords == null) InitKeywords();
            return cSharpKeywords.ContainsKey(value);
        }

        static void InitKeywords() {
            // build the cSharpKeywords hashtable
            cSharpKeywords = new Hashtable(75); // about 75 cSharpKeywords to be added
            Object obj = new Object();
            cSharpKeywords["abstract"] = obj;
            cSharpKeywords["base"] = obj;
            cSharpKeywords["bool"] = obj;
            cSharpKeywords["break"] = obj;
            cSharpKeywords["byte"] = obj;
            cSharpKeywords["case"] = obj;
            cSharpKeywords["catch"] = obj;
            cSharpKeywords["char"] = obj;
            cSharpKeywords["checked"] = obj;
            cSharpKeywords["class"] = obj;
            cSharpKeywords["const"] = obj;
            cSharpKeywords["continue"] = obj;
            cSharpKeywords["decimal"] = obj;
            cSharpKeywords["default"] = obj;
            cSharpKeywords["delegate"] = obj;
            cSharpKeywords["do"] = obj;
            cSharpKeywords["double"] = obj;
            cSharpKeywords["else"] = obj;
            cSharpKeywords["enum"] = obj;
            cSharpKeywords["event"] = obj;
            cSharpKeywords["exdouble"] = obj;
            cSharpKeywords["exfloat"] = obj;
            cSharpKeywords["explicit"] = obj;
            cSharpKeywords["extern"] = obj;
            cSharpKeywords["false"] = obj;
            cSharpKeywords["finally"] = obj;
            cSharpKeywords["fixed"] = obj;
            cSharpKeywords["float"] = obj;
            cSharpKeywords["for"] = obj;
            cSharpKeywords["foreach"] = obj;
            cSharpKeywords["goto"] = obj;
            cSharpKeywords["if"] = obj;
            cSharpKeywords["implicit"] = obj;
            cSharpKeywords["in"] = obj;
            cSharpKeywords["int"] = obj;
            cSharpKeywords["interface"] = obj;
            cSharpKeywords["internal"] = obj;
            cSharpKeywords["is"] = obj;
            cSharpKeywords["lock"] = obj;
            cSharpKeywords["long"] = obj;
            cSharpKeywords["namespace"] = obj;
            cSharpKeywords["new"] = obj;
            cSharpKeywords["null"] = obj;
            cSharpKeywords["object"] = obj;
            cSharpKeywords["operator"] = obj;
            cSharpKeywords["out"] = obj;
            cSharpKeywords["override"] = obj;
            cSharpKeywords["private"] = obj;
            cSharpKeywords["protected"] = obj;
            cSharpKeywords["public"] = obj;
            cSharpKeywords["readonly"] = obj;
            cSharpKeywords["ref"] = obj;
            cSharpKeywords["return"] = obj;
            cSharpKeywords["sbyte"] = obj;
            cSharpKeywords["sealed"] = obj;
            cSharpKeywords["short"] = obj;
            cSharpKeywords["sizeof"] = obj;
            cSharpKeywords["static"] = obj;
            cSharpKeywords["string"] = obj;
            cSharpKeywords["struct"] = obj;
            cSharpKeywords["switch"] = obj;
            cSharpKeywords["this"] = obj;
            cSharpKeywords["throw"] = obj;
            cSharpKeywords["true"] = obj;
            cSharpKeywords["try"] = obj;
            cSharpKeywords["typeof"] = obj;
            cSharpKeywords["uint"] = obj;
            cSharpKeywords["ulong"] = obj;
            cSharpKeywords["unchecked"] = obj;
            cSharpKeywords["unsafe"] = obj;
            cSharpKeywords["ushort"] = obj;
            cSharpKeywords["using"] = obj;
            cSharpKeywords["virtual"] = obj;
            cSharpKeywords["void"] = obj;
            cSharpKeywords["while"] = obj;
        }
    }
}
