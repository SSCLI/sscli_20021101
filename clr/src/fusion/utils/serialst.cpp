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
/*++

Module Name:

    serialst.cxx

Abstract:

    Functions to deal with a serialized list. These are replaced by macros in
    the retail version

    Contents:
        [InitializeSerializedList]
        [TerminateSerializedList]
        [LockSerializedList]
        [UnlockSerializedList]
        [InsertAtHeadOfSerializedList]
        [InsertAtTailOfSerializedList]
        [RemoveFromSerializedList]
        [IsSerializedListEmpty]
        [HeadOfSerializedList]
        [TailOfSerializedList]
        [CheckEntryOnSerializedList]
        [(CheckEntryOnList)]
        SlDequeueHead
        SlDequeueTail
        IsOnSerializedList

Author:

                                            16-Feb-1995

Environment:

    Win-32 user level

Revision History:


--*/

#include "fusionp.h"
#include "debmacro.h"
#include "serialst.h"



#if DBG

#if !defined(PRIVATE)
#define PRIVATE static
#endif

#define DEBUG_FUNCTION

#if !defined(DEBUG_PRINT)
#define DEBUG_PRINT(foo, bar, baz)
#endif

#define ENDEXCEPT

#if !defined(DEBUG_BREAK)
#define DEBUG_BREAK(foo) DebugBreak()
#endif

//
// manifests
//

#define SERIALIZED_LIST_SIGNATURE   0x74736c53      /* 'tslS' */

//
// private prototypes
//

PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    );

//
// data
//

BOOL fCheckEntryOnList = FALSE;
BOOL ReportCheckEntryOnListErrors = FALSE;

//
// functions
//


DEBUG_FUNCTION
VOID
InitializeSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    initializes a serialized list

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    None.

--*/

{
    ASSERT(SerializedList != NULL);

    SerializedList->Signature = SERIALIZED_LIST_SIGNATURE;
    SerializedList->LockCount = 0;


    InitializeListHead(&SerializedList->List);
    SerializedList->ElementCount = 0;
    InitializeCriticalSection(&SerializedList->Lock);
}


DEBUG_FUNCTION
VOID
TerminateSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Undoes InitializeSerializeList

Arguments:

    SerializedList  - pointer to serialized list to terminate

Return Value:

    None.

--*/

{
    ASSERT(SerializedList != NULL);
    ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    ASSERT(SerializedList->ElementCount == 0);

    if (SerializedList->ElementCount != 0) {

        DEBUG_PRINT(SERIALST,
                    ERROR,
                    ("list @ %#x has %d elements, first is %#x\n",
                    SerializedList,
                    SerializedList->ElementCount,
                    SerializedList->List.Flink
                    ));

    } else {

        ASSERT(IsListEmpty(&SerializedList->List));

    }
    DeleteCriticalSection(&SerializedList->Lock);
}


DEBUG_FUNCTION
VOID
InsertAtHeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    None.

--*/

{
    ASSERT(Entry != &SerializedList->List);

    LockSerializedList(SerializedList);
    PAL_TRY {
        if (fCheckEntryOnList) {
            CheckEntryOnList(&SerializedList->List, Entry, FALSE);
        }
        InsertHeadList(&SerializedList->List, Entry);
        ++SerializedList->ElementCount;
    
        ASSERT(SerializedList->ElementCount > 0);
    }
    PAL_FINALLY {
        UnlockSerializedList(SerializedList);
    }
    PAL_ENDTRY
}


DEBUG_FUNCTION
VOID
InsertAtTailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Adds an item to the head of a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to update

    Entry           - thing to update it with

Return Value:

    None.

--*/

{
    ASSERT(Entry != &SerializedList->List);

    LockSerializedList(SerializedList);
    PAL_TRY {
        if (fCheckEntryOnList) {
            CheckEntryOnList(&SerializedList->List, Entry, FALSE);
        }
        InsertTailList(&SerializedList->List, Entry);
        ++SerializedList->ElementCount;
    
        ASSERT(SerializedList->ElementCount > 0);
    }
    PAL_FINALLY {
        UnlockSerializedList(SerializedList);
    }
    PAL_ENDTRY
}


VOID
DEBUG_FUNCTION
RemoveFromSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Removes the entry from a serialized list

Arguments:

    SerializedList  - SERIALIZED_LIST to remove entry from

    Entry           - pointer to entry to remove

Return Value:

    None.

--*/

{
    ASSERT((Entry->Flink != NULL) && (Entry->Blink != NULL));

    LockSerializedList(SerializedList);
    PAL_TRY {
        if (fCheckEntryOnList) {
            CheckEntryOnList(&SerializedList->List, Entry, TRUE);
        }
    
        ASSERT(SerializedList->ElementCount > 0);
    
        RemoveEntryList(Entry);
        --SerializedList->ElementCount;
        Entry->Flink = NULL;
        Entry->Blink = NULL;
    }
    PAL_FINALLY {
        UnlockSerializedList(SerializedList);
    }
    PAL_ENDTRY
}


DEBUG_FUNCTION
BOOL
IsSerializedListEmpty(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Checks if a serialized list contains any elements

Arguments:

    SerializedList  - pointer to list to check

Return Value:

    BOOL

--*/

{
    BOOL empty = FALSE;

    LockSerializedList(SerializedList);

    PAL_TRY {
        ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    
    
        if (IsListEmpty(&SerializedList->List)) {
    
            ASSERT(SerializedList->ElementCount == 0);
    
            empty = TRUE;
        } else {
    
            ASSERT(SerializedList->ElementCount != 0);
    
            empty = FALSE;
        }
    }
    PAL_FINALLY {
        UnlockSerializedList(SerializedList);
    }
    PAL_ENDTRY

    return empty;
}


DEBUG_FUNCTION
PLIST_ENTRY
HeadOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Flink;
}


DEBUG_FUNCTION
PLIST_ENTRY
TailOfSerializedList(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Returns the element at the tail of the list, without taking the lock

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

Return Value:

    PLIST_ENTRY
        pointer to element at tail of list

--*/

{
    ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);

    return SerializedList->List.Blink;
}


DEBUG_FUNCTION
BOOL
CheckEntryOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )

/*++

Routine Description:

    Checks an entry exists (or doesn't exist) on a list

Arguments:

    SerializedList  - pointer to serialized list

    Entry           - pointer to entry

    ExpectedResult  - TRUE if expected on list, else FALSE

Return Value:

    BOOL
        TRUE    - expected result

        FALSE   - unexpected result

--*/

{
    BOOL result = FALSE;

    ASSERT(SerializedList->Signature == SERIALIZED_LIST_SIGNATURE);
    
    LockSerializedList(SerializedList);

    PAL_TRY {
        PAL_TRY {
            result = CheckEntryOnList(&SerializedList->List, Entry, ExpectedResult);
        } PAL_EXCEPT_EX(Label1, EXCEPTION_EXECUTE_HANDLER) {
    
            DEBUG_PRINT(SERIALST,
                        FATAL,
                        ("List @ %#x (%d elements) is bad\n",
                        SerializedList,
                        SerializedList->ElementCount
                        ));
    
            result = FALSE;
        }
        PAL_ENDTRY
    }
    PAL_FINALLY_EX(Label2) {
        UnlockSerializedList(SerializedList);
    }
    PAL_ENDTRY

    return result;
}


PRIVATE
DEBUG_FUNCTION
BOOL
CheckEntryOnList(
    IN PLIST_ENTRY List,
    IN PLIST_ENTRY Entry,
    IN BOOL ExpectedResult
    )
{
    BOOLEAN found = FALSE;
    PLIST_ENTRY p;

    if (!IsListEmpty(List)) {
        for (p = List->Flink; p != List; p = p->Flink) {
            if (p == Entry) {
                found = TRUE;
                break;
            }
        }
    }
    if (found != ExpectedResult) {
        if (ReportCheckEntryOnListErrors) {

            LPCSTR description;

            description = found
                        ? "Entry %#x already on list %#x\n"
                        : "Entry %#x not found on list %#x\n"
                        ;

            DEBUG_PRINT(SERIALST,
                        ERROR,
                        (description,
                        Entry,
                        List
                        ));

            DEBUG_BREAK(SERIALST);

        }
        return FALSE;
    }
    return TRUE;
}

#endif // DBG

//
// functions that are always functions
//


LPVOID
SlDequeueHead(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the head of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry = NULL;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        PAL_TRY {
            if (!IsSerializedListEmpty(SerializedList)) {
                entry = (LPVOID)HeadOfSerializedList(SerializedList);
                RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry);
            } else {
                entry = NULL;
            }
        }
        PAL_FINALLY {
            UnlockSerializedList(SerializedList);
        }
        PAL_ENDTRY
    } else {
        entry = NULL;
    }
    return entry;
}


LPVOID
SlDequeueTail(
    IN LPSERIALIZED_LIST SerializedList
    )

/*++

Routine Description:

    Dequeues the element at the tail of the queue and returns its address or
    NULL if the queue is empty

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST to dequeue from

Return Value:

    LPVOID

--*/

{
    LPVOID entry = NULL;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        PAL_TRY {
            if (!IsSerializedListEmpty(SerializedList)) {
                entry = (LPVOID)TailOfSerializedList(SerializedList);
                RemoveFromSerializedList(SerializedList, (PLIST_ENTRY)entry);
            } else {
                entry = NULL;
            }
        }
        PAL_FINALLY {
            UnlockSerializedList(SerializedList);
        }
        PAL_ENDTRY
    } else {
        entry = NULL;
    }
    return entry;
}


BOOL
IsOnSerializedList(
    IN LPSERIALIZED_LIST SerializedList,
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    Checks if an entry is on a serialized list. Useful to call before
    RemoveFromSerializedList() if multiple threads can remove the element

Arguments:

    SerializedList  - pointer to SERIALIZED_LIST

    Entry           - pointer to element to check

Return Value:

    BOOL
        TRUE    - Entry is on SerializedList

        FALSE   -   "    " not on     "

--*/

{
    BOOL onList = FALSE;
//    LPVOID entry;

    if (!IsSerializedListEmpty(SerializedList)) {
        LockSerializedList(SerializedList);
        PAL_TRY {
            if (!IsSerializedListEmpty(SerializedList)) {
                for (PLIST_ENTRY entry = HeadOfSerializedList(SerializedList);
                    entry != (PLIST_ENTRY)SlSelf(SerializedList);
                    entry = entry->Flink) {
    
                    if (entry == Entry) {
                        onList = TRUE;
                        break;
                    }
                }
            }
        }
        PAL_FINALLY {
            UnlockSerializedList(SerializedList);
        }
        PAL_ENDTRY
    }
    return onList;
}
