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
#include "common.h"
#include "eventstore.hpp"

// A class to maintain a pool of available events.

const int EventStoreLength = 8;
class EventStore
{
public: 
    // Note: No constructors/destructors - global instance

    void Init()
    {
        m_EventStoreCrst.Init("EventStore", CrstEventStore);
        m_Store = NULL;
    }

    void Destroy()
    {
        _ASSERTE (g_fEEShutDown);

        m_EventStoreCrst.Destroy();

        EventStoreElem *walk;
        EventStoreElem *next;

        walk = m_Store;
        while (walk) {
            next = walk->next;
            delete (walk);
            walk = next;
        }
    }

    void StoreHandleForEvent (HANDLE handle)
    {
        _ASSERTE (handle);
        CLR_CRST (&m_EventStoreCrst);
        if (m_Store == NULL) {
            m_Store = new EventStoreElem ();
        }
        EventStoreElem *walk;
#ifdef _DEBUG
        // See if we have some leakage.
        LONG count = 0;
        walk = m_Store; 
        while (walk) {
            count += walk->AvailableEventCount();
            walk = walk->next;
        }
        // The number of events stored in the pool should be small.
        _ASSERTE (count <= g_pThreadStore->ThreadCountInEE() * 2 + 10);
#endif
        walk = m_Store;        
        while (walk) {
            if (walk->StoreHandleForEvent (handle) )
                return;
            if (walk->next == NULL) {
                break;
            }
            walk = walk->next;
        }
        walk->next = new EventStoreElem ();
        walk->next->hArray[0] = handle;
    }

    HANDLE GetHandleForEvent ()
    {
        HANDLE handle;
        CLR_CRST (&m_EventStoreCrst);
        EventStoreElem *walk = m_Store;
        while (walk) {
            handle = walk->GetHandleForEvent();
            if (handle != INVALID_HANDLE_VALUE) {
                return handle;
            }
            walk = walk->next;
        }
        handle = ::WszCreateEvent (NULL, TRUE/*ManualReset*/,
                                          TRUE/*Signalled*/, NULL);
        _ASSERTE (handle != INVALID_HANDLE_VALUE);
        return handle;
    }

private:
    struct EventStoreElem
    {
        HANDLE hArray[EventStoreLength];
        EventStoreElem *next;
        
        EventStoreElem ()
        {
            next = NULL;
            for (int i = 0; i < EventStoreLength; i ++) {
                hArray[i] = INVALID_HANDLE_VALUE;
            }
        }

        ~EventStoreElem ()
        {
            for (int i = 0; i < EventStoreLength; i++) {
                if (hArray[i]) {
                    CloseHandle (hArray[i]);
                    hArray[i] = INVALID_HANDLE_VALUE;
                }
            }
        }

        // Store a handle in the current EventStoreElem.  Return TRUE if succeessful.
        // Return FALSE if failed due to no free slot.
        BOOL StoreHandleForEvent (HANDLE handle)
        {
            int i;
            for (i = 0; i < EventStoreLength; i++) {
                if (hArray[i] == INVALID_HANDLE_VALUE) {
                    hArray[i] = handle;
                    return TRUE;
                }
            }
            return FALSE;
        }

        // Get a handle from the current EventStoreElem.
        HANDLE GetHandleForEvent ()
        {
            int i;
            for (i = 0; i < EventStoreLength; i++) {
                if (hArray[i] != INVALID_HANDLE_VALUE) {
                    HANDLE handle = hArray[i];
                    hArray[i] = INVALID_HANDLE_VALUE;
                    return handle;
                }
            }

            return INVALID_HANDLE_VALUE;
        }

#ifdef _DEBUG
        LONG AvailableEventCount ()
        {
            LONG count = 0;
            for (int i = 0; i < EventStoreLength; i++) {
                if (hArray[i] != INVALID_HANDLE_VALUE) {
                    count ++;
                }
            }
            return count;
        }
#endif
    };

    EventStoreElem  *m_Store;
    
    // Critical section for adding and removing event used for Object::Wait
    CrstStatic      m_EventStoreCrst;
};

static EventStore s_EventStore;
 
HANDLE GetEventFromEventStore()
{
    return s_EventStore.GetHandleForEvent();
}

void StoreEventToEventStore(HANDLE hEvent)
{
    s_EventStore.StoreHandleForEvent(hEvent);
}

void InitEventStore()
{
    s_EventStore.Init();
}

void TerminateEventStore()
{
    s_EventStore.Destroy();
}
