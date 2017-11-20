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
namespace System {
    using System.Text;
    using System.Threading;
    using System.Globalization;
    using ArrayList = System.Collections.ArrayList;
    /*  
     Customized format patterns:
     P.S. Format in the table below is the internal number format used to display the pattern.

     Patterns   Format      Description                           Example
     =========  ==========  ===================================== ========
        "h"     "0"         hour (12-hour clock)w/o leading zero  3
        "hh"    "00"        hour (12-hour clock)with leading zero 03
        "hh*"   "00"        hour (12-hour clock)with leading zero 03

        "H"     "0"         hour (24-hour clock)w/o leading zero  8
        "HH"    "00"        hour (24-hour clock)with leading zero 08
        "HH*"   "00"        hour (24-hour clock)                  08

        "m"     "0"         minute w/o leading zero
        "mm"    "00"        minute with leading zero
        "mm*"   "00"        minute with leading zero

        "s"     "0"         second w/o leading zero
        "ss"    "00"        second with leading zero
        "ss*"   "00"        second with leading zero

        "f"     "0"         second fraction (1 digit)
        "ff"    "00"        second fraction (2 digit)
        "fff"   "000"       second fraction (3 digit)
        "ffff"  "0000"      second fraction (4 digit)
        "fffff" "00000"         second fraction (5 digit)
        "ffffff"    "000000"    second fraction (6 digit)
        "fffffff"   "0000000"   second fraction (7 digit)

        "t"                 first character of AM/PM designator   A
        "tt"                AM/PM designator                      AM
        "tt*"               AM/PM designator                      PM

        "d"     "0"         day w/o leading zero                  1
        "dd"    "00"        day with leading zero                 01
        "ddd"               short weekday name (abbreviation)     Mon
        "dddd"              full weekday name                     Monday
        "dddd*"             full weekday name                     Monday
        

        "M"     "0"         month w/o leading zero                2
        "MM"    "00"        month with leading zero               02
        "MMM"               short month name (abbreviation)       Feb
        "MMMM"              full month name                       Febuary
        "MMMM*"             full month name                       Febuary
       
        "y"     "0"         two digit year (year % 100) w/o leading zero           0
        "yy"    "00"        two digit year (year % 100) with leading zero          00
        "yyy"   "D3"        year                                  2000
        "yyyy"  "D4"        year                                  2000
        "yyyyy" "D5"        year                                  2000
        ...

        "z"     "+0;-0"     timezone offset w/o leading zero      -8
        "zz"    "+00;-00"   timezone offset with leading zero     -08
        "zzz"   "+00;-00" for hour offset, "00" for minute offset   full timezone offset   -08:00
        "zzz*"  "+00;-00" for hour offset, "00" for minute offset   full timezone offset   -08:00

        "g*"                the current era name                  A.D.

        ":"                 time separator                        :
        "/"                 date separator                        /
        "'"                 quoted string                         'ABC' will insert ABC into the formatted string.
        '"'                 quoted string                         "ABC" will insert ABC into the formatted string.
        "%"                 used to quote a single pattern characters      E.g.The format character "%y" is to print two digit year.
        "\"                 escaped character                     E.g. '\d' insert the character 'd' into the format string.
        other characters    insert the character into the format string. 

    Pre-defined format characters: 
        (U) to indicate Universal time is used.
        (G) to indicate Gregorian calendar is used.
    
        Format              Description                             Real format                             Example
        =========           =================================       ======================                  =======================
        "d"                 short date                              culture-specific                        10/31/1999
        "D"                 long data                               culture-specific                        Sunday, October 31, 1999
        "f"                 full date (long date + short time)      culture-specific                        Sunday, October 31, 1999 2:00 AM
        "F"                 full date (long date + long time)       culture-specific                        Sunday, October 31, 1999 2:00:00 AM
        "g"                 general date (short date + short time)  culture-specific                        10/31/1999 2:00 AM
        "G"                 general date (short date + long time)   culture-specific                        10/31/1999 2:00:00 AM
        "m"/"M"             Month/Day date                          culture-specific                        October 31
(G)     "r"/"R"             RFC 1123 date,                          "ddd, dd MMM yyyy HH':'mm':'ss 'GMT'"   Sun, 31 Oct 1999 10:00:00 GMT
(G)     "s"                 Sortable format, based on ISO 8601.     "yyyy-MM-dd'T'HH:mm:ss"                 1999-10-31T02:00:00
                                                                    ('T' for local time)
        "t"                 short time                              culture-specific                        2:00 AM
        "T"                 long time                               culture-specific                        2:00:00 AM
(G)     "u"                 Universal time with sortable format,    "yyyy'-'MM'-'dd HH':'mm':'ss'Z'"        1999-10-31 10:00:00Z
                            based on ISO 8601.
(U)     "U"                 Universal time with full                culture-specific                        Sunday, October 31, 1999 10:00:00 AM
                            (long date + long time) format
                            "y"/"Y"             Year/Month day                          culture-specific                        October, 1999

    */    

    //This class contains only static members and does not require the serializable attribute.    
    internal 
    class DateTimeFormat {
        
        internal const int MaxSecondsFractionDigits = 7;
        
        internal static char[] allStandardFormats = 
        {
            'd', 'D', 'f', 'F', 'g', 'G', 
            'm', 'M', 'r', 'R', 's', 't', 
            'T', 'u', 'U', 'y', 'Y',
        };
    
        private const int DEFAULT_ALL_DATETIMES_SIZE = 132; 

        private unsafe static void FormatDigits(StringBuilder outputBuffer, int digits, int repeat) {
            char* buffer = stackalloc char[16];
            char* p = buffer + 16;
            int n = digits;
            do {
                *--p = (char)(n % 10 + '0');
                n /= 10;
            } while (n != 0);
            
            int length = (int) (buffer + 16 - p);
            
            //If the repeat count is greater than 0, we're trying
            //to emulate the "00" format, so we have to prepend
            //a zero if the string only has one character.
            if (repeat>1 && (length==1)) {
                *--p='0';
                length++;
            }
            outputBuffer.Append(p, length);
        }

        private unsafe static void HebrewFormatDigits(StringBuilder outputBuffer, int digits) {
            outputBuffer.Append(HebrewCalendar.NumberToHebrewLetter(digits));
        }
        
        static int ParseRepeatPattern(String format, int pos, char patternChar)
        {
            int len = format.Length;
            int index = pos + 1;
            while ((index < len) && (format[index] == patternChar)) 
{
                index++;
            }    
            return (index - pos);
        }
        
        private static String FormatDayOfWeek(int dayOfWeek, int repeat, DateTimeFormatInfo dtfi)
        {
            BCLDebug.Assert(dayOfWeek >= 0 && dayOfWeek <= 6, "dayOfWeek >= 0 && dayOfWeek <= 6");
            if (repeat == 3)
            {            
                return (dtfi.GetAbbreviatedDayName((DayOfWeek)dayOfWeek));
            }
            // Call dtfi.GetDayName() here, instead of accessing DayNames property, because we don't
            // want a clone of DayNames, which will hurt perf.
            return (dtfi.GetDayName((DayOfWeek)dayOfWeek));
        }
    
        private static String FormatMonth(int month, int repeatCount, DateTimeFormatInfo dtfi)
        {
            BCLDebug.Assert(month >=1 && month <= 12, "month >=1 && month <= 12");
            if (repeatCount == 3)
            {
                return (dtfi.GetAbbreviatedMonthName(month));
            }
            // Call GetMonthName() here, instead of accessing MonthNames property, because we don't
            // want a clone of MonthNames, which will hurt perf.
            return (dtfi.GetMonthName(month));
        }

        private static String FormatHebrewMonthName(DateTime time, int month, int repeatCount, DateTimeFormatInfo dtfi)
        {
            if (!dtfi.Calendar.IsLeapYear(dtfi.Calendar.GetYear(time)) && (month >= 7)) {
                month++;
            }
            if (repeatCount == 3) {
                return (dtfi.GetAbbreviatedMonthName(month));
            }
            return (dtfi.GetMonthName(month));
        }
    
        //
        // The pos should point to a quote character. This method will
        // get the string encloed by the quote character.
        //
        internal static int ParseQuoteString(String format, int pos, StringBuilder result)
        {
            //
            // NOTE                     : pos will be the index of the quote character in the 'format' string.
            //
            int formatLen = format.Length;
            int beginPos = pos;
            char quoteChar = format[pos++]; // Get the character used to quote the following string.
    
            bool foundQuote = false;
            while (pos < formatLen)
            {
                char ch = format[pos++];        
                if (ch == quoteChar)
                {
                    foundQuote = true;
                    break;
                }
                else if (ch == '\\') {
                    // The following are used to support escaped character.
                    // Escaped character is also supported in the quoted string.
                    // Therefore, someone can use a format like "'minute:' mm\"" to display:
                    //  minute: 45"
                    // because the second double quote is escaped.
                    if (pos < formatLen) {
                        result.Append(format[pos++]);
                    } else {
                            //
                            // This means that '\' is at the end of the formatting string.
                            //
                            throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
                    }                    
                } else {
                    result.Append(ch);
                }
            }
            
            if (!foundQuote)
            {
                // Here we can't find the matching quote.
                throw new FormatException(String.Format(Environment.GetResourceString("Format_BadQuote"), quoteChar));
            }
            
            //
            // Return the character count including the begin/end quote characters and enclosed string.
            //
            return (pos - beginPos);
        }
    
        //
        // Get the next character at the index of 'pos' in the 'format' string.
        // Return value of -1 means 'pos' is already at the end of the 'format' string.
        // Otherwise, return value is the int value of the next character.
        //
        private static int ParseNextChar(String format, int pos)
        {
            if (pos >= format.Length - 1)
            {
                return (-1);
            }
            return ((int)format[pos+1]);
        }

        
        private static String FormatCustomized(DateTime dateTime, String format, DateTimeFormatInfo dtfi) {
            Calendar cal = dtfi.Calendar;
            StringBuilder result = new StringBuilder();
            // This is a flag to indicate if we are format the dates using Hebrew calendar.

            bool isHebrewCalendar = (cal.ID == Calendar.CAL_HEBREW);
            // This is a flag to indicate if we are formating hour/minute/second only.
            bool bTimeOnly = true;
                        
            int i = 0;
            int tokenLen, hour12;
            
            while (i < format.Length) {
                char ch = format[i];
                int nextChar;
                switch (ch) {
                    case 'g':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        result.Append(dtfi.GetEraName(cal.GetEra(dateTime)));
                        break;
                    case 'h':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        hour12 = dateTime.Hour % 12;
                        if (hour12 == 0)
                        {
                            hour12 = 12;
                        }
                        FormatDigits(result, hour12, tokenLen);
                        break;
                    case 'H':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        FormatDigits(result, dateTime.Hour, tokenLen);
                        break;
                    case 'm':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        FormatDigits(result, dateTime.Minute, tokenLen);
                        break;
                    case 's':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        FormatDigits(result, dateTime.Second, tokenLen);
                        break;
                    case 'f':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        if (tokenLen <= MaxSecondsFractionDigits) {
                            long fraction = (dateTime.Ticks % Calendar.TicksPerSecond);
                            fraction = fraction / (long)Math.Pow(10, 7 - tokenLen);
                            result.Append(((int)fraction).ToString((new String('0', tokenLen))));
                        } else {
                            throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
                        }
                        break;
                    case 't':
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        if (tokenLen == 1)
                        {
                            if (dateTime.Hour < 12)
                            {
                                if (dtfi.AMDesignator.Length >= 1)
                                {
                                    result.Append(dtfi.AMDesignator[0]);
                                }
                            }
                            else
                            {
                                if (dtfi.PMDesignator.Length >= 1)
                                {
                                    result.Append(dtfi.PMDesignator[0]);
                                }
                            }
                            
                        }
                        else
                        {
                            result.Append((dateTime.Hour < 12 ? dtfi.AMDesignator : dtfi.PMDesignator));
                        }
                        break;
                    case 'd':
                        //
                        // tokenLen == 1 : Day of month as digits with no leading zero.
                        // tokenLen == 2 : Day of month as digits with leading zero for single-digit months.
                        // tokenLen == 3 : Day of week as a three-leter abbreviation.
                        // tokenLen >= 4 : Day of week as its full name.
                        //
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        if (tokenLen <= 2)
                        {
                            int day = cal.GetDayOfMonth(dateTime);
                            if (isHebrewCalendar) {
                                // For Hebrew calendar, we need to convert numbers to Hebrew text for yyyy, MM, and dd values.
                                HebrewFormatDigits(result, day);
                            } else {
                                FormatDigits(result, day, tokenLen);
                            }
                        } 
                        else
                        {
                            int dayOfWeek = (int)cal.GetDayOfWeek(dateTime);
                            result.Append(FormatDayOfWeek(dayOfWeek, tokenLen, dtfi));
                        }
                        bTimeOnly = false;
                        break;
                    case 'M':
                        // 
                        // tokenLen == 1 : Month as digits with no leading zero.
                        // tokenLen == 2 : Month as digits with leading zero for single-digit months.
                        // tokenLen == 3 : Month as a three-letter abbreviation.
                        // tokenLen >= 4 : Month as its full name.
                        //
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        int month = cal.GetMonth(dateTime);
                        if (tokenLen <= 2)
                        {
                            if (isHebrewCalendar) {
                                // For Hebrew calendar, we need to convert numbers to Hebrew text for yyyy, MM, and dd values.
                                HebrewFormatDigits(result, month);
                            } else {                        
                                FormatDigits(result, month, tokenLen);
                            }
                        } 
                        else
                        {
                            if (isHebrewCalendar) {
                                result.Append(FormatHebrewMonthName(dateTime, month, tokenLen, dtfi));
                            } else {             
                                result.Append(FormatMonth(month, tokenLen, dtfi));
                            }
                        }
                        bTimeOnly = false;
                        break;
                    case 'y':
                        int year = cal.GetYear(dateTime);
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        switch (cal.ID) {
                            // Add a special case for Japanese.  
                            // For Japanese calendar, always use two digit (with leading 0)
                            case (Calendar.CAL_JAPAN):
                                FormatDigits(result, year, 2);
                                break;
                            case (Calendar.CAL_HEBREW):
                                HebrewFormatDigits(result, year);
                                break;
                            default:
                                if (tokenLen <= 2) {
                                    FormatDigits(result, year % 100, tokenLen);
                                } else {
                                    String fmtPattern = "D" + tokenLen;
                                    result.Append(year.ToString(fmtPattern));
                                }
                                break;
                        }
                        bTimeOnly = false;
                        break;
                    case 'z':
                        //
                        // Output the offset of the timezone according to the system timezone setting.
                        //
                        tokenLen = ParseRepeatPattern(format, i, ch);
                        TimeSpan offset;
                        if (bTimeOnly && dateTime.Ticks < Calendar.TicksPerDay) {
                            offset = TimeZone.CurrentTimeZone.GetUtcOffset(DateTime.Now);
                        } else {
                            offset = TimeZone.CurrentTimeZone.GetUtcOffset(dateTime);
                        }
    
                        switch (tokenLen)
                        {
                            case 1:
                                result.Append((offset.Hours).ToString("+0;-0"));
                                break;
                            case 2:
                                result.Append((offset.Hours).ToString("+00;-00"));
                                break;
                            default:
                                if (offset.Hours > 0) {
                                    result.Append(String.Format("+{0:00}:{1:00}", offset.Hours, offset.Minutes));
                                } else {
                                    // When the offset is negative, note that the offset.Minute is also negative.
                                    // So use should use -offset.Minute to get the postive value.
                                    result.Append(String.Format("-{0:00}:{1:00}", -offset.Hours, -offset.Minutes));
                                }
                                break;                        
                        }
                        break;
                    case ':':
                        result.Append(dtfi.TimeSeparator);
                        tokenLen = 1;
                        break;
                    case '/':
                        result.Append(dtfi.DateSeparator);
                        tokenLen = 1;
                        break;
                    case '\'':
                    case '\"':
                        StringBuilder enquotedString = new StringBuilder();
                        tokenLen = ParseQuoteString(format, i, enquotedString); 
                        result.Append(enquotedString);
                        break;
                    case '%':
                        // Optional format character.
                        // For example, format string "%d" will print day of month 
                        // without leading zero.  Most of the cases, "%" can be ignored.
                        nextChar = ParseNextChar(format, i);
                        // nextChar will be -1 if we already reach the end of the format string.
                        // Besides, we will not allow "%%" appear in the pattern.
                        if (nextChar >= 0 && nextChar != (int)'%') {
                            result.Append(FormatCustomized(dateTime, ((char)nextChar).ToString(), dtfi));
                            tokenLen = 2;
                        }
                        else
                        {
                            //
                            // This means that '%' is at the end of the format string or
                            // "%%" appears in the format string.
                            //
                            throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
                        }
                        break;
                    case '\\':
                        // Escaped character.  Can be used to insert character into the format string.
                        // For exmple, "\d" will insert the character 'd' into the string.
                        //
                        // NOTENOTE                     : we can remove this format character if we enforce the enforced quote 
                        // character rule.
                        // That is, we ask everyone to use single quote or double quote to insert characters,
                        // then we can remove this character.
                        //
                        nextChar = ParseNextChar(format, i);
                        if (nextChar >= 0)
                        {
                            result.Append(((char)nextChar));
                            tokenLen = 2;
                        } 
                        else
                        {
                            //
                            // This means that '\' is at the end of the formatting string.
                            //
                            throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
                        }
                        break;
                    default:
                        // NOTENOTE                     : we can remove this rule if we enforce the enforced quote
                        // character rule.
                        // That is, if we ask everyone to use single quote or double quote to insert characters,
                        // then we can remove this default block.
                        result.Append(ch);
                        tokenLen = 1;
                        break;
                }
                i += tokenLen;
            }
            return (result.ToString());
        }
    
        internal static String GetRealFormat(String format, DateTimeFormatInfo dtfi)
        {
            String realFormat = null;
            
            switch (format[0])
            {
                case 'd':       // Short Date
                    realFormat = dtfi.ShortDatePattern;
                    break;
                case 'D':       // Long Date
                    realFormat = dtfi.LongDatePattern;
                    break;
                case 'f':       // Full (long date + short time)
                    realFormat = dtfi.LongDatePattern + " " + dtfi.ShortTimePattern;
                    break;
                case 'F':       // Full (long date + long time)
                    realFormat = dtfi.FullDateTimePattern;
                    break;
                case 'g':       // General (short date + short time)
                    realFormat = dtfi.GeneralShortTimePattern;
                    break;
                case 'G':       // General (short date + long time)
                    realFormat = dtfi.GeneralLongTimePattern;
                    break;
                case 'm':
                case 'M':       // Month/Day Date
                    realFormat = dtfi.MonthDayPattern;
                    break;
                case 'r':
                case 'R':       // RFC 1123 Standard                    
                    realFormat = dtfi.RFC1123Pattern;
                    break;
                case 's':       // Sortable without Time Zone Info
                    realFormat = dtfi.SortableDateTimePattern;
                    break;
                case 't':       // Short Time
                    realFormat = dtfi.ShortTimePattern;
                    break;
                case 'T':       // Long Time
                    realFormat = dtfi.LongTimePattern;
                    break;
                case 'u':       // Universal with Sortable format
                    realFormat = dtfi.UniversalSortableDateTimePattern;
                    break;
                case 'U':       // Universal with Full (long date + long time) format
                    realFormat = dtfi.FullDateTimePattern;
                    break;
                case 'y':
                case 'Y':       // Year/Month Date
                    realFormat = dtfi.YearMonthPattern;
                    break;
                default:
                    throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
            }
            return (realFormat);
        }    

        // Expand a pre-defined format string (like "D" for long date) to the real format that
        // we are going to use in the date time parsing.
        // This method also convert the dateTime if necessary (e.g. when the format is in Universal time),
        // and change dtfi if necessary (e.g. when the format should use invariant culture).
        //
        private static String ExpandPredefinedFormat(String format, ref DateTime dateTime, ref DateTimeFormatInfo dtfi) {
            switch (format[0]) {
                case 'r':
                case 'R':       // RFC 1123 Standard                    
//                    dateTime = dateTime.ToUniversalTime();                        
                    dtfi = DateTimeFormatInfo.InvariantInfo;
                    break;
                case 's':       // Sortable without Time Zone Info                
                    dtfi = DateTimeFormatInfo.InvariantInfo;
                    break;                    
                case 'u':       // Universal time in sortable format.
//                    dateTime = dateTime.ToUniversalTime();                        
                    // Universal time is always in Greogrian calendar.
                    dtfi = DateTimeFormatInfo.InvariantInfo;
                    break;
                case 'U':       // Universal time in culture dependent format.
                    // Universal time is always in Greogrian calendar.
                    //
                    //
                    dtfi = (DateTimeFormatInfo)dtfi.Clone();
                    if (dtfi.Calendar.GetType() != typeof(GregorianCalendar)) {
                        dtfi.Calendar = GregorianCalendar.GetDefaultInstance();
                    }
                    dateTime = dateTime.ToUniversalTime();
                    break;
            }
            format = GetRealFormat(format, dtfi);
            return (format);
        }
        
        internal static String Format(DateTime dateTime, String format, DateTimeFormatInfo dtfi)
        {            
            if (format==null || format.Length==0) {
                format = "G";            
                if (dateTime.Ticks < Calendar.TicksPerDay) {
                    // If the time is less than 1 day, consider it as time of day.
                    // Just print out the short time format.
                    //
                    // This is a workaround for VB, since they use ticks less then one day to be 
                    // time of day.  In cultures which use calendar other than Gregorian calendar, these
                    // alternative calendar may not support ticks less than a day.
                    // For example, Japanese calendar only supports date after 1868/9/8.
                    // This will pose a problem when people in VB get the time of day, and use it
                    // to call ToString(), which will use the general format (short date + long time).
                    // Since Japanese calendar does not support Gregorian year 0001, an exception will be
                    // thrown when we try to get the Japanese year for Gregorian year 0001.
                    // Therefore, the workaround allows them to call ToString() for time of day from a DateTime by
                    // formatting as ISO 8601 format.
                    switch (dtfi.Calendar.ID) {
                        case Calendar.CAL_JAPAN: 
                        case Calendar.CAL_TAIWAN:
                        case Calendar.CAL_HIJRI:
                        case Calendar.CAL_HEBREW:
                        case Calendar.CAL_JULIAN:
                            format = "s";
                            break;                        
                    }
                }
            }            

            if (format.Length == 1) {
                format = ExpandPredefinedFormat(format, ref dateTime, ref dtfi);
            } 
            
            return (FormatCustomized(dateTime, format, dtfi));
        }
    
        internal static String[] GetAllDateTimes(DateTime dateTime, char format, DateTimeFormatInfo dtfi)
        {
            String [] allFormats    = null;
            String [] results       = null;
            
            switch (format)
            {
                case 'd':
                case 'D':                            
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                case 'm':
                case 'M':
                case 't':
                case 'T':                
                case 'y':
                case 'Y':
                    allFormats = dtfi.GetAllDateTimePatterns(format);
                    results = new String[allFormats.Length];
                    for (int i = 0; i < allFormats.Length; i++)
                    {
                        results[i] = Format(dateTime, allFormats[i], dtfi);
                    }
                    break;
                case 'U':
                    DateTime universalTime = dateTime.ToUniversalTime();
                    allFormats = dtfi.GetAllDateTimePatterns(format);
                    results = new String[allFormats.Length];
                    for (int i = 0; i < allFormats.Length; i++)
                    {
                        results[i] = Format(universalTime, allFormats[i], dtfi);
                    }
                    break;                
                //
                // The following ones are special cases because these patterns are read-only in
                // DateTimeFormatInfo.
                //
                case 'r':
                case 'R':
                case 's':
                case 'u':            
                    results = new String[] {Format(dateTime, new String(new char[] {format}), dtfi)};
                    break;            
                default:
                    throw new FormatException(Environment.GetResourceString("Format_InvalidString"));
                
            }
            return (results);
        }
    
        internal static String[] GetAllDateTimes(DateTime dateTime, DateTimeFormatInfo dtfi)
        {
            ArrayList results = new ArrayList(DEFAULT_ALL_DATETIMES_SIZE);
            
            for (int i = 0; i < allStandardFormats.Length; i++)
            {
                String[] strings = GetAllDateTimes(dateTime, allStandardFormats[i], dtfi);
                for (int j = 0; j < strings.Length; j++)
                {
                    results.Add(strings[j]);
                }
            }
            String[] value = new String[results.Count];
            results.CopyTo(0, value, 0, results.Count);
            return (value);
        }
    }
}
