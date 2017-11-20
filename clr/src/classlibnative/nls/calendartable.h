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
#ifndef __CALENDAR_TABLE_TABLE
#define __CALENDAR_TABLE_TABLE

class CalendarTable: public BaseInfoTable {
    public:
        static void InitializeTable();
        static CalendarTable* CreateInstance();
        static CalendarTable* GetInstance();

        virtual int  GetDataItem(int calendarID);

    protected:
        virtual int GetDataItemCultureID(int dataItem); 
    private:
        static CalendarTable* AllocateTable();

        CalendarTable();
        ~CalendarTable();
    private:
        static const CHAR m_lpFileName[];
        static const WCHAR m_lpwMappingName[];

        static CRITICAL_SECTION  m_ProtectDefaultTable;
        static CalendarTable* m_pDefaultTable;
};
#endif

