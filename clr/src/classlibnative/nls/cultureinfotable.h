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
#ifndef __CULTURE_INFO_TABLE2
#define __CULTURE_INFO_TABLE2

////////////////////////////////////////////////////////////////////////////
//
//  Class:    CultureInfoTable
//  Purpose:  This is the class to retrieve culture information like:
//            culture name, date format, currency symbol, etc.
//
//  Date: 	  January 07, 2000
//
////////////////////////////////////////////////////////////////////////////

class CultureInfoTable : public BaseInfoTable {
    public:
        static void InitializeTable();
        static CultureInfoTable* CreateInstance();
        static CultureInfoTable* GetInstance();
        virtual int GetDataItem(int cultureID);

    protected:
        virtual int GetDataItemCultureID(int dataItem);

    private:
        static CultureInfoTable* AllocateTable();

        CultureInfoTable();
        ~CultureInfoTable();

    private:
        static const CHAR m_lpCultureFileName[];
        static const WCHAR m_lpCultureMappingName[];

        static CRITICAL_SECTION  m_ProtectDefaultTable;
        static CultureInfoTable* m_pDefaultTable;
    
};

//
// The list of WORD fields:
//
#define CULTURE_IDIGITS                 0
#define CULTURE_INEGNUMBER              1
#define CULTURE_ICURRDIGITS             2
#define CULTURE_ICURRENCY               3
#define CULTURE_INEGCURR                4
#define CULTURE_ICALENDARTYPE           5
#define CULTURE_IFIRSTDAYOFWEEK         6
#define CULTURE_IFIRSTWEEKOFYEAR        7
#define CULTURE_ILANGUAGE               8
#define CULTURE_WIN32LCID               9
#define CULTURE_INEGATIVEPERCENT        10
#define CULTURE_IPOSITIVEPERCENT        11
#define CULTURE_IDEFAULTANSICODEPAGE    12
#define CULTURE_IDEFAULTOEMCODEPAGE     13
#define CULTURE_IDEFAULTMACCODEPAGE     14
#define CULTURE_IDEFAULTEBCDICCODEPAGE  15
#define CULTURE_IPARENT                 16
#define CULTURE_IREGIONITEM             17

//
// The list of string fields
//

#define CULTURE_SLIST               0
#define CULTURE_SDECIMAL            1
#define CULTURE_STHOUSAND           2
#define CULTURE_SGROUPING           3
#define CULTURE_SCURRENCY           4
#define CULTURE_SMONDECIMALSEP      5
#define CULTURE_SMONTHOUSANDSEP     6
#define CULTURE_SMONGROUPING        7
#define CULTURE_SPOSITIVESIGN       8
#define CULTURE_SNEGATIVESIGN       9
#define CULTURE_STIMEFORMAT         10
#define CULTURE_STIME               11
#define CULTURE_S1159               12
#define CULTURE_S2359               13
#define CULTURE_SSHORTDATE          14
#define CULTURE_SDATE               15
#define CULTURE_SLONGDATE           16
#define CULTURE_SNAME               17
#define CULTURE_SENGDISPLAYNAME     18
#define CULTURE_SABBREVLANGNAME     19
#define CULTURE_SISO639LANGNAME     20
#define CULTURE_SISO639LANGNAME2    21
#define CULTURE_SNATIVEDISPLAYNAME  22
#define CULTURE_SPERCENT            23
#define CULTURE_SNAN                24
#define CULTURE_SPOSINFINITY        25
#define CULTURE_SNEGINFINITY        26
#define CULTURE_SSHORTTIME          27
#define CULTURE_SYEARMONTH          28
#define CULTURE_SMONTHDAY           29
#define CULTURE_SDAYNAME1           30
#define CULTURE_SDAYNAME2           31
#define CULTURE_SDAYNAME3           32
#define CULTURE_SDAYNAME4           33
#define CULTURE_SDAYNAME5           34
#define CULTURE_SDAYNAME6           35
#define CULTURE_SDAYNAME7           36
#define CULTURE_SABBREVDAYNAME1     37
#define CULTURE_SABBREVDAYNAME2     38
#define CULTURE_SABBREVDAYNAME3     39
#define CULTURE_SABBREVDAYNAME4     40
#define CULTURE_SABBREVDAYNAME5     41
#define CULTURE_SABBREVDAYNAME6     42
#define CULTURE_SABBREVDAYNAME7     43
#define CULTURE_SMONTHNAME1         44
#define CULTURE_SMONTHNAME2         45
#define CULTURE_SMONTHNAME3         46
#define CULTURE_SMONTHNAME4         47
#define CULTURE_SMONTHNAME5         48
#define CULTURE_SMONTHNAME6         49
#define CULTURE_SMONTHNAME7         50
#define CULTURE_SMONTHNAME8         51
#define CULTURE_SMONTHNAME9         52
#define CULTURE_SMONTHNAME10        53
#define CULTURE_SMONTHNAME11        54
#define CULTURE_SMONTHNAME12        55
#define CULTURE_SMONTHNAME13        56
#define CULTURE_SABBREVMONTHNAME1   57
#define CULTURE_SABBREVMONTHNAME2   58
#define CULTURE_SABBREVMONTHNAME3   59
#define CULTURE_SABBREVMONTHNAME4   60
#define CULTURE_SABBREVMONTHNAME5   61
#define CULTURE_SABBREVMONTHNAME6   62
#define CULTURE_SABBREVMONTHNAME7   63
#define CULTURE_SABBREVMONTHNAME8   64
#define CULTURE_SABBREVMONTHNAME9   65
#define CULTURE_SABBREVMONTHNAME10  66
#define CULTURE_SABBREVMONTHNAME11  67
#define CULTURE_SABBREVMONTHNAME12  68
#define CULTURE_SABBREVMONTHNAME13  69
#define CULTURE_NLPIOPTIONCALENDAR  70


#endif  // __CULTURE_INFO_TABLE2

