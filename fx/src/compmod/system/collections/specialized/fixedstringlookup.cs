//------------------------------------------------------------------------------
// <copyright file="FixedStringLookup.cs" company="Microsoft">
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

namespace System.Collections.Specialized {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    
    // This class provides a very efficient way to lookup an entry in a list of strings,
    // providing that they are declared in a particular way.
    
    // It requires the set of strings to be orderded into an array of arrays of strings.
    // The first indexer must the length of the string, so that each sub-array is of the
    // same length. The contained array must be in alphabetical order. Furthermore, if the 
    // table is to be searched case-insensitively, the strings must all be lower case.
    internal class FixedStringLookup {
        
        // Returns whether the match is found in the lookup table
        internal static bool Contains(string[][] lookupTable, string value, bool ignoreCase) {
            int length = value.Length;
            if (length <= 0 || length - 1 >= lookupTable.Length) {
                return false;
            }

            string[] subArray = lookupTable[length - 1];
            if (subArray == null) {
                return false;
            }
            return Contains(subArray, value, ignoreCase);            
        }

#if DEBUG

        internal static void VerifyLookupTable(string[][] lookupTable, bool ignoreCase) {
            for (int i = 0; i < lookupTable.Length; i++) {
                string[] subArray = lookupTable[i];
                if (subArray != null) {
                    string lastValue = null;
                    for (int j = 0; j < subArray.Length; j++) {
                        string value = subArray[j];
                        // Must all be the length of the hashed position
                        Debug.Assert(value.Length == i + 1);
                        if (lastValue != null) {
                            // Must be sorted within the sub array;
                            Debug.Assert(string.Compare(lastValue, value, ignoreCase, CultureInfo.InvariantCulture) < 0);
                        }
                        lastValue = value;
                    }
                }
            }
        }

#endif

        // This routine finds a hit within a single sorted array, with the assumption that the
        // value and all the strings are of the same length.
        private static bool Contains(string[] array, string value, bool ignoreCase) {
            int min = 0;
            int max = array.Length;
            int pos = 0;
            char searchChar;
            while (pos < value.Length) {            
                if (ignoreCase) {
                    searchChar = char.ToLower(value[pos], CultureInfo.CurrentCulture);
                } else {
                    searchChar = value[pos];
                }
                if ((max - min) <= 1) {
                    // we are down to a single item, so we can stay on this row until the end.
                    if (searchChar != array[min][pos]) {
                        return false;
                    }
                    pos++;
                    continue;
                }

                // There are multiple items to search, use binary search to find one of the hits
                if (!FindCharacter(array, searchChar, pos, ref min, ref max)) {
                    return false;
                }
                // and move to next char
                pos++;
            }
            return true;
        }

        // Do a binary search on the character array at the specific position and constrict the ranges appropriately.
        private static bool FindCharacter(string[] array, char value, int pos, ref int min, ref int max) {
            int index = min;
            while (min < max) {
                index = (min + max) / 2;
                char comp = array[index][pos];
                if (value == comp) {
                    // We have a match. Now adjust to any adjacent matches
                    int newMin = index;
                    while (newMin > min && array[newMin - 1][pos] == value) {
                        newMin--;
                    }
                    min = newMin;

                    int newMax = index + 1;
                    while (newMax < max && array[newMax][pos] == value) {
                        newMax++;
                    }
                    max = newMax;
                    return true;
                }
                if (value < comp) {
                    max = index;
                }
                else {
                    min = index + 1;
                }
            }
            return false;
        }
    }
}
