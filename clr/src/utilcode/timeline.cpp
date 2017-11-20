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
#include "stdafx.h"

#include "timeline.h"
#include "utilcode.h"

#if ENABLE_TIMELINE

Timeline Timeline::g_Timeline;

#define FPRINTF fprintf
#define VFPRINTF vfprintf
#define FOPEN fopen
#define FCLOSE fclose

void Timeline::Startup() 
{ 
    g_Timeline.Init(); 
}

void Timeline::Shutdown() 
{ 
    g_Timeline.Destroy(); 
}

void Timeline::Init()
{
    m_enabled = REGUTIL::GetConfigDWORD(L"Timeline", 0);

    if (m_enabled == 0)
        goto Done;


    m_out = FOPEN("TIMELINE.LOG", "w+");

    QueryPerformanceFrequency(&m_frequency);
    m_frequency.QuadPart /= 1000; // We'll report times in ms.

    QueryPerformanceCounter(&m_lastTime[0]);

Done:
    m_lastLevel = -1;

    if (m_enabled != 0)
        EventStart("Timeline");
}

void Timeline::Destroy()
{
    if (m_enabled != 0)
    {
        EventEnd("Timeline\n");
        FCLOSE(m_out);
    }
}

void Timeline::Stamp(int level)
{
    if (level >= MAX_LEVEL)
        return;

    //
    // Record this time in our slot.
    //

    timestamp now;
    QueryPerformanceCounter(&now);

    //
    // Print indentation and timestamps
    //

    for (int i=0; i<=level; i++)
    {
        if (i > m_lastLevel)
        {
            m_lastTime[i] = now;
            
            FPRINTF(m_out, "------- ");
        }
        else
        {
            __int64 interval = now.QuadPart - m_lastTime[i].QuadPart;

            FPRINTF(m_out, "%+07.3f ", (double) (interval / m_frequency.QuadPart) / 1000.0);
        }
    }
}

void Timeline::Event(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);
    
    Stamp(m_lastLevel);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);
    
    va_end(args);
}

void Timeline::EventStart(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);

    Stamp(m_lastLevel+1);
    m_lastLevel++;
    VFPRINTF(m_out, "Start ", NULL);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);

    va_end(args);
}

void Timeline::EventEnd(LPCSTR string, ...)
{
    va_list args;
    va_start(args, string);
    
    Stamp(m_lastLevel);
    m_lastLevel--;
    VFPRINTF(m_out, "End ", NULL);
    VFPRINTF(m_out, string, args);
    VFPRINTF(m_out, "\n", NULL);

    va_end(args);
}

#endif // ENABLE_TIMELINE

