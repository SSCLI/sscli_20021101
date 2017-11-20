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
#ifndef __EventStore_hpp
#define __EventStore_hpp

class SyncBlock;

// Used inside Thread class to chain all events that a thread is waiting for by Object::Wait
struct WaitEventLink {
    SyncBlock      *m_WaitSB;
    HANDLE          m_EventWait;
    Thread         *m_Thread;       // Owner of this WaitEventLink.
    WaitEventLink  *m_Next;         // Chain to the next waited SyncBlock.
    SLink           m_LinkSB;       // Chain to the next thread waiting on the same SyncBlock.
    DWORD           m_RefCount;     // How many times Object::Wait is called on the same SyncBlock.
};

HANDLE GetEventFromEventStore();
void StoreEventToEventStore(HANDLE hEvent);
void InitEventStore();
void TerminateEventStore();

#endif
