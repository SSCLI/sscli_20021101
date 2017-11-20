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

/*  THREADS.CPP:
 *
 */

#include "common.h"

#include "tls.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "excep.h"
#include "comsynchronizable.h"
#include "log.h"
#include "gcscan.h"
#include "gc.h"
#include "mscoree.h"
#include "dbginterface.h"
#include "corprof.h"                // profiling
#include "eeprofinterfaces.h"
#include "eeconfig.h"
#include "perfcounters.h"
#include "corhost.h"
#include "win32threadpool.h"
#include "comstring.h"
#include "jitinterface.h"
#include "threads.inl"

#include "permset.h"
#include "appdomainhelper.h"
#include "comutilnative.h"
#include "fusion.h"



typedef Thread *ThreadPtr;
typedef ThreadPtr (__stdcall *POPTIMIZEDTHREADGETTER)();
typedef AppDomain* (__stdcall *POPTIMIZEDAPPDOMAINGETTER)();
MethodDesc *Thread::s_pReserveSlotMD= NULL;

HANDLE ThreadStore::s_hAbortEvt = NULL;
HANDLE ThreadStore::s_hAbortEvtCache = NULL;


// Here starts the unmanaged portion of the compressed stack code.
// The mission of this code is to provide us with an intermediate
// step between the stackwalk that has to happen when we make an
// async call and the formation of the managed PermissionListSet
// object since the latter is a very expensive operation.
//
// The basic structure of the compressed stack at this point is
// a list of compressed stack entries, where each entry represents
// one piece of "interesting" information found during the stackwalk.
// At this time, the "interesting" bits are appdomain transitions, 
// assembly security, descriptors, appdomain security descriptors,
// frame security descriptors, and other compressed stacks.  Of course,
// if that's all there was to it, there wouldn't be an explanatory
// comment even close to this size before you even started reading
// the code.  Since we need to form a compressed stack whenever an
// async operation is registered, it is a very perf critical piece
// of code.  As such, things get very much more complicated than
// the simple list of objects described above.  The special bonus
// feature is that we need to handle appdomain unloads since the
// list tracks appdomain specific data.  Keep reading to find out
// more.

#if defined(_X86_)
#define GetRedirectHandlerForGCThreadControl() (&Thread::RedirectedHandledJITCaseForGCThreadControl)
#define GetRedirectHandlerForDbgThreadControl() (&Thread::RedirectedHandledJITCaseForDbgThreadControl)
#define GetRedirectHandlerForUserSuspend() (&Thread::RedirectedHandledJITCaseForUserSuspend)
#endif

EXTERN_C Thread* __stdcall GetThreadGeneric(VOID);
EXTERN_C AppDomain* __stdcall GetAppDomainGeneric(VOID);



void*
CompressedStackEntry::operator new( size_t size, CompressedStack* stack )
{
    _ASSERTE( stack != NULL );
    return stack->AllocateEntry( size );
}

CompressedStack*
CompressedStackEntry::Destroy( CompressedStack* owner )
{
    CompressedStack* retval = NULL;

    Cleanup();

    if (this->type_ == ECompressedStack)
    {
        retval = (CompressedStack*)this->ptr_;
    }

    delete this;

    return retval;
}

void
CompressedStackEntry::Cleanup( void )
{
    if ((type_ == EFrameSecurityDescriptor || type_ == ECompressedStackObject) &&
        handleStruct_.handle_ != NULL)
    {
        if (!g_fProcessDetach)
        {
            if (handleStruct_.domainId_ == 0 || SystemDomain::GetAppDomainAtId( handleStruct_.domainId_ ) != NULL)
            {
                // There is a race condition between doing cleanup of CompressedStacks
                // and unloading appdomains.  This is because there can be threads doing
                // cleanup of CompressedStacks where the CompressedStack references an
                // appdomain, but the thread itself is not in that appdomain and is
                // therefore free to continue normally while that appdomain unloads.
                // We try to narrow the race to the smallest possible window through
                // the GetAppDomainAtId call above, but it is still necessary to handle
                // the race here, which we do by catching and ignoring any exceptions.

                PAL_TRY
                {
                    DestroyHandle( handleStruct_.handle_ );
                }
                PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                }
                PAL_ENDTRY
            }

            handleStruct_.handle_ = NULL;
        }
    }
}


CompressedStack::CompressedStack( OBJECTREF orStack )
: delayedCompressedStack_( NULL ),
  compressedStackObject_( GetAppDomain()->CreateHandle( orStack ) ),
  compressedStackObjectAppDomain_( GetAppDomain() ),
  pbObjectBlob_( NULL ),
  cbObjectBlob_( 0 ),
  containsOverridesOrCompressedStackObject_( TRUE ),
  refCount_( 1 ),
  isFullyTrustedDecision_( -1 ),
  depth_( 0 ),
  index_( 0 ),
  offset_( 0 ),
  plsOptimizationOn_( TRUE )
{
#ifdef _DEBUG
    creatingThread_ = GetThread();
#endif
    this->entriesMemoryList_.Append( NULL );

    AddToList();
}


void
CompressedStack::CarryOverSecurityInfo(Thread *pFromThread)
{
    overridesCount_ = pFromThread->GetOverridesCount();
    appDomainStack_ = pFromThread->m_ADStack;
}

// In order to improve locality and decrease the number of
// calls to the global new operator, each compressed stack
// keeps a buffer from which it allocates space for its child
// entries.  Notice the implicit assumption that only one thread
// is ever allocating at a time.  We currently guarantee
// this by not handing out references to the compressed stack
// until the stack walk is completed.

void*
CompressedStack::AllocateEntry( size_t size )
{
    void* buffer = this->entriesMemoryList_.Get( index_ );

    if (buffer == NULL)
    {
        buffer = new BYTE[SIZE_ALLOCATE_BUFFERS];
        this->entriesMemoryList_.Set( this->index_, buffer );
    }

    if (this->offset_ + size > SIZE_ALLOCATE_BUFFERS)
    {
        buffer = new BYTE[SIZE_ALLOCATE_BUFFERS];
        this->entriesMemoryList_.Append( buffer );
        ++this->index_;
        this->offset_ = 0;
    }

    void* retval = (BYTE*)buffer + this->offset_;
    this->offset_ += (DWORD)size;

    return retval;
}

CompressedStackEntry*
CompressedStack::FindMatchingEntry( CompressedStackEntry* entry, CompressedStack* stack )
{
    if (entry == NULL)
        return NULL;

    ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

    _ASSERTE( entry->type_ == EApplicationSecurityDescriptor || entry->type_ == ESharedSecurityDescriptor );

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry->type_ == EApplicationSecurityDescriptor ||
            currentEntry->type_ == ESharedSecurityDescriptor)
        {
            if (currentEntry->ptr_ == entry->ptr_)
                return currentEntry;
        }
    }
    
    return NULL;
}

// Due to the "recursive" nature of a certain class of async
// operations, it is important that we limit the growth of
// our compressed stack objects.  To explain this "recursion",
// think about the async pattern illustrated below:
//
// void Foo()
// {
//     ReadDataFromStream();
//
//     if (StillNeedMoreData())
//         RegisterWaitOnStreamWithFooAsCallback();
//     else
//         WereDoneProcessData();
// }
//
// Notice, that this is just a non-blocking form of:
//
// void Foo()
// {
//     ReadDataFromStream();
//    
//     if (StillNeedMoreData())
//         Foo();
//     else
//         WereDoneProcessData();
// }
//
// This second function will create an runtime call
// stack with repeated entries for Foo().  Similarly,
// the logical call stack for the first function will
// have repeated entries for Foo(), the being that
// all those entries for Foo() will not be on the runtime
// call stack but instead in the compressed stack.  Knowing
// this, and knowing that repeated entries of Foo() make
// no difference to the security state of the stack, it is
// easy to see that we can limit the growth of the stack
// while not altering the semantic by removing duplicate
// entries.  This is somewhat complicated by the possible
// presence of stack modifiers.  Given that, it is important
// that when choosing which of two duplicate entries to
// remove that you remove the one that appears earlier in
// the call chain (since it would be processed second).

static CompressedStackEntry* SafeCreateEntry( CompressedStack* stack, AppDomain* domain, OBJECTHANDLE handle, BOOL fullyTrusted, DWORD domainId, CompressedStackType type )
{
    CompressedStackEntry* entry = NULL;

    PAL_TRY
    {
        entry = new( stack ) CompressedStackEntry( domain->CreateHandle( ObjectFromHandle( handle ) ), fullyTrusted, domainId, type );
    }
    PAL_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        entry = NULL;
    }
    PAL_ENDTRY

    return entry;
}

CompressedStack*
CompressedStack::RemoveDuplicates( CompressedStack* current, CompressedStack* candidate )
{
    // If there are no delayed compressed stack lists, then we can just skip out.
    // We can also skip out if the candidate stack already has a PLS.

    CompressedStack* retval = NULL;
    ArrayList::Iterator iter;
    CompressedStackEntry* entry = NULL;
    CompressedStackEntry* matchingEntry;
    CompressedStack* newStack;
    DWORD domainId = (DWORD) -1;
    AppDomain* domain = NULL;
    CompressedStackEntry* storedObj = NULL;

    CompressedStack::listCriticalSection_.Enter();

    if (current->delayedCompressedStack_ == NULL ||
        candidate->delayedCompressedStack_ == NULL ||
        candidate->compressedStackObject_ != NULL ||
        candidate->pbObjectBlob_)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    
    // Check to make sure they have at least one matching entry.
    // Note: for now I just grab the first appdomain security descriptor
    // and check for a matching one in the other compressed stack.

    iter = current->delayedCompressedStack_->Iterate();

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry->type_ == EApplicationSecurityDescriptor ||
            currentEntry->type_ == ESharedSecurityDescriptor)
        {
            entry = currentEntry;
            break;
        }
    }

    // No match, let's not try any more compression.

    matchingEntry = FindMatchingEntry( entry, candidate );

    if (matchingEntry == NULL)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    // Compression is possible.  Let's get rockin'.

    newStack = new (nothrow) CompressedStack();

    if (newStack == NULL)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    newStack->delayedCompressedStack_ = new ArrayList();
    newStack->containsOverridesOrCompressedStackObject_ = candidate->containsOverridesOrCompressedStackObject_;
    newStack->isFullyTrustedDecision_ = candidate->isFullyTrustedDecision_;

    // This isn't exactly correct, but we'll copy over all the overrides
    // and appdomains from the previous stack into this one eventhough
    // we may make some of those appdomains go away
    newStack->appDomainStack_ = candidate->appDomainStack_;
    newStack->overridesCount_ = candidate->overridesCount_;
    newStack->plsOptimizationOn_ = candidate->plsOptimizationOn_;

    iter = candidate->delayedCompressedStack_->Iterate();

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry == NULL)
            continue;

        switch (currentEntry->type_)
        {
        case EAppDomainTransition:
            {
                CompressedStackEntry* lastEntry;
                if (newStack->delayedCompressedStack_->GetCount() > 0 &&
                    (lastEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( newStack->delayedCompressedStack_->GetCount() - 1 )) != NULL &&
                    lastEntry->type_ == EAppDomainTransition)
                {
                    newStack->delayedCompressedStack_->Set( newStack->delayedCompressedStack_->GetCount() - 1, new( newStack ) CompressedStackEntry( currentEntry->indexStruct_.index_, currentEntry->type_ ) );
                    storedObj = NULL;
                }
                else
                {
                    storedObj = new( newStack ) CompressedStackEntry( currentEntry->indexStruct_.index_, currentEntry->type_ );
                }

                domainId = currentEntry->indexStruct_.index_;
                domain = SystemDomain::GetAppDomainAtId( domainId );
                if (domain == NULL)
                {
                    newStack->Release();
                    candidate->AddRef();
                    retval = candidate;
                    goto Exit;
                }
           
            }
            break;
    
        case ESharedSecurityDescriptor:
            matchingEntry = FindMatchingEntry( currentEntry, current );
            if (matchingEntry == NULL)
            {
                newStack->depth_++;
                storedObj = new( newStack ) CompressedStackEntry( currentEntry->ptr_, currentEntry->type_ );
            }
            else
            {
                storedObj = NULL;
            }
            break;

        case EApplicationSecurityDescriptor:
            matchingEntry = FindMatchingEntry( currentEntry, current );
            if (matchingEntry == NULL)
            {
                newStack->depth_++;
                storedObj = new( newStack ) CompressedStackEntry( currentEntry->ptr_, currentEntry->type_ );
            }
            else
            {
                storedObj = NULL;
            }
            break;

        case ECompressedStack:
            {
                CompressedStack* compressedStack = (CompressedStack*)currentEntry->ptr_;
                compressedStack->AddRef();
                storedObj = new( newStack ) CompressedStackEntry( compressedStack, ECompressedStack );
                newStack->depth_ += compressedStack->GetDepth();
            }
            break;

        case EFrameSecurityDescriptor:
            newStack->depth_++;
            _ASSERTE( domain );
            _ASSERTE( currentEntry->handleStruct_.domainId_ == 0 || domain->GetId() == currentEntry->handleStruct_.domainId_ );
            storedObj = SafeCreateEntry( newStack,
                                         domain,
                                         currentEntry->handleStruct_.handle_ ,
                                         currentEntry->handleStruct_.fullyTrusted_,
                                         currentEntry->handleStruct_.domainId_,
                                         currentEntry->type_ );

            if (storedObj == NULL)
            {
                newStack->Release();
                candidate->AddRef();
                retval = candidate;
                goto Exit;
            }

            break;

        case ECompressedStackObject:
            newStack->containsOverridesOrCompressedStackObject_ = TRUE;
            newStack->depth_++;
            _ASSERTE( domain );
            _ASSERTE( currentEntry->handleStruct_.domainId_ == 0 || domain->GetId() == currentEntry->handleStruct_.domainId_ );
            storedObj = SafeCreateEntry( newStack,
                                         domain,
                                         currentEntry->handleStruct_.handle_,
                                         currentEntry->handleStruct_.fullyTrusted_,
                                         currentEntry->handleStruct_.domainId_,
                                         currentEntry->type_ );

            if (storedObj == NULL)
            {
                newStack->Release();
                candidate->AddRef();
                retval = candidate;
                goto Exit;
            }

            break;
    
        default:
            _ASSERTE( !"Unknown CompressedStackType" );
            break;
        }

        if (storedObj != NULL)
            newStack->delayedCompressedStack_->Append( storedObj );
    }

    // As an additional optimization, if we find that we've removed the
    // number of duplicates to the point where we have just one entry
    // and that entry is an appdomain transition, then we don't need
    // the new stack at all.  Similarly, if we remove all but an appdomain
    // transition and another compressed stack, the new stack we care
    // about is simply the compressed stack in the list so return it instead.

    if (newStack->delayedCompressedStack_->GetCount() <= 2)
    {
        if (newStack->delayedCompressedStack_->GetCount() == 1)
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 0 );

            if (entry->type_ == EAppDomainTransition)
            {
                newStack->Release();
                retval = NULL;;
                goto Exit;
            }
        }
        else if (newStack->delayedCompressedStack_->GetCount() == 2)
        {
            CompressedStackEntry* firstEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 0 );
            CompressedStackEntry* secondEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 1 );

            if (firstEntry->type_ == EAppDomainTransition &&
                secondEntry->type_ == ECompressedStack)
            {
                CompressedStack* previousStack = (CompressedStack*)secondEntry->ptr_;
                previousStack->AddRef();
                newStack->Release();
                retval = previousStack;
                goto Exit;
            }
        }
    }

    retval = newStack;

Exit:
    CompressedStack::listCriticalSection_.Leave();

    return retval;
}

ArrayListStatic CompressedStack::allCompressedStacks_;
CrstStatic      CompressedStack::listCriticalSection_;

DWORD CompressedStack::freeList_[FREE_LIST_SIZE];
DWORD CompressedStack::freeListIndex_ = (DWORD) -1;
DWORD CompressedStack::numUntrackedFreeIndices_ = 0;

#define MAX_LOOP 2

void CompressedStack::Init( void )
{
    CompressedStack::allCompressedStacks_.Init();
    CompressedStack::listCriticalSection_.Init("CompressedStackLock", CrstCompressedStack, TRUE, TRUE);
}

void CompressedStack::Shutdown( void )
{
    CompressedStack::listCriticalSection_.Destroy();
    CompressedStack::allCompressedStacks_.Destroy();
}

// In order to handle appdomain unloads, we keep a list of all
// compressed stacks.  This is complicated by the fact that 
// compressed stacks often get deleted and we create a lot of
// them, so we need to reuse slots in the list.  Therefore,
// we maintain a fixed-size array of free indices within
// the list.  As a backup, we also track the number of indices
// in the list that are free but are not tracked in the array
// of free indices.

void
CompressedStack::AddToList( void )
{
    CompressedStack::listCriticalSection_.Enter();

    // If there is an entry in the free list, simply use it.

    if (CompressedStack::freeListIndex_ != (DWORD) -1)
    {
USE_FREE_LIST:
        this->listIndex_ = CompressedStack::freeList_[CompressedStack::freeListIndex_];
        _ASSERTE( CompressedStack::allCompressedStacks_.Get( this->listIndex_ ) == NULL && "The free list points to an index that is in use" );
        CompressedStack::allCompressedStacks_.Set( this->listIndex_, this );
        this->freeListIndex_--;
    }

    // If there are no free list entries, but there are untracked free indices,
    // let's find them by iterating down the list.

    else if (CompressedStack::numUntrackedFreeIndices_ != 0)
    {
        BOOL done = FALSE;
        DWORD count = 0;

        do
        {
            DWORD index = (DWORD) -1;

            CompressedStack::listCriticalSection_.Leave();

            ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

            while (iter.Next())
            {
                if (iter.GetElement() == NULL)
                {
                    index = iter.GetIndex();
                    break;
                }
            }

            CompressedStack::listCriticalSection_.Enter();

            // There's a possibility that while we weren't holding the lock that
            // someone deleted an entry from the list behind the point of our
            // iteration so we didn't detect it.  We detect this and restart the
            // iteration in the code below, but we also don't want to "starve" a
            // thread by having it search for an open spot forever so we limit
            // the number of times you can go through the loop.

            count++;
            if (index == (DWORD) -1)
            {
                if (CompressedStack::numUntrackedFreeIndices_ == 0 || count >= MAX_LOOP)
                    goto APPEND;
            }
            else if (CompressedStack::allCompressedStacks_.Get( index ) == NULL)
            {
                // If anything has been added to the free list while we didn't
                // hold the lock, then we'll check whether the index we found
                // if one of the untracked ones or not.  If it is an untracked
                // index, we should use it as to not waste the search.  However
                // if it is in the free list we'll just use the last free list
                // entry.  Note that this means that we don't necessarily use
                // the index we just found, but it should be an index that is
                // NULL which is all we really care about.

                if (CompressedStack::freeListIndex_ != (DWORD) -1)
                {
                    for (DWORD i = 0; i <= CompressedStack::freeListIndex_; ++i)
                    {
                        if (CompressedStack::freeList_[i] == index)
                            goto USE_FREE_LIST;
                    }
                }

                CompressedStack::numUntrackedFreeIndices_--;
                this->listIndex_ = index;
                CompressedStack::allCompressedStacks_.Set( index, this );
                done = TRUE;
            }
            else if (CompressedStack::numUntrackedFreeIndices_ == 0 || count >= MAX_LOOP)
            {
                goto APPEND;
            }
        }
        while (!done);
    }

    // Otherwise we place this new entry at that end of the list.

    else
    {
APPEND:
        this->listIndex_ = CompressedStack::allCompressedStacks_.GetCount();
        CompressedStack::allCompressedStacks_.Append( this );
    }

    CompressedStack::listCriticalSection_.Leave();
}


void
CompressedStack::RemoveFromList()
{
    CompressedStack::listCriticalSection_.Enter();

    _ASSERTE( this->listIndex_ < CompressedStack::allCompressedStacks_.GetCount() );
    _ASSERTE( CompressedStack::allCompressedStacks_.Get( this->listIndex_ ) == this && "Index tracking failed for this object" );

    CompressedStack::allCompressedStacks_.Set( this->listIndex_, NULL );

    if (CompressedStack::freeListIndex_ == (DWORD) -1 || CompressedStack::freeListIndex_ < FREE_LIST_SIZE - 1)
    {
        CompressedStack::freeList_[++CompressedStack::freeListIndex_] = this->listIndex_;
    }
    else
    {
        CompressedStack::numUntrackedFreeIndices_++;
    }

    CompressedStack::listCriticalSection_.Leave();
}

BOOL
CompressedStack::SetBlobIfAlive( CompressedStack* stack, BYTE* pbBlob, DWORD cbBlob )
{
    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    DWORD index = (DWORD) -1;

    while (iter.Next())
    {
        if (iter.GetElement() == stack)
        {
            index = iter.GetIndex();
            break;
        }
    }

    if (index == (DWORD) -1)
        return FALSE;

    BOOL retval = FALSE;

    COMPLUS_TRY
    {
        CompressedStack::listCriticalSection_.Enter();

        if (CompressedStack::allCompressedStacks_.Get( index ) == stack)
        {
            stack->pbObjectBlob_ = pbBlob;
            stack->cbObjectBlob_ = cbBlob;
            retval = TRUE;
        }
        else
        {
            retval = FALSE;
        }

        CompressedStack::listCriticalSection_.Leave();
    }
    COMPLUS_CATCH
    {
        _ASSERTE( FALSE && "Don't really expect an exception here" );
        CompressedStack::listCriticalSection_.Leave();
    }
    COMPLUS_END_CATCH

    return retval;

}

BOOL
CompressedStack::Alive( CompressedStack* stack )
{
    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    DWORD index = (DWORD) -1;

    while (iter.Next())
    {
        if (iter.GetElement() == stack)
        {
            index = iter.GetIndex();
            break;
        }
    }

    return (index != (DWORD) -1);
}


void
CompressedStack::AllHandleAppDomainUnload( AppDomain* pDomain, DWORD domainId )
{
    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    while (iter.Next())
    {
        CompressedStack* stack = (CompressedStack*)iter.GetElement();

        if (stack != NULL)
            stack->HandleAppDomainUnload( pDomain, domainId );
    }
}


bool
CompressedStack::HandleAppDomainUnload( AppDomain* pDomain, DWORD domainId )
{
    // Nothing to do if the stack is owned by a different domain, doesn't cache
    // an object or has already serialized the stack. (Though if we've
    // serialized the blob but currently own a cached copy, we must junk that).

    // Note that this function is used for two very different cases and must
    // handle both.  The first is the case where a thread has the appdomain somewhere
    // on it's stack.  The second is the case where the appdomain appears somewhere in
    // this compressed stack.  In both cases we need to make sure that we have
    // generated a permission list set and that it is not in the appdomain that is
    // getting unloaded.  If you are making changes to this function you need to make
    // sure that we detect both cases and don't bail out early in either of the conditions
    // below.

    bool retval = false;

    // Serialize a copy of the stack so that others can use it. We must drop the
    // thread store lock while we're doing this, and the thread might go away,
    // so cache all the info we need to make the call.
    BYTE       *pbBlob;
    DWORD       cbBlob;

    OBJECTREF   orCached = NULL;
    ArrayList::Iterator iter;

    CompressedStack::listCriticalSection_.Enter();

    if (!Alive( this ))
    {
        goto Exit;
    }

    if (pDomain != NULL && (pDomain != this->compressedStackObjectAppDomain_ ||
                            this->compressedStackObject_ == NULL))
    {
        retval = true;
        goto Exit;
    }

    if (this->pbObjectBlob_ != NULL) {
        DestroyHandle(this->compressedStackObject_);
        this->compressedStackObject_ = NULL;
        this->compressedStackObjectAppDomain_ = NULL;
        retval = true;
        goto Exit;
    }

    // Temporary up the ref count to make sure the object doesn't go
    // away beneath us.

    this->AddRef();
    CompressedStack::listCriticalSection_.Leave();

    GCPROTECT_BEGIN(orCached);
    ThreadStore::UnlockThreadStore();

    orCached = this->GetPermissionListSetInternal( pDomain, pDomain, domainId, TRUE );

    AppDomainHelper::MarshalObject(pDomain,
                                   &orCached,
                                   &pbBlob,
                                   &cbBlob);

    GCPROTECT_END();
    ThreadStore::LockThreadStore();

    this->pbObjectBlob_ = pbBlob;
    this->cbObjectBlob_ = cbBlob;

    // GetPermissionListSet will generate an objecthandle
    // that will become invalid shortly so we need to
    // destroy it.

    DestroyHandle( this->compressedStackObject_ );
    this->compressedStackObject_ = NULL;
    this->compressedStackObjectAppDomain_ = NULL;

    // We hold some appdomain specific info in the compressed
    // stack list.  We should clean that up now.

    CompressedStack::listCriticalSection_.Enter();

    iter = this->delayedCompressedStack_->Iterate();

    while (iter.Next())
    {
        ((CompressedStackEntry*)iter.GetElement())->Cleanup();
    }

    CompressedStack::listCriticalSection_.Leave();

    // Now we're done so we can release our extra ref.
    // Note: if this deletes the object the blob we
    // just allocated will get cleaned up by the destructor.

    this->Release();

    goto NoLeaveExit;
Exit:
    CompressedStack::listCriticalSection_.Leave();
NoLeaveExit:
    return retval;
    }


void
CompressedStack::AddEntry( void* obj, CompressedStackType type )
{
    AddEntry( obj, NULL, type );
}

// This is the callback used by the stack walk mechanism to build
// up the entries in the compressed stack.  Most of this is pretty
// straightforward, just adding an entry of the correct type to the
// list.  The complicated portion comes in the handling of an entry
// for a compressed stack.  In that case, we try to remove duplicates
// in order to limit the total size of the compressed stack chain.
// However, if after compression we are still beyond our limit then
// we replace the compressed stack entry with a managed permission
// list set object.

void
CompressedStack::AddEntry( void* obj, AppDomain* domain, CompressedStackType type )
{
    _ASSERTE( (compressedStackObject_ == NULL || ObjectFromHandle( compressedStackObject_ ) == NULL) && "The CompressedStack cannot be altered once a PLS has been generated" );

#ifdef _DEBUG
    // Probably don't need to wrap the assert in an #ifdef, but I want to be consistent
    _ASSERTE( this->creatingThread_ == GetThread() && "Only the creating thread should add entries." );
#endif

    if (this->delayedCompressedStack_ == NULL)
        this->delayedCompressedStack_ = new ArrayList();

    if (domain == NULL)
        domain = GetAppDomain();

    CompressedStackEntry* storedObj = NULL;
    CompressedStack* compressedStack;

    switch (type)
    {
    case EAppDomainTransition:
        storedObj = new( this ) CompressedStackEntry( ((AppDomain*)obj)->GetId(), type );
        break;
    
    case ESharedSecurityDescriptor:
        if (((SharedSecurityDescriptor*)obj)->IsSystem())
            return;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( obj, type );
        break;

    case EApplicationSecurityDescriptor:
        if (((ApplicationSecurityDescriptor*)obj)->GetProperties( CORSEC_DEFAULT_APPDOMAIN ))
            return;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( obj, type );
        break;

    case ECompressedStack:
        {
            compressedStack = (CompressedStack*)obj;
            _ASSERTE( compressedStack );
            compressedStack->AddRef();

            CompressedStack* newCompressedStack;
            if ((compressedStack->GetDepth() - this->GetDepth() <= 5 ||
                 this->GetDepth() - compressedStack->GetDepth() <= 5) &&
                this->GetDepth() < 30)
            {
                newCompressedStack = RemoveDuplicates( this, compressedStack );
            }
            else
            {
                compressedStack->AddRef();
                newCompressedStack = compressedStack;
            }

            if (newCompressedStack != NULL)
            {
                this->overridesCount_ += newCompressedStack->overridesCount_;
            
                if (newCompressedStack->appDomainStack_.GetNumDomains() <= MAX_APPDOMAINS_TRACKED && newCompressedStack->plsOptimizationOn_)
                {
                    DWORD dwIndex;
                    newCompressedStack->appDomainStack_.InitDomainIteration(&dwIndex);
                    DWORD domainId;
                    while ((domainId = newCompressedStack->appDomainStack_.GetNextDomainIndexOnStack(&dwIndex)) != (DWORD) -1)
                    {
                        this->appDomainStack_.PushDomainNoDuplicates( domainId );
                    }
                }
                else
                {
                    this->plsOptimizationOn_ = FALSE;
                }

                if (newCompressedStack->GetDepth() + this->depth_ >= MAX_COMPRESSED_STACK_DEPTH)
                {
                    BOOL fullyTrusted = newCompressedStack->LazyIsFullyTrusted();
                    this->depth_++;
                    storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( newCompressedStack->GetPermissionListSet( domain ) ), fullyTrusted, domain->GetId(), ECompressedStackObject );
                    if (!fullyTrusted)
                        this->containsOverridesOrCompressedStackObject_ = TRUE;
                    newCompressedStack->Release();
                }
                else
                {
                    storedObj = new( this ) CompressedStackEntry( newCompressedStack, ECompressedStack );
                    this->depth_ += newCompressedStack->GetDepth();
                }
            }
            else
            {
                storedObj = NULL;
            }

            compressedStack->Release();
        }
        break;

    case EFrameSecurityDescriptor:
        if ((*(FRAMESECDESCREF*)obj)->HasDenials() || (*(FRAMESECDESCREF*)obj)->HasPermitOnly())
        {
            this->containsOverridesOrCompressedStackObject_ = TRUE;
            this->plsOptimizationOn_ = FALSE;
        }
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( *(OBJECTREF*)obj ), domain->GetId(), type );
        break;

    case ECompressedStackObject:
        this->containsOverridesOrCompressedStackObject_ = TRUE;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( *(OBJECTREF*)obj ), domain->GetId(), type );
        break;
    
    default:
        _ASSERTE( !"Unknown CompressedStackType" );
        break;
    }

    if (storedObj != NULL)
        this->delayedCompressedStack_->Append( storedObj );
}

OBJECTREF
CompressedStack::GetPermissionListSet( AppDomain* domain )
{
    if (domain == NULL)
        domain = GetAppDomain();

    return GetPermissionListSetInternal( domain, NULL, (DWORD) -1, TRUE );
}


OBJECTREF
CompressedStack::GetPermissionListSetInternal( AppDomain* domain, AppDomain* unloadingDomain, DWORD unloadingDomainId, BOOL unwindRecursion )
{
    AppDomain* pCurrentDomain = GetAppDomain();

    if (this->compressedStackObject_ != NULL && ObjectFromHandle( this->compressedStackObject_ ) != NULL)
    {
        if ((domain == NULL && this->compressedStackObjectAppDomain_ != pCurrentDomain) || (domain != NULL && this->compressedStackObjectAppDomain_ != domain))
        {
            Thread* pThread = GetThread();
            ContextTransitionFrame frame;
            OBJECTREF temp = NULL;
            OBJECTREF retval;
            GCPROTECT_BEGIN( temp );
            temp = ObjectFromHandle( this->compressedStackObject_ );
            if (pCurrentDomain != domain)
            {
                pThread->EnterContextRestricted(domain->GetDefaultContext(), &frame, TRUE);
                retval = AppDomainHelper::CrossContextCopyFrom( this->compressedStackObjectAppDomain_, &temp );
                pThread->ReturnToContext(&frame, TRUE);
            }
            else
            {
                retval = AppDomainHelper::CrossContextCopyFrom( this->compressedStackObjectAppDomain_, &temp );
            }
            GCPROTECT_END();
            return retval;
        }
        else
            return ObjectFromHandle( this->compressedStackObject_ );
    }
   
    if (this->compressedStackObject_ == NULL || ObjectFromHandle( this->compressedStackObject_ ) == NULL)
    {
        if (this->compressedStackObject_ != NULL)
        {
            OBJECTHANDLE tempHandle = this->compressedStackObject_;
            this->compressedStackObject_ = NULL;
            this->compressedStackObjectAppDomain_ = NULL;
            DestroyHandle( tempHandle );
        }

        if (this->pbObjectBlob_ != NULL)
        {
            OBJECTREF orCached = NULL;
            GCPROTECT_BEGIN( orCached );
            AppDomainHelper::UnmarshalObject(domain,
                                             this->pbObjectBlob_,
                                             this->cbObjectBlob_,
                                             &orCached);

            this->compressedStackObject_ = domain->CreateHandle(orCached);
            this->compressedStackObjectAppDomain_ = domain;
            GCPROTECT_END();
        }
        else
        {
            OBJECTREF temp = NULL;
            GCPROTECT_BEGIN( temp );

            temp = GeneratePermissionListSet( domain, unloadingDomain, unloadingDomainId, unwindRecursion );
            this->compressedStackObject_ = domain->CreateHandle(temp);
            this->compressedStackObjectAppDomain_ = domain;
            GCPROTECT_END();
        }
    }

    return ObjectFromHandle( this->compressedStackObject_ );

}


AppDomain*
CompressedStack::GetAppDomainFromId( DWORD id, AppDomain* unloadingDomain, DWORD unloadingDomainId )
{
    if (id == unloadingDomainId)
    {
        _ASSERTE( unloadingDomain != NULL );
        return unloadingDomain;
    }

    AppDomain* domain = SystemDomain::GetAppDomainAtId( id );

    _ASSERTE( domain != NULL && "Domain has been already been unloaded" );

    return domain;
}


OBJECTREF
CompressedStack::GeneratePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId, BOOL unwindRecursion )
{
    struct _gc
    {
        OBJECTREF permListSet;
    } gc;
    ZeroMemory( &gc, sizeof( gc ) );

    GCPROTECT_BEGIN( gc );

    // Role up the delayedCompressedStack as necessary.  Things get a little wacky
    // here since we want to avoid the recursion that would be necessary if we
    // need to generate a permission list set the child compresseed stack.  Therefore,
    // we're going to search down the virtual linked list of compressed stacks looking
    // for the first one that already has a permission list set (in live or serialized
    // form) and then travel back up the list generating them in backwards order
    // until we reach the <this> compressed stack again.

    if (this->delayedCompressedStack_ != NULL)
    {
        if (!unwindRecursion)
        {
            gc.permListSet = this->CreatePermissionListSet( targetDomain, unloadingDomain, unloadingDomainId );
        }
        else
        {
            ArrayList compressedStackList;

            compressedStackList.Append( this );

            DWORD index = 0;

            while (index < compressedStackList.GetCount())
            {
                CompressedStack* stack = (CompressedStack*)compressedStackList.Get( index );

                if (stack->delayedCompressedStack_ == NULL)
                    break;

                ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

                while (iter.Next())
                {
                    CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

                    switch (entry->type_)
                    {
                    case ECompressedStack:
                        if (stack->compressedStackObject_ == NULL &&
                            stack->pbObjectBlob_ == NULL &&
                            stack->delayedCompressedStack_ != NULL)
                        {
                            compressedStackList.Append( entry->ptr_ );
                        }
                        break;

                    default:
                        break;
                    }

                }

                index++;
            }

            for (DWORD index = compressedStackList.GetCount() - 1; index > 0; --index)
            {
                ((CompressedStack*)compressedStackList.Get( index ))->GetPermissionListSetInternal( targetDomain, unloadingDomain, unloadingDomainId, FALSE );
            }

            gc.permListSet = ((CompressedStack*)compressedStackList.Get( 0 ))->CreatePermissionListSet( targetDomain, unloadingDomain, unloadingDomainId );
        }   
    }

    if (gc.permListSet == NULL)
    {
        ContextTransitionFrame frame;
        AppDomain* currentDomain = GetAppDomain();
        Thread* pThread = GetThread();

        if (targetDomain != currentDomain)
        {
            pThread->EnterContextRestricted(targetDomain->GetDefaultContext(), &frame, TRUE);
        }

        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

        gc.permListSet = AllocateObject(pMT);

        ARG_SLOT arg[1] = { 
            ObjToArgSlot(gc.permListSet)
        };

        pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);

        if (targetDomain != currentDomain)
        {
            pThread->ReturnToContext(&frame, TRUE);
        }
    }

    GCPROTECT_END();

    return gc.permListSet;
}


OBJECTREF
CompressedStack::CreatePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId )
{
    struct _gc
    {
        OBJECTREF permListSet;
        OBJECTREF grant;
        OBJECTREF denied;
        OBJECTREF frame;
        OBJECTREF compressedStack;
    } gc;
    ZeroMemory( &gc, sizeof( gc ) );

    ApplicationSecurityDescriptor* pAppSecDesc;
    SharedSecurityDescriptor* pSharedSecDesc;
    AssemblySecurityDescriptor* pAsmSecDesc;

    // Do all the work in the current appdomain, marshalling
    // things over as necessary.

    // First, generate a new, empty permission list set

    GCPROTECT_BEGIN( gc );

    MethodDesc *pCompress = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
    MethodDesc *pAppend = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__APPEND_STACK);
    MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
    MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

    gc.permListSet = AllocateObject(pMT);

    ARG_SLOT arg[1] = { 
        ObjToArgSlot(gc.permListSet)
    };

    pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);

    DWORD sourceDomainId = (DWORD) -1;
    AppDomain* sourceDomain = NULL;
    AppDomain* currentDomain = GetAppDomain();
    AppDomain* oldSourceDomain = NULL;
    AppDomain* currentPLSDomain = currentDomain;
    Thread* pThread = GetThread();

    ContextTransitionFrame frame;

    ARG_SLOT appendArgs[2];
    ARG_SLOT compressArgs[5];

    _ASSERTE( this->delayedCompressedStack_ != NULL );

    COMPLUS_TRY
    {
        ArrayList::Iterator iter = this->delayedCompressedStack_->Iterate();

        while (iter.Next())
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

            if (entry == NULL)
                continue;

            switch (entry->type_)
            {
            case EApplicationSecurityDescriptor:
                pAppSecDesc = (ApplicationSecurityDescriptor*)entry->ptr_;
                gc.grant = pAppSecDesc->GetGrantedPermissionSet( &gc.denied );
                // No need to marshal since the grant set will already be in the proper domain.
                compressArgs[0] = ObjToArgSlot(gc.permListSet);
                compressArgs[1] = (ARG_SLOT)FALSE;
                compressArgs[2] = ObjToArgSlot(gc.grant);
                compressArgs[3] = ObjToArgSlot(gc.denied);
                compressArgs[4] = NULL;
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;

            case ESharedSecurityDescriptor:
                pSharedSecDesc = (SharedSecurityDescriptor*)entry->ptr_;
                _ASSERTE( !pSharedSecDesc->IsSystem() );
                pAsmSecDesc = pSharedSecDesc->FindSecDesc( sourceDomain );
                _ASSERTE( pAsmSecDesc );
                gc.grant = pAsmSecDesc->GetGrantedPermissionSet( &gc.denied );
                compressArgs[0] = ObjToArgSlot(gc.permListSet);
                compressArgs[1] = (ARG_SLOT)FALSE;
                compressArgs[2] = ObjToArgSlot(gc.grant);
                compressArgs[3] = ObjToArgSlot(gc.denied);
                compressArgs[4] = NULL;
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;

            case EFrameSecurityDescriptor:
                gc.frame = ObjectFromHandle( entry->handleStruct_.handle_ );
                // The frame security descriptor will already be in the correct context
                // so no need to marshal.
                compressArgs[0] = ObjToArgSlot(gc.permListSet);
                compressArgs[1] = (ARG_SLOT)TRUE;
                compressArgs[2] = NULL;
                compressArgs[3] = NULL;
                compressArgs[4] = ObjToArgSlot(gc.frame);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;
        
            case ECompressedStack:
                gc.compressedStack = ((CompressedStack*)entry->ptr_)->GetPermissionListSet( sourceDomain );
                // GetPermissionListSet will give us the object in the proper
                // appdomain so no need to marshal.
                appendArgs[0] = ObjToArgSlot(gc.permListSet);
                appendArgs[1] = ObjToArgSlot(gc.compressedStack);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                }
                break;
        
            case ECompressedStackObject:
                gc.compressedStack = ObjectFromHandle( entry->handleStruct_.handle_ );
                // The compressed stack object will already be in the sourceDomain so
                // no need to marshal.
                appendArgs[0] = ObjToArgSlot(gc.permListSet);
                appendArgs[1] = ObjToArgSlot(gc.compressedStack);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                }
                break;

            case EAppDomainTransition:
                oldSourceDomain = sourceDomain;
                sourceDomainId = entry->indexStruct_.index_;
                sourceDomain = CompressedStack::GetAppDomainFromId( sourceDomainId, unloadingDomain, unloadingDomainId );
                if (sourceDomain == NULL)
                {
                    _ASSERTE( !"An appdomain on the stack has been unloaded and the compressed stack cannot be formed" );

                    // If we hit this case in non-debug builds, we still need to play it safe, so
                    // we'll push an empty grant set onto the compressed stack.
                    gc.grant = SecurityHelper::CreatePermissionSet(FALSE);
                    if (oldSourceDomain != currentPLSDomain)
                        gc.grant = AppDomainHelper::CrossContextCopyTo( currentPLSDomain, &gc.grant );
                    compressArgs[0] = ObjToArgSlot(gc.permListSet);
                    compressArgs[1] = (ARG_SLOT)FALSE;
                    compressArgs[2] = ObjToArgSlot(gc.grant);
                    compressArgs[3] = NULL;
                    compressArgs[4] = NULL;
                    if (currentPLSDomain != currentDomain)
                    {
                        pThread->EnterContextRestricted(currentPLSDomain->GetDefaultContext(), &frame, TRUE);
                        pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                        pThread->ReturnToContext(&frame, TRUE);
                    }
                    else
                    {
                        pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    }
                }
                else if (sourceDomain != currentPLSDomain)
                {
                    // marshal the permission list set into the sourceDomain of the upcoming object on the stack.
                    pThread->EnterContextRestricted(currentPLSDomain->GetDefaultContext(), &frame, TRUE);
                    gc.permListSet = AppDomainHelper::CrossContextCopyTo( sourceDomain, &gc.permListSet );
                    pThread->ReturnToContext(&frame, TRUE);
                    currentPLSDomain = sourceDomain;
                }

                break;

            default:
                _ASSERTE( !"Unrecognized CompressStackType" );
            }
        }
    }
    COMPLUS_CATCH
    {
        // We're not actually expecting any exceptions to occur during any of this code.

        _ASSERTE( !"Unexpected exception while generating compressed security stack" );

        // If an exception does occur, let's play it safe and push an empty grant set
        // onto the compressed stack.

        gc.grant = SecurityHelper::CreatePermissionSet(FALSE);
        if (sourceDomain != currentDomain)
            gc.grant = AppDomainHelper::CrossContextCopyTo( sourceDomain, &gc.compressedStack );
        compressArgs[0] = ObjToArgSlot(gc.permListSet);
        compressArgs[1] = (ARG_SLOT)FALSE;
        compressArgs[2] = ObjToArgSlot(gc.grant);
        compressArgs[3] = NULL;
        compressArgs[4] = NULL;
        if (sourceDomain != currentDomain)
        {
            pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
            pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
            pThread->ReturnToContext(&frame, TRUE);
        }
        else
        {
            pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
        }
    }
    COMPLUS_END_CATCH

    if (sourceDomain != targetDomain)
    {
        pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
        gc.permListSet = AppDomainHelper::CrossContextCopyTo( targetDomain, &gc.permListSet );
        pThread->ReturnToContext(&frame, TRUE);
    }

    GCPROTECT_END();

    return gc.permListSet;
}

// This piece of code tries to take advantage of
// the quick cache or other resolves that might
// have already taken place.  It is only complicated by
// the need to unwind the recursion.


bool
CompressedStack::LazyIsFullyTrusted()
{
    if (this->isFullyTrustedDecision_ != -1)
        return this->isFullyTrustedDecision_ == 1;

    ArrayList virtualStack;

    virtualStack.Append( this );

    ArrayList::Iterator virtualStackIter = virtualStack.Iterate();

    DWORD currentIndex = 0;

    while (currentIndex < virtualStack.GetCount())
    {
        CompressedStack* stack = (CompressedStack*)virtualStack.Get( currentIndex );
        currentIndex++;

        // If we have already compressed the stack, we cannot make
        // a determination of full trust lazily since we cannot
        // be sure that the delayedCompressedStack list is still
        // valid.

        if (stack->isFullyTrustedDecision_ == 0)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        if (stack->isFullyTrustedDecision_ == 1)
            continue;

        if (stack->compressedStackObject_ != NULL || stack->pbObjectBlob_ != NULL)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        // If we don't have a delayed compressed stack then
        // we cannot make a lazy determination.

        if (stack->delayedCompressedStack_ == NULL)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        // If the stack contains overrides than we just
        // give up.

        if (stack->containsOverridesOrCompressedStackObject_)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        ApplicationSecurityDescriptor* pAppSecDesc;
        SharedSecurityDescriptor* pSharedSecDesc;
        FRAMESECDESCREF objRef;

        ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

        while (iter.Next())
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

            if (entry == NULL)
                continue;

            switch (entry->type_)
            {
            // In the case where we have security descriptors, just ask
            // them if they are fully trusted.

            case EApplicationSecurityDescriptor:
                pAppSecDesc = (ApplicationSecurityDescriptor*)entry->ptr_;
                if (!pAppSecDesc->IsFullyTrusted())
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;

            case ESharedSecurityDescriptor:
                pSharedSecDesc = (SharedSecurityDescriptor*)entry->ptr_;
                if (!pSharedSecDesc->IsFullyTrusted())
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;

            case ECompressedStackObject:
                if (!entry->handleStruct_.fullyTrusted_)
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;
    
            case EFrameSecurityDescriptor:
                objRef = (FRAMESECDESCREF)ObjectFromHandle( entry->handleStruct_.handle_ );
                _ASSERTE( !objRef->HasDenials() &&
                          !objRef->HasPermitOnly() );
                break;
        
            case ECompressedStack:
                virtualStack.Append( entry->ptr_ );
                break;
    
            case EAppDomainTransition:
                break;

            default:
                _ASSERT( !"Unrecognized CompressStackType" );
            }
        }
    }

    this->isFullyTrustedDecision_ = 1;
    return true;
}


LONG
CompressedStack::Release()
{
    LONG firstRetval = InterlockedDecrement( &this->refCount_ );

    if (firstRetval == 0)
    {
        ArrayList virtualStack;
        virtualStack.Append( this );

        DWORD currentIndex = 0;

        while (currentIndex < virtualStack.GetCount())
        {
            CompressedStack* stack = (CompressedStack*)virtualStack.Get( currentIndex );

            LONG retval;
            
            if (currentIndex == 0)
            {
                retval = firstRetval;
            }
            else
            {
                retval = InterlockedDecrement( &stack->refCount_ );
            }

            _ASSERTE( retval >= 0 && "We underflowed the Release for this object.  That's no good!" );

            if (retval == 0)
            {
                stack->RemoveFromList();

                if (stack->compressedStackObject_ != NULL)
                {
                    DestroyHandle( stack->compressedStackObject_ );
                    stack->compressedStackObject_ = NULL;
                    stack->compressedStackObjectAppDomain_ = NULL;
                }

                if (stack->delayedCompressedStack_ != NULL)
                {
                    ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

                    while (iter.Next())
                    {
                        CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

                        CompressedStack* stackToPush = entry->Destroy( stack );

                        if (stackToPush != NULL)
                        {
                            virtualStack.Append( stackToPush );
                        }
                    }

                    delete stack->delayedCompressedStack_;
                }
                FreeM( stack->pbObjectBlob_ );

                delete stack;
            }

            currentIndex++;
        }
    }

    return firstRetval;
}


CompressedStack::~CompressedStack( void )
{
    ArrayList::Iterator iter = this->entriesMemoryList_.Iterate();

    while (iter.Next())
    {
        delete [] (BYTE *) iter.GetElement();
    }

}

// #define     NEW_TLS     1

#ifdef _DEBUG

void  Thread::SetFrame(Frame *pFrame) {
    m_pFrame = pFrame;
    _ASSERTE(PreemptiveGCDisabled());
    if (this == GetThread()) {
        static int ctr = 0;
        if (--ctr == 0)
            --ctr;          // just a statement to put a breakpoint on

        Frame* espVal = (Frame*)GetSP();
        while (pFrame != (Frame*) -1) {

            static Frame* stopFrame = 0;
            if (pFrame == stopFrame)
                _ASSERTE(!"SetFrame frame == stopFrame");

            _ASSERTE (espVal < pFrame &&
                      pFrame->GetFrameType() < Frame::TYPE_COUNT);
            pFrame = pFrame->m_Next;
        }
    }
}
#endif

//************************************************************************
// PRIVATE GLOBALS
//************************************************************************
extern "C" {
DWORD         gThreadTLSIndex = ((DWORD)(-1));            // index ( (-1) == uninitialized )
DWORD         gAppDomainTLSIndex = ((DWORD)(-1));         // index ( (-1) == uninitialized )
}


#define ThreadInited()          (gThreadTLSIndex != ((DWORD)(-1)))

// Every PING_JIT_TIMEOUT ms, check to see if a thread in JITted code has wandered
// into some fully interruptible code (or should have a different hijack to improve
// our chances of snagging it at a safe spot).
#define PING_JIT_TIMEOUT        250

// When we find a thread in a spot that's not safe to abort -- how long to wait before
// we try again.
#define ABORT_POLL_TIMEOUT      10
#ifdef _DEBUG
#define ABORT_FAIL_TIMEOUT      40000
#endif


// For now, give our suspension attempts 40 seconds to succeed before trapping to
// the debugger.   Note that we should probably lower this when the JIT is run in
// preemtive mode, as we really should not be starving the GC for 10's of seconds

#ifdef _DEBUG
unsigned DETECT_DEADLOCK_TIMEOUT=40000;
#endif

#ifdef _DEBUG
    #define MAX_WAIT_OBJECTS 2
#else
    #define MAX_WAIT_OBJECTS MAXIMUM_WAIT_OBJECTS
#endif


#define IS_VALID_WRITE_PTR(addr, size)      _ASSERTE( ! ::IsBadWritePtr(addr, size))
#define IS_VALID_CODE_PTR(addr)             _ASSERTE( ! ::IsBadCodePtr(addr))

// This is the code we pass around for Thread.Interrupt, mainly for assertions
#define APC_Code    0xEECEECEE


// Class static data:
LONG    Thread::m_DebugWillSyncCount = -1;
LONG    Thread::m_DetachCount = 0;
LONG    Thread::m_ActiveDetachCount = 0;

//-------------------------------------------------------------------------
// Public function: SetupThread()
// Creates Thread for current thread if not previously created.
// Returns NULL for failure (usually due to out-of-memory.)
//-------------------------------------------------------------------------
Thread* SetupThread()
{
    _ASSERTE(ThreadInited());
    Thread* pThread;

    if ((pThread = GetThread()) != NULL)
        return pThread;

    // Normally, HasStarted is called from the thread's entrypoint to introduce it to
    // the runtime.  But sometimes that thread is used for DLL_THREAD_ATTACH notifications
    // that call into managed code.  In that case, a call to SetupThread here must
    // find the correct Thread object and install it into TLS.
    if (g_pThreadStore->m_PendingThreadCount != 0)
    {
        DWORD  ourThreadId = ::GetCurrentThreadId();

        ThreadStore::LockThreadStore();
        {
            _ASSERTE(pThread == NULL);
            while ((pThread = g_pThreadStore->GetAllThreadList(pThread, Thread::TS_Unstarted, Thread::TS_Unstarted)) != NULL)
                if (pThread->GetThreadId() == ourThreadId)
                    break;
        }
        ThreadStore::UnlockThreadStore();

        // It's perfectly reasonable to not find this guy.  It's just an unrelated
        // thread spinning up.
        if (pThread)
            return (pThread->HasStarted()
                    ? pThread
                    : NULL);
    }

    // First time we've seen this thread in the runtime:
    pThread = new Thread();
    if (!pThread)
        return NULL;

    if (!pThread->PrepareApartmentAndContext())
        goto fail;

    if (!pThread->InitThread())
        goto fail;

    TlsSetValue(gThreadTLSIndex, (VOID*)pThread);
    TlsSetValue(gAppDomainTLSIndex, (VOID*)pThread->GetDomain());
    
    // reset any unstarted bits on the thread object
    FastInterlockAnd((ULONG *) &pThread->m_State, ~Thread::TS_Unstarted);
    FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_LegalToJoin);
    ThreadStore::AddThread(pThread);

#ifdef DEBUGGING_SUPPORTED
    //
    // If we're debugging, let the debugger know that this
    // thread is up and running now.
    //
    if (CORDebuggerAttached())
    {
        g_pDebugInterface->ThreadCreated(pThread);
    }
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "ThreadCreated() not called due to CORDebuggerAttached() being FALSE for thread 0x%x\n", pThread->GetThreadId()));
    }
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // If a profiler is present, then notify the profiler that a
    // thread has been created.
    if (CORProfilerTrackThreads())
    {
        g_profControlBlock.pProfInterface->ThreadCreated(
            (ThreadID)pThread);

        DWORD osThreadId = ::GetCurrentThreadId();

        g_profControlBlock.pProfInterface->ThreadAssignedToOSThread(
            (ThreadID)pThread, osThreadId);
    }
#endif // PROFILING_SUPPORTED

    _ASSERTE(!pThread->IsBackground()); // doesn't matter, but worth checking
    pThread->SetBackground(TRUE);

    return pThread;

fail:
    delete pThread;
    return NULL;
}

//-------------------------------------------------------------------------
// Public function: SetupThreadPoolThread()
// Just like SetupThread, but also sets a bit to indicate that this is a threadpool thread
Thread* SetupThreadPoolThread(ThreadpoolThreadType typeTPThread)
{
    _ASSERTE(ThreadInited());
    Thread* pThread;

    if (NULL == (pThread = GetThread()))
    {
        pThread = SetupThread();
    }
    if ((pThread->m_State & Thread::TS_ThreadPoolThread) == 0)
    {

        if (typeTPThread == WorkerThread)
            FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_ThreadPoolThread | Thread::TS_TPWorkerThread);
        else
            FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_ThreadPoolThread);

    }
    return pThread;
}

void STDMETHODCALLTYPE CorMarkThreadInThreadPool()
{
    // this is no longer needed after our switch to  
    // the Win32 threadpool.
}


//-------------------------------------------------------------------------
// Public function: SetupUnstartedThread()
// This sets up a Thread object for an exposed System.Thread that
// has not been started yet.  This allows us to properly enumerate all threads
// in the ThreadStore, so we can report on even unstarted threads.  Clearly
// there is no physical thread to match, yet.
//
// When there is, complete the setup with Thread::HasStarted()
//-------------------------------------------------------------------------
Thread* SetupUnstartedThread()
{
    _ASSERTE(ThreadInited());
    Thread* pThread = new Thread();

    if (pThread)
    {
        FastInterlockOr((ULONG *) &pThread->m_State,
                        (Thread::TS_Unstarted | Thread::TS_WeOwn));

        ThreadStore::AddThread(pThread);
    }

    return pThread;
}


//-------------------------------------------------------------------------
// Public function: DestroyThread()
// Destroys the specified Thread object, for a thread which is about to die.
//-------------------------------------------------------------------------
void DestroyThread(Thread *th)
{
    _ASSERTE(g_fEEShutDown || th->m_dwLockCount == 0);
    th->OnThreadTerminate(FALSE);
}


//-------------------------------------------------------------------------
// Public function: DetachThread()
// Marks the thread as needing to be destroyed, but doesn't destroy it yet.
//-------------------------------------------------------------------------
void DetachThread(Thread *th)
{
    _ASSERTE(!th->PreemptiveGCDisabled());
    _ASSERTE(g_fEEShutDown || th->m_dwLockCount == 0);

    FastInterlockIncrement(&Thread::m_DetachCount);
    FastInterlockOr((ULONG*)&th->m_State, (long) Thread::TS_Detached);
    if (!(GetThread()->IsBackground()))
    {
        FastInterlockIncrement(&Thread::m_ActiveDetachCount);
        ThreadStore::CheckForEEShutdown();
    }
}


//-------------------------------------------------------------------------
// Public function: GetThread()
// Returns Thread for current thread. Cannot fail since it's illegal to call this
// without having called SetupThread.
//-------------------------------------------------------------------------
ThreadPtr __stdcall DummyGetThread()
{
    return NULL;
}

ThreadPtr (__stdcall *GetThread)() = DummyGetThread;    // Points to platform-optimized GetThread() function.


//---------------------------------------------------------------------------
// Returns the TLS index for the Thread. This is strictly for the use of
// our ASM stub generators that generate inline code to access the Thread.
// Normally, you should use GetThread().
//---------------------------------------------------------------------------
DWORD GetThreadTLSIndex()
{
    return gThreadTLSIndex;
}

//-------------------------------------------------------------------------
// Public function: GetAppDomain()
// Returns AppDomain for current thread. Cannot fail since it's illegal to call this
// without having called SetupThread.
//-------------------------------------------------------------------------
AppDomain* (__stdcall *GetAppDomain)() = NULL;   // Points to platform-optimized GetThread() function.

//---------------------------------------------------------------------------
// Returns the TLS index for the AppDomain. This is strictly for the use of
// our ASM stub generators that generate inline code to access the AppDomain.
// Normally, you should use GetAppDomain().
//---------------------------------------------------------------------------
DWORD GetAppDomainTLSIndex()
{
    return gAppDomainTLSIndex;
}

//-------------------------------------------------------------------------
// Public function: GetCurrentContext()
// Returns the current context.  InitThreadManager initializes this at startup
// to point to GetCurrentContextGeneric().  See that for explanation.
//-------------------------------------------------------------------------
Context* (*GetCurrentContext)() = NULL;


//---------------------------------------------------------------------------
// Portable GetCurrentContext() function: always used for now.  But may be
// replaced later if we move the Context directly into the TLS (for speed &
// COM Interop reasons).
//---------------------------------------------------------------------------
static Context* GetCurrentContextGeneric()
{
    return GetThread()->GetContext();
}

#ifdef _DEBUG
DWORD_PTR Thread::OBJREF_HASH = OBJREF_TABSIZE;
#endif


#if !defined(_X86_)
// The x86 equivalents are in i386\asmhelpers.asm
Thread* GetThreadGeneric()
{
    _ASSERTE(ThreadInited());

    return (Thread*)TlsGetValue(gThreadTLSIndex);
}

AppDomain* GetAppDomainGeneric()
{
    _ASSERTE(ThreadInited());

    return (AppDomain*)TlsGetValue(gAppDomainTLSIndex);
}
#endif //!defined(_X86_)

//---------------------------------------------------------------------------
// One-time initialization. Called during Dll initialization. So
// be careful what you do in here!
//---------------------------------------------------------------------------
BOOL InitThreadManager()
{
    _ASSERTE(gThreadTLSIndex == ((DWORD)(-1)));
    _ASSERTE(g_TrapReturningThreads == 0);

#ifdef _DEBUG
    // Randomize OBJREF_HASH to handle hash collision.
    Thread::OBJREF_HASH = OBJREF_TABSIZE - (DbgGetEXETimeStamp()%10);
#endif
    
    DWORD idx = TlsAlloc();
    if (idx == ((DWORD)(-1)))
        goto fail;

    gThreadTLSIndex = idx;

    GetThread = (POPTIMIZEDTHREADGETTER)MakeOptimizedTlsGetter(gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThreadGeneric);
    if (!GetThread)
        goto fail;

    idx = TlsAlloc();
    if (idx == ((DWORD)(-1)))
        goto fail;

    gAppDomainTLSIndex = idx;

    GetAppDomain = (POPTIMIZEDAPPDOMAINGETTER)MakeOptimizedTlsGetter(gAppDomainTLSIndex, (POPTIMIZEDTLSGETTER)GetAppDomainGeneric);
    if (!GetAppDomain)
        goto fail;

    // For now, access GetCurrentContext() as a function pointer even though it is
    // just pulled out of the Thread object.  That's because it may move directly
    // into the TLS later, for speed and COM interoperability reasons.  This avoids
    // having to change all the clients, if it does.
    GetCurrentContext = GetCurrentContextGeneric;


    return ThreadStore::InitThreadStore();

fail:
    if (GetAppDomain != NULL)
    {
        FreeOptimizedTlsGetter( gAppDomainTLSIndex, (POPTIMIZEDTLSGETTER)GetAppDomain );
        GetAppDomain = NULL;
    }

    if (gAppDomainTLSIndex != (DWORD) -1)
    {
        TlsFree(gAppDomainTLSIndex);
        gAppDomainTLSIndex = (DWORD) -1;
    }

    if (GetThread != DummyGetThread)
    {
        FreeOptimizedTlsGetter( gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThread );
        GetThread = DummyGetThread;
    }

    if (gThreadTLSIndex != (DWORD) -1)
    {
        TlsFree(gThreadTLSIndex);
        gThreadTLSIndex = (DWORD) -1;
    }

    _ASSERTE(FALSE);
    return FALSE;
}


//---------------------------------------------------------------------------
// One-time cleanup. Called during Dll cleanup. So
// be careful what you do in here!
//---------------------------------------------------------------------------


//************************************************************************
// Thread members
//************************************************************************


#if defined(_DEBUG) && defined(TRACK_SYNC)

// One outstanding synchronization held by this thread:
struct Dbg_TrackSyncEntry
{
    int          m_caller;
    AwareLock   *m_pAwareLock;

    BOOL        Equiv      (int caller, void *pAwareLock)
    {
        return (m_caller == caller) && (m_pAwareLock == pAwareLock);
    }

    BOOL        Equiv      (void *pAwareLock)
    {
        return (m_pAwareLock == pAwareLock);
    }
};

// Each thread has a stack that tracks all enter and leave requests
struct Dbg_TrackSyncStack : public Dbg_TrackSync
{
    enum
    {
        MAX_TRACK_SYNC  = 20,       // adjust stack depth as necessary
    };

    void    EnterSync  (int caller, void *pAwareLock);
    void    LeaveSync  (int caller, void *pAwareLock);

    Dbg_TrackSyncEntry  m_Stack [MAX_TRACK_SYNC];
    int                 m_StackPointer;
    BOOL                m_Active;

    Dbg_TrackSyncStack() : m_StackPointer(0),
                           m_Active(TRUE)
    {
    }
};

// A pain to do all this from ASM, but watch out for trashed registers
void EnterSyncHelper    (int caller, void *pAwareLock)
{
    GetThread()->m_pTrackSync->EnterSync(caller, pAwareLock);
}
void LeaveSyncHelper    (int caller, void *pAwareLock)
{
    GetThread()->m_pTrackSync->LeaveSync(caller, pAwareLock);
}

void Dbg_TrackSyncStack::EnterSync(int caller, void *pAwareLock)
{
    if (m_Active)
    {
        if (m_StackPointer >= MAX_TRACK_SYNC)
        {
            _ASSERTE(!"Overflowed synchronization stack checking.  Disabling");
            m_Active = FALSE;
            return;
        }
    }
    m_Stack[m_StackPointer].m_caller = caller;
    m_Stack[m_StackPointer].m_pAwareLock = (AwareLock *) pAwareLock;

    m_StackPointer++;
}

void Dbg_TrackSyncStack::LeaveSync(int caller, void *pAwareLock)
{
    if (m_Active)
    {
        if (m_StackPointer == 0)
            _ASSERTE(!"Underflow in leaving synchronization");
        else
        if (m_Stack[m_StackPointer - 1].Equiv(pAwareLock))
        {
            m_StackPointer--;
        }
        else
        {
            for (int i=m_StackPointer - 2; i>=0; i--)
            {
                if (m_Stack[i].Equiv(pAwareLock))
                {
                    _ASSERTE(!"Locks are released out of order.  This might be okay...");
                    memcpy(&m_Stack[i], &m_Stack[i+1],
                           sizeof(m_Stack[0]) * (m_StackPointer - i - 1));

                    return;
                }
            }
            _ASSERTE(!"Trying to release a synchronization lock which isn't held");
        }
    }
}

#endif  // TRACK_SYNC


//--------------------------------------------------------------------
// Thread construction
//--------------------------------------------------------------------
Thread::Thread()
{
    m_pFrame                = FRAME_TOP;
    m_pUnloadBoundaryFrame  = NULL;

    m_fPreemptiveGCDisabled = 0;
#ifdef _DEBUG
    m_ulForbidTypeLoad      = 0;
    m_ulGCForbidCount       = 0;
    m_GCOnTransitionsOK             = TRUE;
    m_ulEnablePreemptiveGCCount       = 0;
    m_ulReadyForSuspensionCount       = 0;
    m_ComPlusCatchDepth = (LPVOID) -1;
#endif

    m_dwLockCount = 0;
    
    // Initialize lock state
    m_pHead = &m_embeddedEntry;
    m_embeddedEntry.pNext = m_pHead;
    m_embeddedEntry.pPrev = m_pHead;
    m_embeddedEntry.dwLLockID = 0;
    m_embeddedEntry.dwULockID = 0;
    m_embeddedEntry.wReaderLevel = 0;

    m_alloc_context.init();

    m_UserInterrupt = 0;
    m_SafeEvent = m_SuspendEvent = INVALID_HANDLE_VALUE;
    m_EventWait = INVALID_HANDLE_VALUE;
    m_WaitEventLink.m_Next = NULL;
    m_WaitEventLink.m_LinkSB.m_pNext = NULL;
    m_ThreadHandle = INVALID_HANDLE_VALUE;
    m_ThreadHandleForClose = INVALID_HANDLE_VALUE;
    m_ThreadId = 0;
    m_Priority = INVALID_THREAD_PRIORITY;
    m_ExternalRefCount = 1;
    m_State = TS_Unstarted;
    m_StateNC = TSNC_Unknown;

    // It can't be a LongWeakHandle because we zero stuff out of the exposed
    // object as it is finalized.  At that point, calls to GetCurrentThread()
    // had better get a new one,!
    m_ExposedObject = CreateGlobalShortWeakHandle(NULL);
    m_StrongHndToExposedObject = CreateGlobalStrongHandle(NULL);

    m_LastThrownObjectHandle = NULL;

    m_debuggerWord1 = NULL; // Zeros out both filter CONTEXT* and the extra state flags.
    m_debuggerCantStop = 0;

#ifdef _DEBUG
    m_pCleanedStackBase = NULL;

#endif

#if defined(_DEBUG) && defined(TRACK_SYNC)
    m_pTrackSync = new Dbg_TrackSyncStack;
#endif  // TRACK_SYNC

    m_PreventAsync = 0;
    m_pDomain = NULL;
    m_Context = NULL;
    m_TraceCallCount = 0;
    m_ThrewControlForThread = 0;
    m_OSContext = NULL;
    m_ThreadTasks = (ThreadTasks)0;

    Thread *pThread = GetThread();
    _ASSERTE(SystemDomain::System()->DefaultDomain()->GetDefaultContext());
    InitContext();
    _ASSERTE(m_Context);
    if (pThread) 
    {
        _ASSERTE(pThread->GetDomain() && pThread->GetDomain()->GetDefaultContext());
        // Start off the new thread in the default context of
        // the creating thread's appDomain. This could be changed by SetDelegate
        SetKickOffDomain(pThread->GetDomain());
    } else
        SetKickOffDomain(SystemDomain::System()->DefaultDomain());

    // The state and the tasks must be 32-bit aligned for atomicity to be guaranteed.
    _ASSERTE((((size_t) &m_State) & 3) == 0);
    _ASSERTE((((size_t) &m_ThreadTasks) & 3) == 0);

    m_dNumAccessOverrides = 0;
    // Track perf counter for the logical thread object.
    COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsLogical++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsLogical++);

#ifdef STRESS_HEAP
        // ON all callbacks, call the trap code, which we now have
        // wired to cause a GC.  THus we will do a GC on all Transition Frame Transitions (and more).  
   if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION)
        m_State = (ThreadState) (m_State | TS_GCOnTransitions); 
#endif

    m_pSharedStaticData = NULL;
    m_pUnsharedStaticData = NULL;
    m_pStaticDataList = NULL;
    m_pDLSHash = NULL;
    m_pCtx = NULL;

    m_fSecurityStackwalk = FALSE;
    m_compressedStack = NULL;
    m_fPLSOptimizationState = TRUE;

    m_pFusionAssembly = NULL;
    m_pAssembly = NULL;
    m_pModuleToken = mdFileNil;

#ifdef STRESS_THREAD
    m_stressThreadCount = -1;
#endif
}


//--------------------------------------------------------------------
// Failable initialization occurs here.
//--------------------------------------------------------------------
BOOL Thread::InitThread()
{
    HANDLE  hDup = INVALID_HANDLE_VALUE;
    BOOL    ret = TRUE;

    if ((m_State & TS_WeOwn) == 0)
    {
        _ASSERTE(GetThreadHandle() == INVALID_HANDLE_VALUE);

        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cRecognizedThreads++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cRecognizedThreads++);

        // For WinCE, all clients have the same handle for a thread.  Duplication is
        // not possible.  We make sure we never close this handle unless we created
        // the thread (TS_WeOwn).
        //
        // For Win32, each client has its own handle.  This is achieved by duplicating
        // the pseudo-handle from ::GetCurrentThread().  Unlike WinCE, this service
        // returns a pseudo-handle which is only useful for duplication.  In this case
        // each client is responsible for closing its own (duplicated) handle.
        //
        // We don't bother duplicating if WeOwn, because we created the handle in the
        // first place.
        // Thread is created when or after the physical thread started running
        HANDLE curProcess = ::GetCurrentProcess();

        
        if (::DuplicateHandle(curProcess, ::GetCurrentThread(), curProcess, &hDup,
                              0 /*ignored*/, FALSE /*inherit*/, DUPLICATE_SAME_ACCESS))
        {
            _ASSERTE(hDup != INVALID_HANDLE_VALUE);
    
            SetThreadHandle(hDup);
        }
        else
        {
            ret = FALSE;
            goto leav;
        }

        if (!AllocHandles())
        {
            ret = FALSE;
            goto leav;
        }
    }
    else
    {
        _ASSERTE(GetThreadHandle() != INVALID_HANDLE_VALUE);
        
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical++);
    }

    m_pExceptionList = PAL_GetBottommostRegistrationPtr();


leav:
    
    if (!ret)
    {
        if (hDup != INVALID_HANDLE_VALUE)
            ::CloseHandle(hDup);
    }
    return ret;
}


// Allocate all the handles.  When we are kicking of a new thread, we can call
// here before the thread starts running.
BOOL Thread::AllocHandles()
{
    _ASSERTE(m_SafeEvent == INVALID_HANDLE_VALUE);
    _ASSERTE(m_SuspendEvent == INVALID_HANDLE_VALUE);
    _ASSERTE(m_EventWait == INVALID_HANDLE_VALUE);

    // create a manual reset event for getting the thread to a safe point
    m_SafeEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_SafeEvent)
    {
        m_SuspendEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_SuspendEvent)
        {
            m_EventWait = ::WszCreateEvent(NULL, TRUE/*ManualReset*/,
                                           TRUE/*Signalled*/, NULL);
            if (m_EventWait)
            {
                return TRUE;
            }
            m_EventWait = INVALID_HANDLE_VALUE;

            ::CloseHandle(m_SuspendEvent);
            m_SuspendEvent = INVALID_HANDLE_VALUE;
        }
        ::CloseHandle(m_SafeEvent);
        m_SafeEvent = INVALID_HANDLE_VALUE;
    }

    // I should like to do COMPlusThrowWin32(), but the thread never got set up
    // correctly.
    return FALSE;
}

void Thread::SetInheritedSecurityStack(OBJECTREF orStack)
{
    THROWSCOMPLUSEXCEPTION();

    if (orStack == NULL)
    {
        // The only synchronization we use here is making
        // sure that only this thread alters itself.

        _ASSERTE(GetThread() == this || (this->GetSnapshotState() & TS_Unstarted));
        this->m_compressedStack->Release();
        this->m_compressedStack = NULL;
        return;
    }

    _ASSERTE( this->m_compressedStack == NULL);

    this->m_compressedStack = new CompressedStack( orStack );

    // If an appdomain unloaded has started for the current appdomain (but we
    // haven't got to the point where threads are refused admittance), we're
    // racing with cleanup code that will try and serialize compressed stacks
    // so they can be used in other appdomains if the thread survives the
    // unload. If it looks like this is the case for our thread, we could use
    // some elaborate synchronization to ensure that either the cleanup code or
    // this code serializes the data so it isn't missed in the race. But, since
    // this is a rare edge condition and since we're only just starting the new
    // thread (from the context of the appdomain being unloaded), we might as
    // well just throw an appdomain unloaded exception (if the creating thread
    // had been just a little slower, this would have been the result anyway).
    if (GetAppDomain()->IsUnloading())
    {
        this->m_compressedStack->Release();
        this->m_compressedStack = NULL;
        COMPlusThrow(kAppDomainUnloadedException);
    }
}

void Thread::SetDelayedInheritedSecurityStack(CompressedStack* pStack)
{
    THROWSCOMPLUSEXCEPTION();

    if (pStack == NULL)
    {
        // The only synchronization we use here is making
        // sure that only this thread alters itself.

        _ASSERTE(GetThread() == this || (this->GetSnapshotState() & TS_Unstarted));
        if (this->m_compressedStack != NULL)
        {
            this->m_compressedStack->Release();
            this->m_compressedStack = NULL;
        }
        return;
    }

    _ASSERTE(this->m_compressedStack == NULL);

    if (pStack != NULL)
    {
        pStack->AddRef();
        this->m_compressedStack = pStack;

        // If an appdomain unloaded has started for the current appdomain (but we
        // haven't got to the point where threads are refused admittance), we're
        // racing with cleanup code that will try and serialize compressed stacks
        // so they can be used in other appdomains if the thread survives the
        // unload. If it looks like this is the case for our thread, we could use
        // some elaborate synchronization to ensure that either the cleanup code or
        // this code serializes the data so it isn't missed in the race. But, since
        // this is a rare edge condition and since we're only just starting the new
        // thread (from the context of the appdomain being unloaded), we might as
        // well just throw an appdomain unloaded exception (if the creating thread
        // had been just a little slower, this would have been the result anyway).
        if (GetAppDomain()->IsUnloading())
        {
            this->m_compressedStack->Release();
            this->m_compressedStack = NULL;
            COMPlusThrow(kAppDomainUnloadedException);
        }
    }
}



OBJECTREF Thread::GetInheritedSecurityStack()
{
    // The only synchronization we have here is that this method is only called
    // by the thread on itself.
    _ASSERTE(GetThread() == this);

    if (this->m_compressedStack != NULL)
        return this->m_compressedStack->GetPermissionListSet();
    else
        return NULL;
}

CompressedStack* Thread::GetDelayedInheritedSecurityStack()
{
    // The only synchronization we have here is that this method is only called
    // by the thread on itself.
    _ASSERTE(GetThread() == this);

    return this->m_compressedStack;
    }


// Called when an appdomain is unloading to remove any appdomain specific
// resources from the inherited security stack.
// This routine is called with the thread store lock held, and may drop and
// re-acquire the lock as part of it's processing. The boolean returned
// indicates whether the caller may resume enumerating the thread list where
// they left off (true) or must restart the scan from the beginning (false).
bool Thread::CleanupInheritedSecurityStack(AppDomain *pDomain, DWORD domainId)
{
    if (this->m_compressedStack != NULL)
        return this->m_compressedStack->HandleAppDomainUnload( pDomain, domainId );
    else
        return true;
    }

//--------------------------------------------------------------------
// This is the alternate path to SetupThread/InitThread.  If we created
// an unstarted thread, we have SetupUnstartedThread/HasStarted.
//--------------------------------------------------------------------
BOOL Thread::HasStarted()
{
    _ASSERTE(!m_fPreemptiveGCDisabled);     // can't use PreemptiveGCDisabled() here

    // This is cheating a little.  There is a pathway here from SetupThread, but only
    // via IJW SystemDomain::RunDllMain.  Normally SetupThread returns a thread in
    // preemptive mode, ready for a transition.  But in the IJW case, it can return a
    // cooperative mode thread.  RunDllMain handles this "surprise" correctly.
    m_fPreemptiveGCDisabled = TRUE;

    // Normally, HasStarted is called from the thread's entrypoint to introduce it to
    // the runtime.  But sometimes that thread is used for DLL_THREAD_ATTACH notifications
    // that call into managed code.  In that case, the second HasStarted call is
    // redundant and should be ignored.
    if (GetThread() == this)
        return TRUE;

    _ASSERTE(GetThread() == 0);
    _ASSERTE(GetThreadHandle() != INVALID_HANDLE_VALUE);

    BOOL    res;
    res = PrepareApartmentAndContext();
    if (res)
        res = InitThread();

    // Interesting to debug.  Presumably the system has run out of resources.
    _ASSERTE(res);

    ThreadStore::TransferStartedThread(this);

#ifdef DEBUGGING_SUPPORTED
    //
    // If we're debugging, let the debugger know that this
    // thread is up and running now.
    //
    if (CORDebuggerAttached())
    {
        g_pDebugInterface->ThreadCreated(this);
    }    
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "ThreadCreated() not called due to CORDebuggerAttached() being FALSE for thread 0x%x\n", GetThreadId()));
    }
    
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // If a profiler is running, let them know about the new thread.
    if (CORProfilerTrackThreads())
    {
            g_profControlBlock.pProfInterface->ThreadCreated((ThreadID) this);

            DWORD osThreadId = ::GetCurrentThreadId();

            g_profControlBlock.pProfInterface->ThreadAssignedToOSThread(
                (ThreadID) this, osThreadId);
    }
#endif // PROFILING_SUPPORTED

    if (res)
    {
        // we've been told to stop, before we get properly started
        if (m_State & TS_StopRequested)
            res = FALSE;
        else
        {
            // Is there a pending user suspension?
            if (m_State & TS_SuspendUnstarted)
            {
                BOOL    doSuspend = FALSE;

                ThreadStore::LockThreadStore();

                // Perhaps we got resumed before it took effect?
                if (m_State & TS_SuspendUnstarted)
                {
                    FastInterlockAnd((ULONG *) &m_State, ~TS_SuspendUnstarted);
                    ClearSuspendEvent();
                    MarkForSuspension(TS_UserSuspendPending);
                    doSuspend = TRUE;
                }

                ThreadStore::UnlockThreadStore();
                if (doSuspend)
                {
                    EnablePreemptiveGC();
                    WaitSuspendEvent();
                    DisablePreemptiveGC();
                }
            }
        }
    }
    
    return res;
}


// We don't want ::CreateThread() calls scattered throughout the source.  So gather
// them all here.
HANDLE Thread::CreateNewThread(DWORD stackSize, ThreadStartFunction start,
                               void *args, DWORD *pThreadId)
{
    DWORD   ourId;
    HANDLE  h = ::CreateThread(NULL     /*=SECURITY_ATTRIBUTES*/,
                               stackSize,
                               start,
                               args,
                               CREATE_SUSPENDED,
                               (pThreadId ? pThreadId : &ourId));
    if (h == NULL)
        goto fail;

    _ASSERTE(!m_fPreemptiveGCDisabled);     // leave in preemptive until HasStarted.

    // Make sure we have all our handles, in case someone tries to suspend us
    // as we are starting up.
    if (!AllocHandles())
    {
        ::CloseHandle(h);
fail:   // OS is out of handles?
        return INVALID_HANDLE_VALUE;
    }

    SetThreadHandle(h);

    FastInterlockIncrement(&g_pThreadStore->m_PendingThreadCount);

    return h;
}


// General comments on thread destruction.
//
// The C++ Thread object can survive beyond the time when the Win32 thread has died.
// This is important if an exposed object has been created for this thread.  The
// exposed object will survive until it is GC'ed.
//
// A client like an exposed object can place an external reference count on that
// object.  We also place a reference count on it when we construct it, and we lose
// that count when the thread finishes doing useful work (OnThreadTerminate).
//
// One way OnThreadTerminate() is called is when the thread finishes doing useful
// work.  This case always happens on the correct thread.
//
// The other way OnThreadTerminate()  is called is during product shutdown.  We do
// a "best effort" to eliminate all threads except the Main thread before shutdown
// happens.  But there may be some background threads or external threads still
// running.
//
// When the final reference count disappears, we destruct.  Until then, the thread
// remains in the ThreadStore, but is marked as "Dead".

int Thread::IncExternalCount()
{
    // !!! The caller of IncExternalCount should not hold the ThreadStoreLock.
    Thread *pCurThread = GetThread();
    _ASSERTE (pCurThread == NULL || g_fProcessDetach
              || !ThreadStore::HoldingThreadStore(pCurThread));

    // Must synchronize count and exposed object handle manipulation. We use the
    // thread lock for this, which implies that we must be in pre-emptive mode
    // to begin with and avoid any activity that would invoke a GC (this
    // acquires the thread store lock).
    BOOL ToggleGC = pCurThread->PreemptiveGCDisabled();
    if (ToggleGC)
        pCurThread->EnablePreemptiveGC();

    int RetVal;
    ThreadStore::LockThreadStore();

    _ASSERTE(m_ExternalRefCount > 0);
    m_ExternalRefCount++;
    RetVal = m_ExternalRefCount;

    // If we have an exposed object and the refcount is greater than one
    // we must make sure to keep a strong handle to the exposed object
    // so that we keep it alive even if nobody has a reference to it.
    if (((*((void**)m_ExposedObject)) != NULL) && (m_ExternalRefCount > 1))
    {
        // The exposed object exists and needs a strong handle so check
        // to see if it has one.
        if ((*((void**)m_StrongHndToExposedObject)) == NULL)
        {
            // Switch to cooperative mode before using OBJECTREF's.
            pCurThread->DisablePreemptiveGC();

            // Store the object in the strong handle.
            StoreObjectInHandle(m_StrongHndToExposedObject, ObjectFromHandle(m_ExposedObject));

            ThreadStore::UnlockThreadStore();

            // Switch back to the initial GC mode.
            if (!ToggleGC)
                pCurThread->EnablePreemptiveGC();

            return RetVal;
        }

    }

    ThreadStore::UnlockThreadStore();

    // Switch back to the initial GC mode.
    if (ToggleGC)
        pCurThread->DisablePreemptiveGC();

    return RetVal;
}

void Thread::DecExternalCount(BOOL holdingLock)
{
    // Note that it's possible to get here with a NULL current thread (during
    // shutdown of the thread manager).
    Thread *pCurThread = GetThread();
    _ASSERTE (pCurThread == NULL || g_fProcessDetach || g_fFinalizerRunOnShutDown
              || (!holdingLock && !ThreadStore::HoldingThreadStore(pCurThread))
              || (holdingLock && ThreadStore::HoldingThreadStore(pCurThread)));
    
    BOOL ToggleGC = FALSE;
    BOOL SelfDelete = FALSE;

    // Must synchronize count and exposed object handle manipulation. We use the
    // thread lock for this, which implies that we must be in pre-emptive mode
    // to begin with and avoid any activity that would invoke a GC (this
    // acquires the thread store lock).
    if (pCurThread)
    {
        ToggleGC = pCurThread->PreemptiveGCDisabled();
        if (ToggleGC)
            pCurThread->EnablePreemptiveGC();
    }
    if (!holdingLock)
    {
        LOG((LF_SYNC, INFO3, "DecExternal obtain lock\n"));
        ThreadStore::LockThreadStore();
    }
    
    _ASSERTE(m_ExternalRefCount >= 1);
    _ASSERTE(!holdingLock ||
             g_pThreadStore->m_Crst.GetEnterCount() > 0 ||
             g_fProcessDetach);

    m_ExternalRefCount--;

    if (m_ExternalRefCount == 0)
    {
        HANDLE h = m_ThreadHandle;
        if (h == INVALID_HANDLE_VALUE)
        {
            h = m_ThreadHandleForClose;
            m_ThreadHandleForClose = INVALID_HANDLE_VALUE;
        }
        // Can not assert like this.  We have already removed the Unstarted bit.
        //_ASSERTE (IsUnstarted() || h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(h);
        }

        // Switch back to cooperative mode to manipulate the thread.
        if (pCurThread)
            pCurThread->DisablePreemptiveGC();

        // during process detach the thread might still be in the thread list
        // if it hasn't seen its DLL_THREAD_DETACH yet.  Use the following
        // tweak to decide if the thread has terminated yet.
        if (GetThreadHandle() == INVALID_HANDLE_VALUE)
        {
            SelfDelete = this == pCurThread;
            m_handlerInfo.FreeStackTrace();
            if (SelfDelete) {
                TlsSetValue(gThreadTLSIndex, (VOID*)NULL);
            }
            delete this;
        }

        if (!holdingLock)
            ThreadStore::UnlockThreadStore();

        // It only makes sense to restore the GC mode if we didn't just destroy
        // our own thread object.
        if (pCurThread && !SelfDelete && !ToggleGC)
            pCurThread->EnablePreemptiveGC();

        return;
    }
    else if (pCurThread == NULL)
    {
        // We're in shutdown, too late to be worrying about having a strong
        // handle to the exposed thread object, we've already performed our
        // final GC.
        return;
    }
    else
    {
        // Check to see if the external ref count reaches exactly one. If this
        // is the case and we have an exposed object then it is that exposed object
        // that is holding a reference to us. To make sure that we are not the
        // ones keeping the exposed object alive we need to remove the strong 
        // reference we have to it.
        if ((m_ExternalRefCount == 1) && ((*((void**)m_StrongHndToExposedObject)) != NULL))
        {
            // Switch back to cooperative mode to manipulate the object.

            // Clear the handle and leave the lock.
            // We do not have to to DisablePreemptiveGC here, because
            // we just want to put NULL into a handle.
            StoreObjectInHandle(m_StrongHndToExposedObject, NULL);          

            if (!holdingLock)
                ThreadStore::UnlockThreadStore();

            // Switch back to the initial GC mode.
            if (ToggleGC)
                pCurThread->DisablePreemptiveGC();

            return;
        }
    }

    if (!holdingLock)
        ThreadStore::UnlockThreadStore();

    // Switch back to the initial GC mode.
    if (ToggleGC)
        pCurThread->DisablePreemptiveGC();
}


//--------------------------------------------------------------------
// Destruction. This occurs after the associated native thread
// has died.
//--------------------------------------------------------------------
Thread::~Thread()
{
    _ASSERTE(m_ThrewControlForThread == 0);

#if defined(_DEBUG) && defined(TRACK_SYNC)
    _ASSERTE(IsAtProcessExit() || ((Dbg_TrackSyncStack *) m_pTrackSync)->m_StackPointer == 0);
    delete m_pTrackSync;
#endif // TRACK_SYNC

    _ASSERTE(IsDead() || IsAtProcessExit());

    if (m_WaitEventLink.m_Next != NULL && !IsAtProcessExit())
    {
        WaitEventLink *walk = &m_WaitEventLink;
        while (walk->m_Next) {
            ThreadQueue::RemoveThread(this, (SyncBlock*)((DWORD_PTR)walk->m_Next->m_WaitSB & ~1));
            StoreEventToEventStore (walk->m_Next->m_EventWait);
        }
        m_WaitEventLink.m_Next = NULL;
    }

#ifdef _DEBUG
    BOOL    ret = 
#endif
    ThreadStore::RemoveThread(this);
    _ASSERTE(ret);
    
#ifdef _DEBUG
    m_pFrame = (Frame *)POISONC;
#endif

    // Update Perfmon counters.
    COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsLogical--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsLogical--);
    
    // Current recognized threads are non-runtime threads that are alive and ran under the 
    // runtime. Check whether this Thread was one of them.
    if ((m_State & TS_WeOwn) == 0)
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cRecognizedThreads--);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cRecognizedThreads--);
    } 
    else
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical--);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical--);
    } 


    
    _ASSERTE(GetThreadHandle() == INVALID_HANDLE_VALUE);

    if (m_SafeEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_SafeEvent);
        m_SafeEvent = INVALID_HANDLE_VALUE;
    }
    if (m_SuspendEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_SuspendEvent);
        m_SuspendEvent = INVALID_HANDLE_VALUE;
    }
    if (m_EventWait != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_EventWait);
        m_EventWait = INVALID_HANDLE_VALUE;
    }
    if (m_OSContext != NULL)
        delete m_OSContext;

    if (GetSavedRedirectContext())
    {
        delete GetSavedRedirectContext();
        SetSavedRedirectContext(NULL);
    }


    if (!g_fProcessDetach)
    {
        if (m_LastThrownObjectHandle != NULL)
            DestroyHandle(m_LastThrownObjectHandle);
        if (m_handlerInfo.m_pThrowable != NULL) 
            DestroyHandle(m_handlerInfo.m_pThrowable);
        DestroyShortWeakHandle(m_ExposedObject);
        DestroyStrongHandle(m_StrongHndToExposedObject);
    }

    if (m_compressedStack != NULL)
        m_compressedStack->Release();

    ClearContext();

    DeleteThreadStaticData();
    if (g_fProcessDetach)
        RemoveAllDomainLocalStores();

    if(SystemDomain::BeforeFusionShutdown()) {
        SetFusionAssembly(NULL);
    }
}


void Thread::CleanupDetachedThreads(GCHeap::SUSPEND_REASON reason)
{
    _ASSERTE(ThreadStore::HoldingThreadStore());

    Thread *thread = ThreadStore::GetThreadList(NULL);

    while (m_DetachCount > 0 && thread != NULL)
    {
        Thread *next = ThreadStore::GetThreadList(thread);

        if (thread->IsDetached())
        {
            // Unmark that the thread is detached while we have the
            // thread store lock. This will ensure that no other
            // thread will race in here and try to delete it, too.
            FastInterlockAnd((ULONG*)&(thread->m_State), ~TS_Detached);
            FastInterlockDecrement(&m_DetachCount);
            if (!thread->IsBackground())
                FastInterlockDecrement(&m_ActiveDetachCount);
            
            // If the debugger is attached, then we need to unlock the
            // thread store before calling OnThreadTerminate. That
            // way, we won't be holding the thread store lock if we
            // need to block sending a detach thread event.
            BOOL debuggerAttached = 
#ifdef DEBUGGING_SUPPORTED
                CORDebuggerAttached();
#else // !DEBUGGING_SUPPORTED
                FALSE;
#endif // !DEBUGGING_SUPPORTED
            
            if (debuggerAttached)
                ThreadStore::UnlockThreadStore();
            
            thread->OnThreadTerminate(debuggerAttached ? FALSE : TRUE,
                                      FALSE);

#ifdef DEBUGGING_SUPPORTED
            // When we re-lock, we make sure to pass FALSE as the
            // second parameter to ensure that CleanupDetachedThreads
            // wont get called again. Because we've unlocked the
            // thread store, this may block due to a collection, but
            // that's okay.
            if (debuggerAttached)
            {
                ThreadStore::LockThreadStore(reason, FALSE);

                // We remember the next Thread in the thread store
                // list before deleting the current one. But we can't
                // use that Thread pointer now that we release the
                // thread store lock in the middle of the loop. We
                // have to start from the beginning of the list every
                // time. If two threads T1 and T2 race into
                // CleanupDetachedThreads, then T1 will grab the first
                // Thread on the list marked for deletion and release
                // the lock. T2 will grab the second one on the
                // list. T2 may complete destruction of its Thread,
                // then T1 might re-acquire the thread store lock and
                // try to use the next Thread in the thread store. But
                // T2 just deleted that next Thread.
                thread = ThreadStore::GetThreadList(NULL);
            }
            else
#endif // DEBUGGING_SUPPORTED
            {
                thread = next;
        }
        }
        else
            thread = next;
    }
}

// See general comments on thread destruction above.
void Thread::OnThreadTerminate(BOOL holdingLock,
                               BOOL threadCleanupAllowed)
{
    DWORD CurrentThreadID = ::GetCurrentThreadId();
    DWORD ThisThreadID = GetThreadId();


    // We took a count during construction, and we rely on the count being
    // non-zero as we terminate the thread here.
    _ASSERTE(m_ExternalRefCount > 0);
    
    if  (g_pGCHeap)
    {
        // Guaranteed to NOT be a shutdown case, because we tear down the heap before
        // we tear down any threads during shutdown.
        if (ThisThreadID == CurrentThreadID)
        {
            BOOL toggleGC = !PreemptiveGCDisabled();

            COMPLUS_TRY
            {
                if (toggleGC)
                    DisablePreemptiveGC();
            }
            COMPLUS_CATCH
            {
                // continue with the shutdown.  The 'try' scope must terminate before the
                // DecExternalCount() below, because the current thread might destruct
                // inside it.    We cannot tear down the SEH after the thread destructs, since
                // we use TLS and the Thread object itself during the tear down.
            }
            COMPLUS_END_CATCH
            g_pGCHeap->FixAllocContext(&m_alloc_context, FALSE, NULL);

            if (toggleGC)
                EnablePreemptiveGC();
        }
    }

    // cleanup the security handle now because if we wait for the destructor (triggered by the finalization 
    // of the managed thread object), the appdomain might already have been unloaded by then.
    if (m_compressedStack != NULL)
    {
        m_compressedStack->Release();
        m_compressedStack = NULL;
    }

    // We switch a thread to dead when it has finished doing useful work.  But it
    // remains in the thread store so long as someone keeps it alive.  An exposed
    // object will do this (it releases the refcount in its finalizer).  If the
    // thread is never released, we have another look during product shutdown and
    // account for the unreleased refcount of the uncollected exposed object:
    if (IsDead())
    {
        _ASSERTE(IsAtProcessExit());
        ClearContext();
        if (m_ExposedObject != NULL)
            DecExternalCount(holdingLock);             // may destruct now
    }
    else
    {
#ifdef PROFILING_SUPPORTED
        // If a profiler is present, then notify the profiler of thread destroy
        if (CORProfilerTrackThreads())
            g_profControlBlock.pProfInterface->ThreadDestroyed((ThreadID) this);

        if (CORProfilerInprocEnabled())
            g_pDebugInterface->InprocOnThreadDestroy(this);
#endif // PROFILING_SUPPORTED

#ifdef DEBUGGING_SUPPORTED
        //
        // If we're debugging, let the debugger know that this thread is
        // gone.
        //
        if (CORDebuggerAttached())
        {    
            g_pDebugInterface->DetachThread(this, holdingLock);
        }
#endif // DEBUGGING_SUPPORTED

        if (!holdingLock)
        {
            LOG((LF_SYNC, INFO3, "OnThreadTerminate obtain lock\n"));
            ThreadStore::LockThreadStore(GCHeap::SUSPEND_OTHER,
                                         threadCleanupAllowed);
        }

        if  (g_pGCHeap && ThisThreadID != CurrentThreadID)
        {
            // We must be holding the ThreadStore lock in order to clean up alloc context.
            // We should never call FixAllocContext during GC.
            g_pGCHeap->FixAllocContext(&m_alloc_context, FALSE, NULL);
        }

        FastInterlockOr((ULONG *) &m_State, TS_Dead);
        g_pThreadStore->m_DeadThreadCount++;

        if (IsUnstarted())
            g_pThreadStore->m_UnstartedThreadCount--;
        else
        {
            if (IsBackground())
                g_pThreadStore->m_BackgroundThreadCount--;
        }

        FastInterlockAnd((ULONG *) &m_State, ~(TS_Unstarted | TS_Background));

        //
        // If this thread was told to trip for debugging between the
        // sending of the detach event above and the locking of the
        // thread store lock, then remove the flag and decrement the
        // global trap returning threads count.
        //
        if (!IsAtProcessExit())
        {
            // A thread can't die during a GCPending, because the thread store's
            // lock is held by the GC thread.
            if (m_State & TS_DebugSuspendPending)
                UnmarkForSuspension(~TS_DebugSuspendPending);

            if (m_State & TS_UserSuspendPending)
                UnmarkForSuspension(~TS_UserSuspendPending);
        }
        
        if (m_ThreadHandle != INVALID_HANDLE_VALUE)
        {
            _ASSERTE (m_ThreadHandleForClose == INVALID_HANDLE_VALUE);
            m_ThreadHandleForClose = m_ThreadHandle;
            m_ThreadHandle = INVALID_HANDLE_VALUE;
        }

        m_ThreadId = NULL;

        // Perhaps threads are waiting to suspend us.  This is as good as it gets.
        SetSafeEvent();

        // If nobody else is holding onto the thread, we may destruct it here:
        ULONG   oldCount = m_ExternalRefCount;

        DecExternalCount(TRUE);
        oldCount--;
        // If we are shutting down the process, we only have one thread active in the
        // system.  So we can disregard all the reasons that hold this thread alive --
        // TLS is about to be reclaimed anyway.
        if (IsAtProcessExit())
            while (oldCount > 0)
            {
                DecExternalCount(TRUE);
                oldCount--;
            }

        // ASSUME THAT THE THREAD IS DELETED, FROM HERE ON

        _ASSERTE(g_pThreadStore->m_ThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_BackgroundThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_UnstartedThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_DeadThreadCount);

        // One of the components of OtherThreadsComplete() has changed, so check whether
        // we should now exit the EE.
        ThreadStore::CheckForEEShutdown();

        if (!holdingLock)
        {
            _ASSERTE(g_pThreadStore->m_HoldingThread == this || !threadCleanupAllowed || g_fProcessDetach);
            LOG((LF_SYNC, INFO3, "OnThreadTerminate releasing lock\n"));
            ThreadStore::UnlockThreadStore();
            _ASSERTE(g_pThreadStore->m_HoldingThread != this || g_fProcessDetach);
        }

        if (ThisThreadID == CurrentThreadID)
        {
            // NULL out the thread block  in the tls.  We can't do this if we aren't on the
            // right thread.  But this will only happen during a shutdown.  And we've made
            // a "best effort" to reduce to a single thread before we begin the shutdown.
            TlsSetValue(gThreadTLSIndex, (VOID*)NULL);
            TlsSetValue(gAppDomainTLSIndex, (VOID*)NULL);
        }
    }
}

// Helper functions to check for duplicate handles. we only do this check if
// a waitfor multiple fails.
int __cdecl compareHandles( const void *arg1, const void *arg2 )
{
    HANDLE h1 = *(HANDLE*)arg1;
    HANDLE h2 = *(HANDLE*)arg2;
    return  (h1 == h2) ? 0 : ((h1 < h2) ? -1 : 1);
}

BOOL CheckForDuplicateHandles(int countHandles, HANDLE *handles)
{
    qsort(handles,countHandles,sizeof(HANDLE),compareHandles);
    for (int i=0; i < countHandles-1; i++)
    {
        if (handles[i] == handles[i+1])
            return TRUE;
    }
    return FALSE;
}
//--------------------------------------------------------------------
// Based on whether this thread has a message pump, do the appropriate
// style of Wait.
//--------------------------------------------------------------------
DWORD Thread::DoAppropriateWait(int countHandles, HANDLE *handles, BOOL waitAll,
                                DWORD millis, BOOL alertable, PendingSync *syncState)
{
    _ASSERTE(alertable || syncState == 0);
    THROWSCOMPLUSEXCEPTION(); // ThreadInterruptedException
    
    DWORD dwRet = (DWORD) -1;

    EE_TRY_FOR_FINALLY {
        dwRet =DoAppropriateWaitWorker(countHandles, handles, waitAll, millis, alertable);
    }
    EE_FINALLY {
        if (syncState) {
            if (!__GotException &&
                dwRet >= WAIT_OBJECT_0 && dwRet < (DWORD)(WAIT_OBJECT_0 + countHandles)) {
                // This thread has been removed from syncblk waiting list by the signalling thread
                syncState->Restore(FALSE);
            }
            else
                syncState->Restore(TRUE);
        }
    
        if (!__GotException && dwRet == WAIT_IO_COMPLETION)
        {
            if (!PreemptiveGCDisabled())
                DisablePreemptiveGC();
            // if an interrupt and abort happen at the same time, give priority to abort
            if (IsAbortRequested() && 
                    !IsAbortInitiated() &&
                    (GetThrowable() == NULL))
            {
                    SetAbortInitiated();
                    COMPlusThrow(kThreadAbortException);
            }

            COMPlusThrow(kThreadInterruptedException);
        }
    }
    EE_END_FINALLY;
    
    return(dwRet);
}


//--------------------------------------------------------------------
// Do appropriate wait based on apartment state (STA or MTA)
DWORD Thread::DoAppropriateAptStateWait(int numWaiters, HANDLE* pHandles, BOOL bWaitAll, 
                                         DWORD timeout,BOOL alertable)
{
    return ::WaitForMultipleObjectsEx(numWaiters, pHandles,bWaitAll, timeout,alertable);
}

//--------------------------------------------------------------------
// Based on whether this thread has a message pump, do the appropriate
// style of Wait.
//--------------------------------------------------------------------
DWORD Thread::DoAppropriateWaitWorker(int countHandles, HANDLE *handles, BOOL waitAll,
                                      DWORD millis, BOOL alertable)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!GetThread()->GCForbidden());

    // During a <clinit>, this thread must not be asynchronously
    // stopped or interrupted.  That would leave the class unavailable
    // and is therefore a security hole.  We don't have to worry about
    // multithreading, since we only manipulate the current thread's count.
    if(alertable && m_PreventAsync > 0)
        alertable = FALSE;

    // disable GC (toggle)
    BOOL toggleGC = PreemptiveGCDisabled();
    if(toggleGC)
        EnablePreemptiveGC();

    DWORD ret;
    if(alertable)
    {
        // A word about ordering for Interrupt.  If someone tries to interrupt a thread
        // that's in the interruptible state, we queue an APC.  But if they try to interrupt
        // a thread that's not in the interruptible state, we just record that fact.  So
        // we have to set TS_Interruptible before we test to see whether someone wants to
        // interrupt us or else we have a race condition that causes us to skip the APC.
        FastInterlockOr((ULONG *) &m_State, TS_Interruptible);

        // If someone has interrupted us, we should not enter the wait.
        if (IsUserInterrupted(TRUE /*=reset*/))
        {
            // It is safe to clear the following two bits of state while
            // m_UserInterrupt is clear since both bits are only manipulated
            // within the context of the thread (TS_Interrupted is set via APC,
            // but these are not half as asynchronous as their name implies). If
            // an APC was queued, it has either gone off (and set the
            // TS_Interrupted bit which we're about to clear) or will execute at
            // some arbitrary later time. This is OK. If it executes while an
            // interrupt is not being requested it will simply become a no-op.
            // Otherwise it will serve it's original intended purpose (we don't
            // care which APC matches up with which interrupt attempt).
            FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

            if (toggleGC)
                DisablePreemptiveGC();

            return(WAIT_IO_COMPLETION);
        }
        // Safe to clear the interrupted state, no APC could have fired since we
        // reset m_UserInterrupt (which inhibits our APC callback from doing
        // anything).
        FastInterlockAnd((ULONG *) &m_State, ~TS_Interrupted);
    }

    DWORD dwStart, dwEnd;    
retry:
    dwStart = ::GetTickCount();
    BOOL blocked = FALSE;
    if (FALSE /*g_Win32Threadpool*/ && (m_State & TS_ThreadPoolThread)) 
    {
        blocked = ThreadpoolMgr::ThreadAboutToBlock(this);    // inform the threadpool that this thread is about to block
    }
    ret = DoAppropriateAptStateWait(countHandles, handles, waitAll, millis, alertable);
    if (blocked) 
    {
        ThreadpoolMgr::ThreadAboutToUnblock();  // inform the threadpool that a previously blocked thread is now ready to run
    }
    if (ret == WAIT_IO_COMPLETION)
    {
        // We could be woken by some spurious APC or an EE APC queued to
        // interrupt us. In the latter case the TS_Interrupted bit will be set
        // in the thread state bits. Otherwise we just go back to sleep again.
        if (!(m_State & TS_Interrupted))
        {
            // Compute the new timeout value by assume that the timeout 
            // is not large enough for more than one wrap
            dwEnd = ::GetTickCount();
            if (millis != INFINITE)
            {
                DWORD newTimeout;
                if(dwStart <= dwEnd)
                    newTimeout = millis - (dwEnd - dwStart);
                else
                    newTimeout = millis - (0xFFFFFFFF - dwStart - dwEnd);
                // check whether the delta is more than millis
                if (newTimeout > millis)    
                {
                    ret = WAIT_TIMEOUT;
                    goto WaitCompleted;
                }
                else
                    millis = newTimeout;
            }
            goto retry;
        }
    }
    _ASSERTE((ret >= WAIT_OBJECT_0  && ret < (DWORD)(WAIT_OBJECT_0  + countHandles)) ||
             (ret >= WAIT_ABANDONED && ret < (DWORD)(WAIT_ABANDONED + countHandles)) ||
             (ret == WAIT_TIMEOUT) || (ret == WAIT_IO_COMPLETION) || (ret == WAIT_FAILED));

    // We support precisely one WAIT_FAILED case, where we attempt to wait on a
    // thread handle and the thread is in the process of dying we might get a
    // invalid handle substatus. Turn this into a successful wait.
    // There are three cases to consider:
    //  1)  Only waiting on one handle: return success right away.
    //  2)  Waiting for all handles to be signalled: retry the wait without the
    //      affected handle.
    //  3)  Waiting for one of multiple handles to be signalled: return with the
    //      first handle that is either signalled or has become invalid.
    if (ret == WAIT_FAILED)
    {
        DWORD errorCode = ::GetLastError();
        if (errorCode == ERROR_INVALID_PARAMETER) 
        {
            if (toggleGC)
                DisablePreemptiveGC();
            if (CheckForDuplicateHandles(countHandles, handles))
                COMPlusThrow(kDuplicateWaitObjectException);
            else
                COMPlusThrowWin32();
        }

        _ASSERTE(errorCode == ERROR_INVALID_HANDLE);

        if (countHandles == 1)
            ret = WAIT_OBJECT_0;
        else if (waitAll)
        {
            // Probe all handles with a timeout of zero. When we find one that's
            // invalid, move it out of the list and retry the wait.
#ifdef _DEBUG
            BOOL fFoundInvalid = FALSE;
#endif
            for (int i = 0; i < countHandles; i++)
            {
                DWORD subRet = WaitForSingleObject(handles[i], 0);
                if (subRet != WAIT_FAILED)
                    continue;
                _ASSERTE(::GetLastError() == ERROR_INVALID_HANDLE);
                if ((countHandles - i - 1) > 0)
                    memmove(&handles[i], &handles[i+1], (countHandles - i - 1) * sizeof(HANDLE));
                countHandles--;
#ifdef _DEBUG
                fFoundInvalid = TRUE;
#endif
                break;
            }
            _ASSERTE(fFoundInvalid);

            // Compute the new timeout value by assume that the timeout 
            // is not large enough for more than one wrap
            dwEnd = ::GetTickCount();
            if (millis != INFINITE)
            {
                DWORD newTimeout;
                if(dwStart <= dwEnd)
                    newTimeout = millis - (dwEnd - dwStart);
                else
                    newTimeout = millis - (0xFFFFFFFF - dwStart - dwEnd);
                if (newTimeout > millis)
                    goto WaitCompleted;
                else
                    millis = newTimeout;
            }
            goto retry;
        }
        else
        {
            // Probe all handles with a timeout as zero, succeed with the first
            // handle that doesn't timeout.
            ret = WAIT_OBJECT_0;
            int i;
            for (i = 0; i < countHandles; i++)
            {
            TryAgain:
                DWORD subRet = WaitForSingleObject(handles[i], 0);
                if ((subRet == WAIT_OBJECT_0) || (subRet == WAIT_FAILED))
                    break;
                if (subRet == WAIT_ABANDONED)
                {
                    ret = (ret - WAIT_OBJECT_0) + WAIT_ABANDONED;
                    break;
                }
                // If we get alerted it just masks the real state of the current
                // handle, so retry the wait.
                if (subRet == WAIT_IO_COMPLETION)
                    goto TryAgain;
                _ASSERTE(subRet == WAIT_TIMEOUT);
                ret++;
            }
            _ASSERTE(i != countHandles);
        }
    }

WaitCompleted:

    _ASSERTE((ret != WAIT_TIMEOUT) || (millis != INFINITE));

    if (toggleGC)
        DisablePreemptiveGC();

    if (alertable)
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

    // Make one last check to see if an interrupt request was made (and clear it
    // atomically). It's OK to clear the previous two bits first because they're
    // only accessed from this thread context.
    if (IsUserInterrupted(TRUE /*=reset*/))
        ret = WAIT_IO_COMPLETION;

    return ret;
}




// Called out of SyncBlock::Wait() to block this thread until the Notify occurs.
BOOL Thread::Block(INT32 timeOut, PendingSync *syncState)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(this == GetThread());

    // Before calling Block, the SyncBlock queued us onto it's list of waiting threads.
    // However, before calling Block the SyncBlock temporarily left the synchronized
    // region.  This allowed threads to enter the region and call Notify, in which
    // case we may have been signalled before we entered the Wait.  So we aren't in the
    // m_WaitSB list any longer.  Not a problem: the following Wait will return
    // immediately.  But it means we cannot enforce the following assertion:
//    _ASSERTE(m_WaitSB != NULL);

    return (Wait(&syncState->m_WaitEventLink->m_Next->m_EventWait, 1, timeOut, syncState) != WAIT_OBJECT_0);
}


// Return whether or not a timeout occured.  TRUE=>we waited successfully
DWORD Thread::Wait(HANDLE *objs, int cntObjs, INT32 timeOut, PendingSync *syncInfo)
{
    DWORD   dwResult;
    DWORD   dwTimeOut32;

    _ASSERTE(timeOut >= 0 || timeOut == INFINITE_TIMEOUT);

    dwTimeOut32 = (timeOut == INFINITE_TIMEOUT
                   ? INFINITE
                   : (DWORD) timeOut);

    dwResult = DoAppropriateWait(cntObjs, objs, FALSE /*=waitAll*/, dwTimeOut32,
                                 TRUE /*alertable*/, syncInfo);

    // Either we succeeded in the wait, or we timed out
    _ASSERTE((dwResult >= WAIT_OBJECT_0 && dwResult < (DWORD)(WAIT_OBJECT_0 + cntObjs)) ||
             (dwResult == WAIT_TIMEOUT));

    return dwResult;
}

void Thread::Wake(SyncBlock *psb)
{
    HANDLE hEvent = INVALID_HANDLE_VALUE;
    WaitEventLink *walk = &m_WaitEventLink;
    while (walk->m_Next) {
        if (walk->m_Next->m_WaitSB == psb) {
            hEvent = walk->m_Next->m_EventWait;
            // We are guaranteed that only one thread can change walk->m_Next->m_WaitSB
            // since the thread is helding the syncblock.
            walk->m_Next->m_WaitSB = (SyncBlock*)((DWORD_PTR)walk->m_Next->m_WaitSB | 1);
            break;
        }
#ifdef _DEBUG
        else if ((SyncBlock*)((DWORD_PTR)walk->m_Next & ~1) == psb) {
            _ASSERTE (!"Can not wake a thread on the same SyncBlock more than once");
        }
#endif
    }
    _ASSERTE (hEvent != INVALID_HANDLE_VALUE);
    ::SetEvent(hEvent);
}


// This is the service that backs us out of a wait that we interrupted.  We must
// re-enter the monitor to the same extent the SyncBlock would, if we returned
// through it (instead of throwing through it).  And we need to cancel the wait,
// if it didn't get notified away while we are processing the interrupt.
void PendingSync::Restore(BOOL bRemoveFromSB)
{
    _ASSERTE(m_EnterCount);

    Thread      *pCurThread = GetThread();

    _ASSERTE (pCurThread == m_OwnerThread);

    WaitEventLink *pRealWaitEventLink = m_WaitEventLink->m_Next;

    pRealWaitEventLink->m_RefCount --;
    if (pRealWaitEventLink->m_RefCount == 0)
    {
        if (bRemoveFromSB) {
            ThreadQueue::RemoveThread(pCurThread, pRealWaitEventLink->m_WaitSB);
        }
        if (pRealWaitEventLink->m_EventWait != pCurThread->m_EventWait) {
            // Put the event back to the pool.
            StoreEventToEventStore(pRealWaitEventLink->m_EventWait);
        }
        // Remove from the link.
        m_WaitEventLink->m_Next = m_WaitEventLink->m_Next->m_Next;
    }

    // Someone up the stack is responsible for keeping the syncblock alive by protecting
    // the object that owns it.  But this relies on assertions that EnterMonitor is only
    // called in cooperative mode.  Even though we are safe in preemptive, do the
    // switch.
    pCurThread->DisablePreemptiveGC();

    SyncBlock *psb = (SyncBlock*)((DWORD_PTR)pRealWaitEventLink->m_WaitSB & ~1);
    for (LONG i=0; i < m_EnterCount; i++)
         psb->EnterMonitor();

    pCurThread->EnablePreemptiveGC();
}



// This is the callback from the OS, when we queue an APC to interrupt a waiting thread.
// The callback occurs on the thread we wish to interrupt.  It is a STATIC method.
void __stdcall Thread::UserInterruptAPC(ULONG_PTR data)
{
    _ASSERTE(data == APC_Code);

    Thread *pCurThread = GetThread();
    if (pCurThread)
        // We should only take action if an interrupt is currently being
        // requested (our synchronization does not guarantee that we won't fire
        // spuriously). It's safe to check the m_UserInterrupt field and then
        // set TS_Interrupted in a non-atomic fashion because m_UserInterrupt is
        // only cleared in this thread's context (though it may be set from any
        // context).
        if (pCurThread->m_UserInterrupt)
            // Set bit to indicate this routine was called (as opposed to other
            // generic APCs).
            FastInterlockOr((ULONG *) &pCurThread->m_State, TS_Interrupted);
}

// This is the workhorse for Thread.Interrupt().
void Thread::UserInterrupt()
{
    // Transition from 0 to 1 implies we are responsible for queueing an APC.
    if ((FastInterlockExchange(&m_UserInterrupt, 1) == 0) &&
         GetThreadHandle() &&
         (m_State & TS_Interruptible))
    {
        ::QueueUserAPC((PAPCFUNC)UserInterruptAPC, GetThreadHandle(), APC_Code);
    }
}

// Is the interrupt flag set?  Optionally, reset it
DWORD Thread::IsUserInterrupted(BOOL reset)
{
    LONG    state = (reset
                     ? FastInterlockExchange(&m_UserInterrupt, 0)
                     : m_UserInterrupt);

    return (state);
}

// Implementation of Thread.Sleep().
void Thread::UserSleep(INT32 time)
{
    _ASSERTE(!GetThread()->GCForbidden());

    THROWSCOMPLUSEXCEPTION();       // InterruptedException

    BOOL    alertable = (m_PreventAsync == 0);
    DWORD   res;

    EnablePreemptiveGC();

    if (alertable)
    {
        // A word about ordering for Interrupt.  If someone tries to interrupt a thread
        // that's in the interruptible state, we queue an APC.  But if they try to interrupt
        // a thread that's not in the interruptible state, we just record that fact.  So
        // we have to set TS_Interruptible before we test to see whether someone wants to
        // interrupt us or else we have a race condition that causes us to skip the APC.
        FastInterlockOr((ULONG *) &m_State, TS_Interruptible);

        // If someone has interrupted us, we should not enter the wait.
        if (IsUserInterrupted(TRUE /*=reset*/))
        {
            // It is safe to clear the following two bits of state while
            // m_UserInterrupt is clear since both bits are only manipulated
            // within the context of the thread (TS_Interrupted is set via APC,
            // but these are not half as asynchronous as their name implies). If
            // an APC was queued, it has either gone off (and set the
            // TS_Interrupted bit which we're about to clear) or will execute at
            // some arbitrary later time. This is OK. If it executes while an
            // interrupt is not being requested it will simply become a no-op.
            // Otherwise it will serve it's original intended purpose (we don't
            // care which APC matches up with which interrupt attempt).
            FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));
            DisablePreemptiveGC();
            COMPlusThrow(kThreadInterruptedException);
        }
        // Safe to clear the interrupted state, no APC could have fired since we
        // reset m_UserInterrupt (which inhibits our APC callback from doing
        // anything).
        FastInterlockAnd((ULONG *) &m_State, ~TS_Interrupted);
    }

retry:

    BOOL blocked = FALSE;
    if (FALSE /*g_Win32Threadpool*/ && (m_State & TS_ThreadPoolThread)) 
    {
        blocked = ThreadpoolMgr::ThreadAboutToBlock(this);    // inform the threadpool that this thread is about to block
    }
    res = ::SleepEx(time, alertable);
    if (blocked) 
    {
        ThreadpoolMgr::ThreadAboutToUnblock();  // inform the threadpool that a previously blocked thread is now ready to run
    }
    if (res == WAIT_IO_COMPLETION)
    {
        _ASSERTE(alertable);

        // We could be woken by some spurious APC or an EE APC queued to
        // interrupt us. In the latter case the TS_Interrupted bit will be set
        // in the thread state bits. Otherwise we just go back to sleep again.
        if (!(m_State & TS_Interrupted))
        {
            // Don't bother with accurate accounting here.  Just ensure we make progress.
            // Note that this is not us doing the APC.
			if (time == 0)
			{
				res = WAIT_TIMEOUT;
			}
			else
			{
            if (time != (INT32) INFINITE)
                time--;

            goto retry;
        }
    }
    }
    _ASSERTE(res == WAIT_TIMEOUT || res == WAIT_OBJECT_0 || res == WAIT_IO_COMPLETION);

    DisablePreemptiveGC();
    HandleThreadAbort();
    
    if (alertable)
    {
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

    // Make one last check to see if an interrupt request was made (and clear it
    // atomically). It's OK to clear the previous two bits first because they're
    // only accessed from this thread context.
    if (IsUserInterrupted(TRUE /*=reset*/))
        res = WAIT_IO_COMPLETION;

        if (res == WAIT_IO_COMPLETION)
            COMPlusThrow(kThreadInterruptedException);
    }
}

OBJECTREF Thread::GetExposedObjectRaw()
{
    return ObjectFromHandle(m_ExposedObject);
}
 
// Correspondence between an EE Thread and an exposed System.Thread:
OBJECTREF Thread::GetExposedObject()
{
    THROWSCOMPLUSEXCEPTION();
    Thread *pCurThread = GetThread();
    _ASSERTE (!(pCurThread == NULL || g_fProcessDetach));

    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    if (ObjectFromHandle(m_ExposedObject) == NULL)
    {
        // Initialize ThreadNative::m_MT if it hasn't been done yet...
        ThreadNative::InitThread();

        // Allocate the exposed thread object.
        THREADBASEREF attempt = (THREADBASEREF) AllocateObject(ThreadNative::m_MT);
        GCPROTECT_BEGIN(attempt);

        BOOL fNeedThreadStore = (! ThreadStore::HoldingThreadStore(pCurThread));
        if (fNeedThreadStore) {
            // Take a lock to make sure that only one thread creates the object.
            pCurThread->EnablePreemptiveGC();
            ThreadStore::LockThreadStore();
            pCurThread->DisablePreemptiveGC();
        }

        // Check to see if another thread has not already created the exposed object.
        if (ObjectFromHandle(m_ExposedObject) == NULL)
        {
            // Keep a weak reference to the exposed object.
            StoreObjectInHandle(m_ExposedObject, (OBJECTREF) attempt);

            // Increase the external ref count. We can't call IncExternalCount because we
            // already hold the thread lock and IncExternalCount won't be able to take it.
            m_ExternalRefCount++;

            // Check to see if we need to store a strong pointer to the object.
            if (m_ExternalRefCount > 1)
                StoreObjectInHandle(m_StrongHndToExposedObject, (OBJECTREF) attempt);

            EE_TRY_FOR_FINALLY
            {
                // The exposed object keeps us alive until it is GC'ed.  This
                // doesn't mean the physical thread continues to run, of course.
                attempt->SetInternal(this);

                // Note that we are NOT calling the constructor on the Thread.  That's
                // because this is an internal create where we don't want a Start
                // address.  And we don't want to expose such a constructor for our
                // customers to accidentally call.  The following is in lieu of a true
                // constructor:
                attempt->InitExisting();
            }
            EE_FINALLY
            {
                if (GOT_EXCEPTION()) {
                    // Set both the weak and the strong handle's to NULL.
                    StoreObjectInHandle(m_ExposedObject, NULL);
                    StoreObjectInHandle(m_StrongHndToExposedObject, NULL);
                }
                // Now that we have stored the object in the handle we can release the lock.

                if (fNeedThreadStore)
                    ThreadStore::UnlockThreadStore();
            } EE_END_FINALLY;
        }
        else if (fNeedThreadStore)
            ThreadStore::UnlockThreadStore();

        GCPROTECT_END();
    }
    return ObjectFromHandle(m_ExposedObject);
}


// We only set non NULL exposed objects for unstarted threads that haven't exited 
// their constructor yet.  So there are no race conditions.
void Thread::SetExposedObject(OBJECTREF exposed)
{
    if (exposed != NULL)
    {
        _ASSERTE(IsUnstarted());
        _ASSERTE(ObjectFromHandle(m_ExposedObject) == NULL);
        // The exposed object keeps us alive until it is GC'ed.  This doesn't mean the
        // physical thread continues to run, of course.
        StoreObjectInHandle(m_ExposedObject, exposed);
        // This makes sure the contexts on the backing thread
        // and the managed thread start off in sync with each other.
        _ASSERTE(m_Context);
        ((THREADBASEREF)exposed)->SetExposedContext(m_Context->GetExposedObjectRaw());
        // BEWARE: the IncExternalCount call below may cause GC to happen.

        IncExternalCount();
    }
    else
    {
        // Simply set both of the handles to NULL. The GC of the old exposed thread
        // object will take care of decrementing the external ref count.
        StoreObjectInHandle(m_ExposedObject, NULL);
        StoreObjectInHandle(m_StrongHndToExposedObject, NULL);
    }
}


void Thread::SetLastThrownObject(OBJECTREF throwable) {

     if (m_LastThrownObjectHandle != NULL)
         DestroyHandle(m_LastThrownObjectHandle);

     if (throwable == NULL)
         m_LastThrownObjectHandle = NULL;
     else
         _ASSERTE(this == GetThread());
         m_LastThrownObjectHandle = GetDomain()->CreateHandle(throwable);
}


BOOL Thread::IsAtProcessExit()
{
    return ((g_pThreadStore->m_StoreState & ThreadStore::TSS_ShuttingDown) != 0);
}


// returns 0 if the thread is already marked to be aborted.
// else returns the new value of m_PendingExceptions
BOOL Thread::MarkThreadForAbort()
{

    DWORD initialValue = m_State;
    DWORD newValue;

    while (true)
    {
        if (initialValue & TS_AbortRequested)   // thread already marked for abort by someone else
            return FALSE;

        newValue = (initialValue | TS_AbortRequested);
        
        DWORD oldValue = FastInterlockCompareExchange((LONG*)&m_State, newValue, initialValue);
        if (initialValue  == oldValue)                              // exchange succeeded
            return TRUE;    
        else
            initialValue = oldValue;
    } 
}


void Thread::UserAbort(THREADBASEREF orThreadBase)
{
    THROWSCOMPLUSEXCEPTION();

    // We must set this before we start flipping thread bits to avoid races where
    // trap returning threads is already high due to other reasons.

    ThreadStore::TrapReturningThreads(TRUE);

    if (!MarkThreadForAbort()) { // the thread was already marked to be aborted
        ThreadStore::TrapReturningThreads(FALSE);
        return;
    }

    GCPROTECT_BEGIN(orThreadBase) {

    // else we are the first one to abort and there are no pending exceptions 
    if (this == GetThread())
    {
        SetAbortInitiated();
        // TrapReturningThreads will be decremented when the exception leaves managed code.
        COMPlusThrow(kThreadAbortException);
    }


    _ASSERTE(this != GetThread());      // Aborting another thread.
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);

#ifdef _DEBUG
    DWORD elapsed_time = 0;
#endif

    for (;;) {

        // Lock the thread store
        LOG((LF_SYNC, INFO3, "UserAbort obtain lock\n"));
        ThreadStore::LockThreadStore();     // GC can occur here.

        // Get the thread handle.
        HANDLE hThread = GetThreadHandle();

        if (hThread == INVALID_HANDLE_VALUE) {

            // Take a lock, and get the handle again.  This lock is necessary to syncronize 
            // with the startup code.
            orThreadBase->EnterObjMonitor();
            hThread = GetThreadHandle();
            DWORD state = m_State;
            orThreadBase->LeaveObjMonitor();

            // Could be unstarted, in which case, we just leave.
            if (hThread == INVALID_HANDLE_VALUE) {
                if (state & TS_Unstarted) {
                    // This thread is not yet started.  Leave the thread marked for abort, reset
                    // the trap returning count, and leave.
                    _ASSERTE(state & TS_AbortRequested);
                    ThreadStore::TrapReturningThreads(FALSE);
                    break;
                } else {
                    // Must be dead, or about to die.
                    if (state & TS_AbortRequested)
                        ThreadStore::TrapReturningThreads(FALSE);
                    break;
                }
            }
        }

        // Win32 suspend the thread, so it isn't moving under us.
#ifdef _DEBUG
        DWORD oldSuspendCount = (DWORD)
#endif
        ::SuspendThread(hThread);   // returns -1 on failure.

        _ASSERTE(oldSuspendCount != (DWORD) -1);

        // What if someone else has this thread suspended already?   It'll depend where the
        // thread got suspended.
        //
        // User Suspend:
        //     We'll just set the abort bit and hope for the best on the resume.
        //
        // GC Suspend:
        //    If it's suspended in jitted code, we'll hijack the IP.
        //    If it's suspended but not in jitted code, we'll get suspended for GC, the GC
        //    will complete, and then we'll abort the target thread.
        //

        // It's possible that the thread has completed the abort already.
        //
        if (!(m_State & TS_AbortRequested)) {
            ::ResumeThread(hThread);
            break;
        }

        _ASSERTE(m_State & TS_AbortRequested);

        // If a thread is Dead or Detached, abort is a NOP.
        //
        if (m_State & (TS_Dead | TS_Detached)) {
            ThreadStore::TrapReturningThreads(FALSE);
            ::ResumeThread(hThread);
            break;
        }

        // It's possible that some stub notices the AbortRequested bit -- even though we 
        // haven't done any real magic yet.  If the thread has already started it's abort, we're 
        // done.
        //
        // Two more cases can be folded in here as well.  If the thread is unstarted, it'll
        // abort when we start it.
        //
        // If the thread is user suspended (SyncSuspended) -- we're out of luck.  Set the bit and 
        // hope for the best on resume. 
        // 
        if (m_State & (TS_AbortInitiated | TS_Unstarted)) {
            _ASSERTE(m_State & TS_AbortRequested);
            ::ResumeThread(hThread);
            break;
        }

        if (m_State & TS_SyncSuspended) {
            ThreadStore::TrapReturningThreads(FALSE);
            ThreadStore::UnlockThreadStore();
            COMPlusThrow(kThreadStateException, IDS_EE_THREAD_ABORT_WHILE_SUSPEND);
            _ASSERTE(0); // NOT REACHED
        }

        // If the thread has no managed code on it's call stack, abort is a NOP.  We're about
        // to touch the unmanaged thread's stack -- for this to be safe, we can't be 
        // Dead/Detached/Unstarted.
        //
        _ASSERTE(!(m_State & (  TS_Dead 
                              | TS_Detached 
                              | TS_Unstarted 
                              | TS_AbortInitiated))); 

        if (    m_pFrame == FRAME_TOP 
            && GetFirstCOMPlusSEHRecord(this) == EXCEPTION_CHAIN_END) {
            FastInterlockAnd((ULONG *)&m_State, 
                             ((~TS_AbortRequested) & (~TS_AbortInitiated) & (~TS_StopRequested)));
            ThreadStore::TrapReturningThreads(FALSE);
            ::ResumeThread(hThread);
            break;
        }

        // If an exception is currently being thrown, one of two things will happen.  Either, we'll
        // catch, and notice the abort request in our end-catch, or we'll not catch [in which case
        // we're leaving managed code anyway.  The top-most handler is responsible for resetting
        // the bit.
        //
        if (GetThrowable() != NULL) {
            ::ResumeThread(hThread);
            break;
        }

        // If the thread is in sleep, wait, or join interrupt it
        // However, we do NOT want to interrupt if the thread is already processing an exception
        if (m_State & TS_Interruptible) {
            UserInterrupt();        // if the user wakes up because of this, it will read the 
                                    // abort requested bit and initiate the abort
            ::ResumeThread(hThread);
            break;
        } 
        
        if (!m_fPreemptiveGCDisabled) {
            if (   m_pFrame != FRAME_TOP
                && m_pFrame->IsTransitionToNativeFrame()
                && ((size_t) GetFirstCOMPlusSEHRecord(this) > ((size_t) m_pFrame) - 20)
                ) {
                // If the thread is running outside the EE, and is behind a stub that's going
                // to catch...
                ::ResumeThread(hThread);
                break;
            } 
        }

        // Ok.  It's not in managed code, nor safely out behind a stub that's going to catch
        // it on the way in.  We have to poll.

        ::ResumeThread(hThread);
        ThreadStore::UnlockThreadStore();

        // Don't do a Sleep.  It's possible that the thread we are trying to abort is
        // stuck in unmanaged code trying to get into the apartment that we are supposed
        // to be pumping!  Instead, ping the current thread's handle.  Obviously this
        // will time out, but it will pump if we need it to.
        // ::Sleep(ABORT_POLL_TIMEOUT);
        {
            Thread *pCurThread = GetThread();  // not the thread we are aborting!
            HANDLE  h = pCurThread->GetThreadHandle();
            pCurThread->DoAppropriateWait(1, &h, FALSE, ABORT_POLL_TIMEOUT, TRUE, NULL);
        }



#ifdef _DEBUG
        elapsed_time += ABORT_POLL_TIMEOUT;
        _ASSERTE(elapsed_time < ABORT_FAIL_TIMEOUT);
#endif

    } // for(;;)

    } GCPROTECT_END(); // orThreadBase

    _ASSERTE(ThreadStore::HoldingThreadStore());
    ThreadStore::UnlockThreadStore();
}


void Thread::UserResetAbort()
{
    _ASSERTE(this == GetThread());
    _ASSERTE(IsAbortRequested());
    _ASSERTE(!IsDead());

    ThreadStore::TrapReturningThreads(FALSE);
    FastInterlockAnd((ULONG *)&m_State, ((~TS_AbortRequested) & (~TS_AbortInitiated) & (~TS_StopRequested)));
    GetHandlerInfo()->ResetIsInUnmanagedHandler();
}


// The debugger needs to be able to perform a UserStop on a runtime
// thread. Since this will only ever happen from the helper thread, we
// can't call the normal UserStop, since that can throw a COM+
// exception. This is a minor variant on UserStop that does the same
// thing.
//
// See the notes in UserStop() above for more details on what this is
// doing.
void Thread::UserStopForDebugger()
{
    // Note: this can only happen from the debugger helper thread.
    _ASSERTE(dbgOnly_IsSpecialEEThread());
    
    UserSuspendThread();
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);
    UserResumeThread();
}

// No longer attempt to Stop this thread.
void Thread::ResetStopRequest()
{
    FastInterlockAnd((ULONG *) &m_State, ~TS_StopRequested);
}

// Throw a thread stop request when a suspended thread is resumed. Make sure you know what you
// are doing when you call this routine.
void Thread::SetStopRequest()
{
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);
}

// Throw a thread abort request when a suspended thread is resumed. Make sure you know what you
// are doing when you call this routine.
void Thread::SetAbortRequest()
{
    MarkThreadForAbort();
    SetStopRequest();

    if (m_State & TS_Interruptible)
        UserInterrupt();

    ThreadStore::TrapReturningThreads(TRUE);
}

// Background threads must be counted, because the EE should shut down when the
// last non-background thread terminates.  But we only count running ones.
void Thread::SetBackground(BOOL isBack)
{
    // booleanize IsBackground() which just returns bits
    if (isBack == !!IsBackground())
        return;

    LOG((LF_SYNC, INFO3, "SetBackground obtain lock\n"));
    ThreadStore::LockThreadStore();

    if (IsDead())
    {
        // This can only happen in a race condition, where the correct thing to do
        // is ignore it.  If it happens without the race condition, we throw an
        // exception.
    }
    else
    if (isBack)
    {
        if (!IsBackground())
        {
            FastInterlockOr((ULONG *) &m_State, TS_Background);

            // unstarted threads don't contribute to the background count
            if (!IsUnstarted())
                g_pThreadStore->m_BackgroundThreadCount++;

            // If we put the main thread into a wait, until only background threads exist,
            // then we make that
            // main thread a background thread.  This cleanly handles the case where it
            // may or may not be one as it enters the wait.

            // One of the components of OtherThreadsComplete() has changed, so check whether
            // we should now exit the EE.
            ThreadStore::CheckForEEShutdown();
        }
    }
    else
    {
        if (IsBackground())
        {
            FastInterlockAnd((ULONG *) &m_State, ~TS_Background);

            // unstarted threads don't contribute to the background count
            if (!IsUnstarted())
                g_pThreadStore->m_BackgroundThreadCount--;

            _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
            _ASSERTE(g_pThreadStore->m_BackgroundThreadCount <= g_pThreadStore->m_ThreadCount);
        }
    }

    ThreadStore::UnlockThreadStore();
}

// When the thread starts running, make sure it is running in the correct apartment
// and context.
BOOL Thread::PrepareApartmentAndContext()
{
    m_ThreadId = ::GetCurrentThreadId();

    return TRUE;
}




//----------------------------------------------------------------------------
//
//    ThreadStore Implementation
//
//----------------------------------------------------------------------------

ThreadStore::ThreadStore()
           : m_StoreState(TSS_Normal),
             m_Crst("ThreadStore", CrstThreadStore),
             m_HashCrst("ThreadDLStore", CrstThreadDomainLocalStore),
             m_ThreadCount(0),
             m_UnstartedThreadCount(0),
             m_BackgroundThreadCount(0),
             m_PendingThreadCount(0),
             m_DeadThreadCount(0),
             m_GuidCreated(FALSE),
             m_HoldingThread(0),
             m_holderthreadid(NULL),
             m_dwIncarnation(0)
{
    m_TerminationEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
    _ASSERTE(m_TerminationEvent != INVALID_HANDLE_VALUE);
}


BOOL ThreadStore::InitThreadStore()
{
    g_pThreadStore = new ThreadStore;

    BOOL fInited = (g_pThreadStore != NULL);
    return fInited;
}







void ThreadStore::LockThreadStore(GCHeap::SUSPEND_REASON reason,
                                  BOOL threadCleanupAllowed)
{
    // There's a nasty problem here.  Once we start shutting down because of a
    // process detach notification, threads are disappearing from under us.  There
    // are a surprising number of cases where the dying thread holds the ThreadStore
    // lock.  For example, the finalizer thread holds this during startup in about
    // 10 of our COM BVTs.
    if (!g_fProcessDetach)
    {
        BOOL gcOnTransitions;

        Thread *pCurThread = GetThread();
        // During ShutDown, the shutdown thread suspends EE. Then it pretends that
        // FinalizerThread is the one to suspend EE.
        // We should allow Finalizer thread to grab ThreadStore lock.
        if (g_fFinalizerRunOnShutDown
            && pCurThread == g_pGCHeap->GetFinalizerThread()) {
            return;
        }

        gcOnTransitions = GC_ON_TRANSITIONS(FALSE);                // dont do GC for GCStress 3

        BOOL toggleGC = (   pCurThread != NULL 
                         && pCurThread->PreemptiveGCDisabled()
                         && reason != GCHeap::SUSPEND_FOR_GC);

        // Note: there is logic in gc.cpp surrounding suspending all
        // runtime threads for a GC that depends on the fact that we
        // do an EnablePreemptiveGC and a DisablePreemptiveGC around
        // taking this lock.
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();

        LOG((LF_SYNC, INFO3, "Locking thread store\n"));

        // Any thread that holds the thread store lock cannot be stopped by unmanaged breakpoints and exceptions when
        // we're doing managed/unmanaged debugging. Calling SetDebugCantStop(true) on the current thread helps us
        // remember that.
        if (pCurThread)
            pCurThread->SetDebugCantStop(true);

        // This is used to avoid thread starvation if non-GC threads are competing for
        // the thread store lock when there is a real GC-thread waiting to get in.
        // This is initialized lazily when the first non-GC thread backs out because of
        // a waiting GC thread.
        if (s_hAbortEvt != NULL &&
            !(reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP) &&
            g_pGCHeap->GetGCThreadAttemptingSuspend() != NULL &&
            g_pGCHeap->GetGCThreadAttemptingSuspend() != pCurThread)
        {
            HANDLE hAbortEvt = s_hAbortEvt;

            if (hAbortEvt != NULL)
            {
                LOG((LF_SYNC, INFO3, "Performing suspend abort wait.\n"));
                WaitForSingleObject(hAbortEvt, INFINITE);
                LOG((LF_SYNC, INFO3, "Release from suspend abort wait.\n"));
            }
        }
    
        g_pThreadStore->Enter();

        _ASSERTE(g_pThreadStore->m_holderthreadid == 0);
        g_pThreadStore->m_holderthreadid = ::GetCurrentThreadId();
        
        LOG((LF_SYNC, INFO3, "Locked thread store\n"));

        // Established after we obtain the lock, so only useful for synchronous tests.
        // A thread attempting to suspend us asynchronously already holds this lock.
        g_pThreadStore->m_HoldingThread = pCurThread;

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        GC_ON_TRANSITIONS(gcOnTransitions);

        //
        // See if there are any detached threads which need cleanup. Only do this on
        // real EE threads.
        //

        if (Thread::m_DetachCount && threadCleanupAllowed && GetThread() != NULL)
            Thread::CleanupDetachedThreads(reason);
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Locking thread store skipped upon detach\n"));
#endif
}

    
void ThreadStore::UnlockThreadStore()
{
    // There's a nasty problem here.  Once we start shutting down because of a
    // process detach notification, threads are disappearing from under us.  There
    // are a surprising number of cases where the dying thread holds the ThreadStore
    // lock.  For example, the finalizer thread holds this during startup in about
    // 10 of our COM BVTs.
    if (!g_fProcessDetach)
    {
        Thread *pCurThread = GetThread();
        // During ShutDown, the shutdown thread suspends EE. Then it pretends that
        // FinalizerThread is the one to suspend EE.
        // We should allow Finalizer thread to grab ThreadStore lock.
        if (g_fFinalizerRunOnShutDown
            && pCurThread == g_pGCHeap->GetFinalizerThread ()) {
            return;
        }
        LOG((LF_SYNC, INFO3, "Unlocking thread store\n"));
        _ASSERTE(GetThread() == NULL || g_pThreadStore->m_HoldingThread == GetThread());

        g_pThreadStore->m_HoldingThread = NULL;
        g_pThreadStore->m_holderthreadid = 0;
        g_pThreadStore->Leave();

        // We're out of the critical area for managed/unmanaged debugging.
        if (pCurThread)
            pCurThread->SetDebugCantStop(false);
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Unlocking thread store skipped upon detach\n"));
#endif
}


void ThreadStore::LockDLSHash()
{
    if (!g_fProcessDetach)
    {
        LOG((LF_SYNC, INFO3, "Locking thread DLS hash\n"));
        g_pThreadStore->EnterDLSHashLock();
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Locking thread DLS hash skipped upon detach\n"));
#endif
}

void ThreadStore::UnlockDLSHash()
{
    if (!g_fProcessDetach)
    {
        LOG((LF_SYNC, INFO3, "Unlocking thread DLS hash\n"));
        g_pThreadStore->LeaveDLSHashLock();
    }

#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Unlocking thread DLS hash skipped upon detach\n"));
#endif
}

// AddThread adds 'newThread' to m_ThreadList
void ThreadStore::AddThread(Thread *newThread)
{
    LOG((LF_SYNC, INFO3, "AddThread obtain lock\n"));

    LockThreadStore();

    g_pThreadStore->m_ThreadList.InsertTail(newThread);
    g_pThreadStore->m_ThreadCount++;
    if (newThread->IsUnstarted())
        g_pThreadStore->m_UnstartedThreadCount++;

    _ASSERTE(!newThread->IsBackground());
    _ASSERTE(!newThread->IsDead());

    g_pThreadStore->m_dwIncarnation++;

    UnlockThreadStore();
}


// Whenever one of the components of OtherThreadsComplete() has changed in the
// correct direction, see whether we can now shutdown the EE because only background
// threads are running.
void ThreadStore::CheckForEEShutdown()
{
    if (g_fWeControlLifetime && g_pThreadStore->OtherThreadsComplete())
    {
#ifdef _DEBUG
        BOOL bRet =
#endif
        ::SetEvent(g_pThreadStore->m_TerminationEvent);
        _ASSERTE(bRet);
    }
}


BOOL ThreadStore::RemoveThread(Thread *target)
{
    BOOL    found;
    Thread *ret;

    _ASSERTE(g_pThreadStore->m_Crst.GetEnterCount() > 0 || g_fProcessDetach);
    _ASSERTE(g_pThreadStore->DbgFindThread(target));
    ret = g_pThreadStore->m_ThreadList.FindAndRemove(target);
    _ASSERTE(ret && ret == target);
    found = (ret != NULL);

    if (found)
    {
        g_pThreadStore->m_ThreadCount--;

        if (target->IsDead())
            g_pThreadStore->m_DeadThreadCount--;

        // Unstarted threads are not in the Background count:
        if (target->IsUnstarted())
            g_pThreadStore->m_UnstartedThreadCount--;
        else
        if (target->IsBackground())
            g_pThreadStore->m_BackgroundThreadCount--;


        _ASSERTE(g_pThreadStore->m_ThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_BackgroundThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_UnstartedThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_DeadThreadCount);

        // One of the components of OtherThreadsComplete() has changed, so check whether
        // we should now exit the EE.
        CheckForEEShutdown();

        g_pThreadStore->m_dwIncarnation++;
    }
    return found;
}


// When a thread is created as unstarted.  Later it may get started, in which case
// someone calls Thread::HasStarted() on that physical thread.  This completes
// the Setup and calls here.
void ThreadStore::TransferStartedThread(Thread *thread)
{
    _ASSERTE(GetThread() == NULL);
    TlsSetValue(gThreadTLSIndex, (VOID*)thread);
    TlsSetValue(gAppDomainTLSIndex, (VOID*)thread->m_pDomain);

    LOG((LF_SYNC, INFO3, "TransferUnstartedThread obtain lock\n"));
    LockThreadStore();

    _ASSERTE(g_pThreadStore->DbgFindThread(thread));
    _ASSERTE(thread->GetThreadHandle() != INVALID_HANDLE_VALUE);
    _ASSERTE(thread->m_State & Thread::TS_WeOwn);
    _ASSERTE(thread->IsUnstarted());
    _ASSERTE(!thread->IsDead());

    // Of course, m_ThreadCount is already correct since it includes started and
    // unstarted threads.

    g_pThreadStore->m_UnstartedThreadCount--;

    // We only count background threads that have been started
    if (thread->IsBackground())
        g_pThreadStore->m_BackgroundThreadCount++;

    _ASSERTE(g_pThreadStore->m_PendingThreadCount > 0);
    FastInterlockDecrement(&g_pThreadStore->m_PendingThreadCount);

    // As soon as we erase this bit, the thread becomes eligible for suspension,
    // stopping, interruption, etc.
    FastInterlockAnd((ULONG *) &thread->m_State, ~Thread::TS_Unstarted);
    FastInterlockOr((ULONG *) &thread->m_State, Thread::TS_LegalToJoin);

    // One of the components of OtherThreadsComplete() has changed, so check whether
    // we should now exit the EE.
    CheckForEEShutdown();

    g_pThreadStore->m_dwIncarnation++;

    UnlockThreadStore();
}


// Access the list of threads.  You must be inside a critical section, otherwise
// the "cursor" thread might disappear underneath you.  Pass in NULL for the
// cursor to begin at the start of the list.
Thread *ThreadStore::GetAllThreadList(Thread *cursor, ULONG mask, ULONG bits)
{
    _ASSERTE(g_pThreadStore->m_Crst.GetEnterCount() > 0 || g_fProcessDetach || g_fRelaxTSLRequirement);

    while (TRUE)
    {
        cursor = (cursor
                  ? g_pThreadStore->m_ThreadList.GetNext(cursor)
                  : g_pThreadStore->m_ThreadList.GetHead());

        if (cursor == NULL)
            break;

        if ((cursor->m_State & mask) == bits)
            return cursor;
    }
    return NULL;
}

// Iterate over the threads that have been started
Thread *ThreadStore::GetThreadList(Thread *cursor)
{
    return GetAllThreadList(cursor, (Thread::TS_Unstarted | Thread::TS_Dead), 0);
}


// We shut down the EE only when all the non-background threads have terminated
// (unless this is an exceptional termination).  So the main thread calls here to
// wait before tearing down the EE.
void ThreadStore::WaitForOtherThreads()
{
    CHECK_ONE_STORE();

    Thread      *pCurThread = GetThread();

    // Regardless of whether the main thread is a background thread or not, force
    // it to be one.  This simplifies our rules for counting non-background threads.
    pCurThread->SetBackground(TRUE);

    LOG((LF_SYNC, INFO3, "WaitForOtherThreads obtain lock\n"));
    LockThreadStore();
    if (!OtherThreadsComplete())
    {
        UnlockThreadStore();

        FastInterlockOr((ULONG *) &pCurThread->m_State, Thread::TS_ReportDead);
#ifdef _DEBUG
        DWORD   ret =
#endif
        pCurThread->DoAppropriateWait(1, &m_TerminationEvent, FALSE, INFINITE, TRUE, NULL);
        _ASSERTE(ret == WAIT_OBJECT_0);
    }
    else
        UnlockThreadStore();
}


// Every EE process can lazily create a GUID that uniquely identifies it (for
// purposes of remoting).
const GUID &ThreadStore::GetUniqueEEId()
{
    if (!m_GuidCreated)
    {
        LockThreadStore();
        if (!m_GuidCreated)
        {
            HRESULT hr = ::CoCreateGuid(&m_EEGuid);

            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
                m_GuidCreated = TRUE;
        }
        UnlockThreadStore();

        if (!m_GuidCreated)
            return IID_NULL;
    }
    return m_EEGuid;
}


DWORD ThreadStore::GetIncarnation()
{
    return g_pThreadStore->m_dwIncarnation;
}


#ifdef _DEBUG
BOOL ThreadStore::DbgFindThread(Thread *target)
{
    CHECK_ONE_STORE();

    // Clear the poisoned flag for g_TrapReturningThreads.
    g_TrapReturningThreadsPoisoned = false;
    
    BOOL    found = FALSE;
    Thread *cur = NULL;
    LONG    cnt = 0;
    LONG    cntBack = 0;
    LONG    cntUnstart = 0;
    LONG    cntDead = 0;
    LONG    cntReturn = 0;

    while ((cur = GetAllThreadList(cur, 0, 0)) != NULL)
    {
        cnt++;

        if (cur->IsDead())
            cntDead++;

        // Unstarted threads do not contribute to the count of background threads
        if (cur->IsUnstarted())
            cntUnstart++;
        else
        if (cur->IsBackground())
            cntBack++;

        if (cur == target)
            found = TRUE;

        // Note that (DebugSuspendPending | SuspendPending) implies a count of 2.
        // We don't count GCPending because a single trap is held for the entire
        // GC, instead of counting each interesting thread.
        if (cur->m_State & Thread::TS_DebugSuspendPending)
            cntReturn++;

        if (cur->m_State & Thread::TS_UserSuspendPending)
            cntReturn++;

        if (cur->m_TraceCallCount > 0)
            cntReturn++;

        if (cur->IsAbortRequested())
            cntReturn++;
    }

    _ASSERTE(cnt == m_ThreadCount);
    _ASSERTE(cntUnstart == m_UnstartedThreadCount);
    _ASSERTE(cntBack == m_BackgroundThreadCount);
    _ASSERTE(cntDead == m_DeadThreadCount);
    _ASSERTE(0 <= m_PendingThreadCount);


    // Because of race conditions and the fact that the GC places its
    // own count, I can't assert this precisely.  But I do want to be
    // sure that this count isn't wandering ever higher -- with a
    // nasty impact on the performance of GC mode changes and method
    // call chaining!
    //
    // We don't bother asserting this during process exit, because
    // during a shutdown we will quietly terminate threads that are
    // being waited on.  (If we aren't shutting down, we carefully
    // decrement our counts and alert anyone waiting for us to
    // return).
    //
    // Note: we don't actually assert this if
    // g_TrapReturningThreadsPoisoned is true. It is set to true when
    // ever a thread bumps g_TrapReturningThreads up, and it is set to
    // false on entry into this routine. Therefore, if its true, it
    // indicates that a thread has bumped the count up while we were
    // counting, which will throw out count off.
        
    _ASSERTE((cntReturn + 2 >= g_TrapReturningThreads) ||
             g_fEEShutDown ||
             g_TrapReturningThreadsPoisoned);
        
    return found;
}

#endif // _DEBUG



//----------------------------------------------------------------------------
//
// Suspending threads, rendezvousing with threads that reach safe places, etc.
//
//----------------------------------------------------------------------------

// A note on SUSPENSIONS.
//
// We must not suspend a thread while it is holding the ThreadStore lock, or
// the lock on the thread.  Why?  Because we need those locks to resume the
// thread (and to perform a GC, use the debugger, spawn or kill threads, etc.)
//
// There are two types of suspension we must consider to enforce the above
// rule.  Synchronous suspensions are where we persuade the thread to suspend
// itself.  This is CommonTripThread and its cousins.  In other words, the
// thread toggles the GC mode, or it hits a hijack, or certain opcodes in the
// interpreter, etc.  In these cases, the thread can simply check whether it
// is holding these locks before it suspends itself.
//
// The other style is an asynchronous suspension.  This is where another
// thread looks to see where we are.  If we are in a fully interruptible region
// of JIT code, we will be left suspended.  In this case, the thread performing
// the suspension must hold the locks on the thread and the threadstore.  This
// ensures that we aren't suspended while we are holding these locks.
//
// Note that in the asynchronous case it's not enough to just inspect the thread
// to see if it's holding these locks.  Since the thread must be in preemptive
// mode to block to acquire these locks, and since there will be a few inst-
// ructions between acquiring the lock and noting in our state that we've
// acquired it, then there would be a window where we would seem eligible for
// suspension -- but in fact would not be.

//----------------------------------------------------------------------------

// We can't leave preemptive mode and enter cooperative mode, if a GC is
// currently in progress.  This is the situation when returning back into
// the EE from outside.  See the comments in DisablePreemptiveGC() to understand
// why we Enable GC here!
void Thread::RareDisablePreemptiveGC()
{
    if ((g_pGCHeap->IsGCInProgress() && (this != g_pGCHeap->GetGCThread())) ||
        (m_State & (TS_UserSuspendPending | TS_DebugSuspendPending)))
    {
        if (!ThreadStore::HoldingThreadStore(this) || g_fRelaxTSLRequirement)
        {
            do
            {
                EnablePreemptiveGC();
            
                // just wait until the GC is over.
                if (this != g_pGCHeap->GetGCThread())
                {
#ifdef PROFILING_SUPPORTED
                    // If profiler desires GC events, notify it that this thread is waiting until the GC is over
                    // Do not send suspend notifications for debugger suspensions
                    if (CORProfilerTrackSuspends() && !(m_State & TS_DebugSuspendPending))
                    {
                        g_profControlBlock.pProfInterface->RuntimeThreadSuspended((ThreadID)this, (ThreadID)this);
                    }
#endif // PROFILING_SUPPORTED


                        // thread -- they had better not be fiberizing something from the threadpool!

                        // First, check to see if there's an IDbgThreadControl interface that needs
                        // notification of the suspension
                        if (m_State & TS_DebugSuspendPending)
                        {
                            IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();

                            if (pDbgThreadControl)
                                pDbgThreadControl->ThreadIsBlockingForDebugger();

                        }

                        // If not, check to see if there's an IGCThreadControl interface that needs
                        // notification of the suspension
                        IGCThreadControl *pGCThreadControl = CorHost::GetGCThreadControl();

                        if (pGCThreadControl)
                            pGCThreadControl->ThreadIsBlockingForSuspension();

                        g_pGCHeap->WaitUntilGCComplete();


#ifdef PROFILING_SUPPORTED
                    // Let the profiler know that this thread is resuming
                    if (CORProfilerTrackSuspends())
                        g_profControlBlock.pProfInterface->RuntimeThreadResumed((ThreadID)this, (ThreadID)this);
#endif // PROFILING_SUPPORTED
                }
    
                m_fPreemptiveGCDisabled = 1;

                // The fact that we check whether 'this' is the GC thread may seem
                // strange.  After all, we determined this before entering the method.
                // However, it is possible for the current thread to become the GC
                // thread while in this loop.  This happens if you use the COM+
                // debugger to suspend this thread and then release it.

            } while ((g_pGCHeap->IsGCInProgress() && (this != g_pGCHeap->GetGCThread())) ||
                     (m_State & (TS_UserSuspendPending | TS_DebugSuspendPending)));
        }
    }
}

void Thread::HandleThreadAbort ()
{
    // Sometimes we call this without any CLR SEH in place.  An example is UMThunkStubRareDisableWorker.
    // That's okay since COMPlusThrow will eventually erect SEH around the RaiseException,
    // but it prevents us from stating THROWSCOMPLUSEXCEPTION here.
    //THROWSCOMPLUSEXCEPTION();
    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;

    if ((m_State & TS_AbortRequested) && 
        !(m_State & TS_AbortInitiated) &&
        (! IsExceptionInProgress() || m_handlerInfo.IsInUnmanagedHandler()))
    { // generate either a ThreadAbort exception
        LOG((LF_APPDOMAIN, LL_INFO100, "Thread::HandleThreadAbort throwing abort for %x\n", GetThreadId()));
        SetAbortInitiated();
        ResetStopRequest();
        // if an abort and interrupt happen at the same time (e.g. on a sleeping thread),
        // the abort is favored. But we do need to reset the interrupt bits. 
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));
        IsUserInterrupted(TRUE /*=reset*/);
        COMPlusThrow(kThreadAbortException);
    }    
}

#ifdef _DEBUG
#define MAXSTACKBYTES (0x800*sizeof(PVOID))             // two pages
void CleanStackForFastGCStress ()
{
    size_t nBytes = MAXSTACKBYTES;
    nBytes &= ~sizeof (size_t);
    if (nBytes > MAXSTACKBYTES) {
        nBytes = MAXSTACKBYTES;
    }
    size_t* buffer = (size_t*) _alloca (nBytes);
    memset(buffer, 0, nBytes);
    GetThread()->m_pCleanedStackBase = &nBytes;
}

void Thread::ObjectRefFlush(Thread* thread) {
    _ASSERTE(thread->PreemptiveGCDisabled());  // Should have been in managed code     
    memset(thread->dangerousObjRefs, 0, sizeof(thread->dangerousObjRefs));
    CLEANSTACKFORFASTGCSTRESS ();
}
#endif

#if defined(STRESS_HEAP)

PtrHashMap *g_pUniqueStackMap = NULL;
Crst *g_pUniqueStackCrst = NULL;

#define UniqueStackDepth 8

BOOL StackCompare (UPTR val1, UPTR val2)
{
    size_t *p1 = (size_t *)(val1 << 1);
    size_t *p2 = (size_t *)val2;
    if (p1[0] != p2[0]) {
        return FALSE;
    }
    size_t nElem = p1[0];
    if (nElem >= UniqueStackDepth) {
        nElem = UniqueStackDepth;
    }
    p1 ++;
    p2 ++;

    for (size_t n = 0; n < nElem; n ++) {
        if (p1[n] != p2[n]) {
            return FALSE;
        }
    }

    return TRUE;
}

void StartUniqueStackMap ()
{
    static volatile LONG fUniqueStackInit = 0;

    if (FastInterlockExchange (&fUniqueStackInit, 1) == 0) {
        _ASSERTE (g_pUniqueStackMap == NULL);
        g_pUniqueStackCrst = ::new Crst ("HashMap", CrstUniqueStack, TRUE, FALSE);
        PtrHashMap *map = new (SystemDomain::System()->GetLowFrequencyHeap()) PtrHashMap ();
        LockOwner lock = {g_pUniqueStackCrst, IsOwnerOfCrst};
        map->Init (32, StackCompare, TRUE, &lock);
        g_pUniqueStackMap = map;
    }
    else
    {
        while (g_pUniqueStackMap == NULL) {
            __SwitchToThread (0);
        }
    }
}



#if defined(_DEBUG)

// This function is for GC stress testing.  Before we enable preemptive GC, let us do a GC
// because GC may happen while the thread is in preemptive GC mode.
void Thread::PerformPreemptiveGC()
{
    if (g_fProcessDetach)
        return;
    
    if (!(g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION))
        return;

    if (!m_GCOnTransitionsOK
        || GCForbidden() 
        || g_fEEShutDown 
        || g_pGCHeap->IsGCInProgress() 
        || GCHeap::GetGcCount() == 0    // Need something that works for isolated heap.
        || ThreadStore::HoldingThreadStore()) 
        return;
    
#ifdef DEBUGGING_SUPPORTED
    // Don't collect if the debugger is attach and either 1) there
    // are any threads held at unsafe places or 2) this thread is
    // under the control of the debugger's dispatch logic (as
    // evidenced by having a non-NULL filter context.)
    if ((CORDebuggerAttached() &&
        (g_pDebugInterface->ThreadsAtUnsafePlaces() ||
        (GetFilterContext() != NULL)))) 
        return;
#endif // DEBUGGING_SUPPORTED

    _ASSERTE(m_fPreemptiveGCDisabled == false);     // we are in preemtive mode when we call this
    
    m_GCOnTransitionsOK = FALSE;
    DisablePreemptiveGC();
    g_pGCHeap->StressHeap();
    EnablePreemptiveGC();
    m_GCOnTransitionsOK = TRUE; 
}
#endif  // DEBUG
#endif // STRESS_HEAP

// To leave cooperative mode and enter preemptive mode, if a GC is in progress, we
// no longer care to suspend this thread.  But if we are trying to suspend the thread
// for other reasons (e.g. Thread.Suspend()), now is a good time.
//
// Note that it is possible for an N/Direct call to leave the EE without explicitly
// enabling preemptive GC.
void Thread::RareEnablePreemptiveGC()
{
#if defined(STRESS_HEAP) && defined(_DEBUG)
    if (!IsDetached())
        PerformPreemptiveGC();
#endif

    if (!ThreadStore::HoldingThreadStore(this) || g_fRelaxTSLRequirement)
    {

        // wake up any threads waiting to suspend us, like the GC thread.
        SetSafeEvent();

        // for GC, the fact that we are leaving the EE means that it no longer needs to
        // suspend us.  But if we are doing a non-GC suspend, we need to block now.
        // Give the debugger precedence over user suspensions:
        while (m_State & (TS_DebugSuspendPending | TS_UserSuspendPending))
        {
            BOOL threadStoreLockOwner = FALSE;
            
#ifdef DEBUGGING_SUPPORTED
            if (m_State & TS_DebugWillSync)
            {
                _ASSERTE(m_State & TS_DebugSuspendPending);

                FastInterlockAnd((ULONG *) &m_State, ~TS_DebugWillSync);

                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: sync reached.\n", m_ThreadId));

                if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
                {
                    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: complete.\n", m_ThreadId));

                    // We need to know if this thread is going to be blocking while holding the thread store lock
                    // below. If that's the case, we'll actually wake the thread up in SysResumeFromDebug even though is
                    // is supposed to be suspended. (See comments in SysResumeFromDebug.)
                    SetThreadStateNC(TSNC_DebuggerUserSuspendSpecial);
                    
                    threadStoreLockOwner = g_pDebugInterface->SuspendComplete(FALSE);

                    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: owns TS: %d\n", m_ThreadId, threadStoreLockOwner));
                }
            }
            
            // Check to see if there's an IDbgThreadControl interface that needs
            // notification of the suspension
            if (m_State & TS_DebugSuspendPending)
            {
                IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();

                if (pDbgThreadControl)
                    pDbgThreadControl->ThreadIsBlockingForDebugger();

            }
#endif // DEBUGGING_SUPPORTED

#ifdef LOGGING
            if (!CorHost::IsDebuggerSpecialThread(m_ThreadId))
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: suspended while enabling gc.\n", m_ThreadId));

            else
                LOG((LF_CORDB, LL_INFO1000,
                     "[0x%x] ALERT: debugger special thread did not suspend while enabling gc.\n", m_ThreadId));
#endif

            WaitSuspendEvent(); // sets bits, too

            // We no longer have to worry about this thread blocking with the thread store lock held, so remove the
            // bit. (Again, see comments in SysResumeFromDebug.)
            ResetThreadStateNC(TSNC_DebuggerUserSuspendSpecial);

            // If we're the holder of the thread store lock after a SuspendComplete from above, then release the thread
            // store lock here. We're releasing after waiting, which means this thread holds the thread store lock the
            // entire time the Runtime is stopped.
            if (threadStoreLockOwner)
            {
                // If this thread is marked as debugger suspended and its the holder of the thread store lock, then
                // clear the suspend event and let us loop around to block again, this time without the thread store
                // lock held. This ensures that if this thread is marked by the debugger as suspended (while the runtime
                // is stopped), that it will release the thread store lock when the process is resumed but still
                // continue waiting.
                if (m_StateNC & TSNC_DebuggerUserSuspend)
                {
                    // We can assert this because we're holding the thread store lock, so we know that no one can reset
                    // this flag on us.
                    _ASSERTE(m_State & TS_DebugSuspendPending);
                    
                    ClearSuspendEvent();
                }

                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: releasing thread store lock.\n", m_ThreadId));

                ThreadStore::UnlockThreadStore();
            }
            else
            {
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: not releasing thread store lock.\n", m_ThreadId));
            }
        }
    }
}


// Called out of CommonTripThread, we are passing through a Safe spot.  Do the right
// thing with this thread.  This may involve waiting for the GC to complete, or
// performing a pending suspension.
void Thread::PulseGCMode()
{
    _ASSERTE(this == GetThread());

    if (PreemptiveGCDisabled() && CatchAtSafePoint())
    {
        EnablePreemptiveGC();
        DisablePreemptiveGC();
    }
}


// Indicate whether threads should be trapped when returning to the EE (i.e. disabling
// preemptive GC mode)
void ThreadStore::TrapReturningThreads(BOOL yes)
{
    if (yes)
    {
#ifdef _DEBUG
        g_TrapReturningThreadsPoisoned = true;
#endif
        
        FastInterlockIncrement(&g_TrapReturningThreads);
        _ASSERTE(g_TrapReturningThreads > 0);
    }
    else
    {
        FastInterlockDecrement(&g_TrapReturningThreads);
        _ASSERTE(g_TrapReturningThreads >= 0);
    }
}


// Grab a consistent snapshot of the thread's state, for reporting purposes only.
Thread::ThreadState Thread::GetSnapshotState()
{
    ThreadState     res = m_State;

    if (res & TS_ReportDead)
        res = (ThreadState) (res | TS_Dead);

    return res;
}




//************************************************************************************
// The basic idea is to make a first pass while the threads are suspended at the OS
// level.  This pass marks each thread to indicate that it is requested to get to a
// safe spot.  Then the threads are resumed.  In a second pass, we actually wait for
// the threads to get to their safe spot and rendezvous with us.
HRESULT Thread::SysSuspendForGC(GCHeap::SUSPEND_REASON reason)
{
    Thread  *pCurThread = GetThread();
    Thread  *thread = NULL;
    LONG     countThreads = 0;
    LONG     iCount = 0, i;
    HANDLE   ThreadEventArray[MAX_WAIT_OBJECTS];
    Thread  *ThreadArray[MAX_WAIT_OBJECTS];
    DWORD    res;

    // Caller is expected to be holding the ThreadStore lock.  Also, caller must
    // have set GcInProgress before coming here, or things will break;
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach);
    _ASSERTE(g_pGCHeap->IsGCInProgress());

#ifdef PROFILING_SUPPORTED
    // If the profiler desires information about GCs, then let it know that one
    // is starting.
    if (CORProfilerTrackSuspends())
    {
        _ASSERTE(reason != GCHeap::SUSPEND_FOR_DEBUGGER);

        g_profControlBlock.pProfInterface->RuntimeSuspendStarted(
            (COR_PRF_SUSPEND_REASON)reason,
            (ThreadID)pCurThread);

        if (pCurThread)
        {
            // Notify the profiler that the thread that is actually doing the GC is 'suspended',
            // meaning that it is doing stuff other than run the managed code it was before the
            // GC started.
            g_profControlBlock.pProfInterface->RuntimeThreadSuspended((ThreadID)pCurThread, (ThreadID)pCurThread);
        }        
    }
#endif // PROFILING_SUPPORTED

    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysStartSuspendForDebug.  Please make
    // sure to make appropriate changes there as well.

    if (pCurThread)     // concurrent GC occurs on threads we don't know about
    {
        _ASSERTE(pCurThread->m_Priority == INVALID_THREAD_PRIORITY);
        DWORD priority = GetThreadPriority(pCurThread->GetThreadHandle());
        if (priority < THREAD_PRIORITY_NORMAL)
        {
            pCurThread->m_Priority = priority;
            SetThreadPriority(pCurThread->GetThreadHandle(),THREAD_PRIORITY_NORMAL);
        }
    }
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        if (thread == pCurThread)
            continue;
        
        // Nothing confusing left over from last time.
        _ASSERTE((thread->m_State & TS_GCSuspendPending) == 0);

        // Threads can be in Preemptive or Cooperative GC mode.  Threads cannot switch
        // to Cooperative mode without special treatment when a GC is happening.
        if (thread->m_fPreemptiveGCDisabled)
        {
            // Check a little more carefully.  Threads might sneak out without telling
            // us, because we haven't marked them, or because of inlined N/Direct.
            DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

            if (thread->m_fPreemptiveGCDisabled)
            {

                // We clear the event here, and we'll set it in any of our
                // rendezvous points when the thread is ready for us.
                //
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                //
                // NOTE: we do this even if we've failed to suspend the thread above!
                // This ensures that we wait for the thread below.
                //
                if ((thread->m_State & TS_UserSuspendPending) == 0)
                    thread->ClearSafeEvent();

                FastInterlockOr((ULONG *) &thread->m_State, TS_GCSuspendPending);

                countThreads++;

                // Only resume if we actually suspended the thread above.
                if (dwSuspendCount != (DWORD) -1)
                    ::ResumeThread(thread->GetThreadHandle());
            }
            else if (dwSuspendCount != (DWORD) -1)
            {
                // Oops.
                ::ResumeThread(thread->GetThreadHandle());
            }
        }
    }

#ifdef _DEBUG

    {
        int     countCheck = 0;
        Thread *InnerThread = NULL;

        while ((InnerThread = ThreadStore::GetThreadList(InnerThread)) != NULL)
        {
            if (InnerThread != pCurThread &&
                (InnerThread->m_State & TS_GCSuspendPending) != 0)
            {
                countCheck++;
            }
        }
        _ASSERTE(countCheck == countThreads);
    }

#endif

    // Pass 2: Whip through the list again.

    _ASSERTE(thread == NULL);

    while (countThreads)
    {
        thread = ThreadStore::GetThreadList(thread);

        if (thread == pCurThread)
            continue;

        if ((thread->m_State & TS_GCSuspendPending) == 0)
            continue;

        if (thread->m_fPreemptiveGCDisabled)
        {
            ThreadArray[iCount] = thread;
            ThreadEventArray[iCount] = thread->m_SafeEvent;
            iCount++;
        }
        else
        {
            // Inlined N/Direct can sneak out to preemptive without actually checking.
            // If we find one, we can consider it suspended (since it can't get back in).
            countThreads--;
        }

        if ((iCount >= MAX_WAIT_OBJECTS) || (iCount == countThreads))
        {
#ifdef _DEBUG
            DWORD dbgTotalTimeout = 0;
#endif
            while (iCount)
            {
                // If another thread is trying to do a GC, there is a chance of deadlock
                // because this thread holds the threadstore lock and the GC thread is stuck
                // trying to get it, so this thread must bail and do a retry after the GC completes.
                if (g_pGCHeap->GetGCThreadAttemptingSuspend() != NULL && g_pGCHeap->GetGCThreadAttemptingSuspend() != pCurThread)
                {
#ifdef PROFILING_SUPPORTED
                    // Must let the profiler know that this thread is aborting it's attempt at suspending
                    if (CORProfilerTrackSuspends())
                    {
                        g_profControlBlock.pProfInterface->RuntimeSuspendAborted((ThreadID)thread);                            
                    }
#endif // PROFILING_SUPPORTED

                    return (ERROR_TIMEOUT);
                }

                res = ::WaitForMultipleObjects(iCount, ThreadEventArray,
                                               FALSE /*Any one is fine*/, PING_JIT_TIMEOUT);

                if (res == WAIT_TIMEOUT || res == WAIT_IO_COMPLETION)
                {
#ifdef _DEBUG
                    if ((dbgTotalTimeout += PING_JIT_TIMEOUT) > DETECT_DEADLOCK_TIMEOUT)
                    {
                        // Do not change this to _ASSERTE.
                        // We want to catch the state of the machine at the
                        // time when we can not suspend some threads.
                        // It takes too long for _ASSERTE to stop the process.
                        DebugBreak();
                        _ASSERTE(!"Timed out trying to suspend EE due to thread");
                        char message[256];
                        for (int i = 0; i < iCount; i ++)
                        {
                            sprintf (message, "Thread %x cannot be suspended",
                                     ThreadArray[i]->GetThreadId());
                            DbgAssertDialog(__FILE__, __LINE__, message);
                        }
                    }
#endif
                    // all these threads should be in cooperative mode unless they have
                    // set their SafeEvent on the way out.  But there's a race between
                    // when we time out and when they toggle their mode, so sometimes
                    // we will suspend a thread that has just left.
                    for (i=0; i<iCount; i++)
                    {
                        Thread  *InnerThread;

                        InnerThread = ThreadArray[i];

                        // If the thread is gone, do not wait for it.
                        if (res == WAIT_TIMEOUT)
                        {
                            if (WaitForSingleObject (InnerThread->GetThreadHandle(), 0)
                                != WAIT_TIMEOUT)
                            {
                                // The thread is not there.
                                iCount--;
                                countThreads--;
                                
                                ThreadEventArray[i] = ThreadEventArray[iCount];
                                ThreadArray[i] = ThreadArray[iCount];
                                continue;
                            }
                        }

                        DWORD dwSuspendCount = ::SuspendThread(InnerThread->GetThreadHandle());


                        // If the thread was redirected, then keep track of it like any other
                        // thread that's in cooperative mode that can't be suspended.  It will
                        // eventually go into preemptive mode which will allow the runtime
                        // suspend to complete.
                        if (!InnerThread->m_fPreemptiveGCDisabled)
                        {
                            iCount--;
                            countThreads--;

                            ThreadEventArray[i] = ThreadEventArray[iCount];
                            ThreadArray[i] = ThreadArray[iCount];
                        }

                        // Whether in cooperative mode & stubborn, or now in
                        // preemptive mode because of inlined N/Direct, let this
                        // thread go.
                        if (dwSuspendCount != (DWORD) -1)
                            ::ResumeThread(InnerThread->GetThreadHandle());
                    }
                }
                else
                if ((res >= WAIT_OBJECT_0) && (res < WAIT_OBJECT_0 + (DWORD)iCount))
                {
                    // A dying thread will signal us here, too.
                    iCount--;
                    countThreads--;

                    ThreadEventArray[res] = ThreadEventArray[iCount];
                    ThreadArray[res] = ThreadArray[iCount];
                }
                else
                {
                    // No WAIT_FAILED, WAIT_ABANDONED, etc.
                    _ASSERTE(!"unexpected wait termination during gc suspension");
                }
            }
        }
    }


    // Alert the host that a GC is starting, in case the host is scheduling threads
    // for non-runtime tasks during GC.
    IGCThreadControl    *pGCThreadControl = CorHost::GetGCThreadControl();

    if (pGCThreadControl)
        pGCThreadControl->SuspensionStarting();

#ifdef PROFILING_SUPPORTED
    // If a profiler is keeping track of GC events, notify it
    if (CORProfilerTrackSuspends())
        g_profControlBlock.pProfInterface->RuntimeSuspendFinished((ThreadID)pCurThread);
#endif // PROFILING_SUPPORTED

#ifdef _DEBUG
    if (reason == GCHeap::SUSPEND_FOR_GC) {
        thread = NULL;
        while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
        {
            thread->DisableStressHeap();
        }
    }
#endif

    return S_OK;
}

#ifdef _DEBUG
void EnableStressHeapHelper()
{
    ENABLESTRESSHEAP();
}
#endif

// We're done with our GC.  Let all the threads run again
void Thread::SysResumeFromGC(BOOL bFinishedGC, BOOL SuspendSucceded)
{
    Thread  *thread = NULL;
    Thread  *pCurThread = GetThread();

#ifdef PROFILING_SUPPORTED
    // If a profiler is keeping track suspend events, notify it

    if (CORProfilerTrackSuspends())
    {
        g_profControlBlock.pProfInterface->RuntimeResumeStarted((ThreadID)pCurThread);
    }
#endif // PROFILING_SUPPORTED

    // Caller is expected to be holding the ThreadStore lock.  But they must have
    // reset GcInProgress, or threads will continue to suspend themselves and won't
    // be resumed until the next GC.
    _ASSERTE(ThreadStore::HoldingThreadStore());
    _ASSERTE(!g_pGCHeap->IsGCInProgress());

    // Alert the host that a GC is ending, in case the host is scheduling threads
    // for non-runtime tasks during GC.
    IGCThreadControl    *pGCThreadControl = CorHost::GetGCThreadControl();

    if (pGCThreadControl)
    {
        // If we the suspension was for a GC, tell the host what generation GC.
        DWORD   Generation = (bFinishedGC
                              ? g_pGCHeap->GetCondemnedGeneration()
                              : ~0U);

        pGCThreadControl->SuspensionEnding(Generation);
    }
    
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {

        FastInterlockAnd((ULONG *) &thread->m_State, ~TS_GCSuspendPending);
    }

#ifdef PROFILING_SUPPORTED
    // Need to give resume event for the GC thread
    if (CORProfilerTrackSuspends())
    {
        if (pCurThread)
        {
            g_profControlBlock.pProfInterface->RuntimeThreadResumed(
                (ThreadID)pCurThread, (ThreadID)pCurThread);
        }

        // If a profiler is keeping track suspend events, notify it
        if (CORProfilerTrackSuspends())
        {
            g_profControlBlock.pProfInterface->RuntimeResumeFinished((ThreadID)pCurThread);
        }
    }

    g_profControlBlock.inprocState = ProfControlBlock::INPROC_PERMITTED;
#endif // PROFILING_SUPPORTED
    
    ThreadStore::UnlockThreadStore();

    if (pCurThread)
    {
        if (pCurThread->m_Priority != INVALID_THREAD_PRIORITY)
        {
            SetThreadPriority(pCurThread->GetThreadHandle(),pCurThread->m_Priority);
            pCurThread->m_Priority = INVALID_THREAD_PRIORITY;
        }

    }
}


// Resume a thread at this location, to persuade it to throw a ThreadStop.  The
// exception handler needs a reasonable idea of how large this method is, so don't
// add lots of arbitrary code here.
void
ThrowControlForThread()
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(pThread->m_OSContext);

    FaultingExceptionFrame fef;
    fef.InitAndLink(pThread->m_OSContext);

#ifdef _X86_
    CalleeSavedRegisters *pRegs = fef.GetCalleeSavedRegisters();
    pRegs->edi = 0;     // Enregisters roots need to be nuked ... this may not have been a gc-safe
    pRegs->esi = 0;     // point.
    pRegs->ebx = 0;
#elif defined(_PPC_)
    CalleeSavedRegisters *pRegs = fef.GetCalleeSavedRegisters();
    ZeroMemory(&(pRegs->r[0]), sizeof(INT32)*NUM_CALLEESAVED_REGISTERS);
#else
    PORTABILITY_ASSERT("ThrowControlForThread");
#endif

    // Here we raise an exception.
    RaiseException(EXCEPTION_COMPLUS,
                   0, 
                   0,
                   NULL);
}


//****************************************************************************
//
//****************************************************************************
bool Thread::SysStartSuspendForDebug(AppDomain *pAppDomain)
{
    Thread  *pCurThread = GetThread();
    Thread  *thread = NULL;

    if (g_fProcessDetach)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "SUSPEND: skipping suspend due to process detach.\n"));
        return true;
    }

    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: starting suspend.  Trap count: %d\n",
         pCurThread ? pCurThread->m_ThreadId : (DWORD) -1, g_TrapReturningThreads)); 

    // Caller is expected to be holding the ThreadStore lock
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach);

    // If there is a debugging thread control object, tell it we're suspending the Runtime.
    IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();
    
    if (pDbgThreadControl)
        pDbgThreadControl->StartBlockingForDebugger(0);
    
    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysSuspendForGC.  Please make
    // sure to make appropriate changes there as well.

    _ASSERTE(m_DebugWillSyncCount == -1);
    m_DebugWillSyncCount++;
    
    
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
    
        // Don't try to suspend threads that you've left suspended.
        if (thread->m_StateNC & TSNC_DebuggerUserSuspend)
            continue;
        
        if (thread == pCurThread)
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "[0x%x] SUSPEND: marking current thread.\n",
                 thread->m_ThreadId));

            _ASSERTE(!thread->m_fPreemptiveGCDisabled);
            
            // Mark this thread so it trips when it tries to re-enter
            // after completing this call.
            thread->ClearSuspendEvent();
            thread->MarkForSuspension(TS_DebugSuspendPending);
            
            continue;
        }

        // Threads can be in Preemptive or Cooperative GC mode.
        // Threads cannot switch to Cooperative mode without special
        // treatment when a GC is happening.  But they can certainly
        // switch back and forth during a debug suspension -- until we
        // can get their Pending bit set.
        DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

        if (thread->m_fPreemptiveGCDisabled && dwSuspendCount != (DWORD) -1)
        {


            // When the thread reaches a safe place, it will wait
            // on the SuspendEvent which clients can set when they
            // want to release us.
            thread->ClearSuspendEvent();

            // Remember that this thread will be running to a safe point
            FastInterlockIncrement(&m_DebugWillSyncCount);
            thread->MarkForSuspension(TS_DebugSuspendPending |
                                      TS_DebugWillSync);

            // Resume the thread and let it run to a safe point
            ::ResumeThread(thread->GetThreadHandle());

            LOG((LF_CORDB, LL_INFO1000, 
                 "[0x%x] SUSPEND: gc disabled - will sync.\n",
                 thread->m_ThreadId));
        }
        else if (dwSuspendCount != (DWORD) -1)
        {
            // Mark threads that are outside the Runtime so that if
            // they attempt to re-enter they will trip.
            thread->ClearSuspendEvent();
            thread->MarkForSuspension(TS_DebugSuspendPending);

            ::ResumeThread(thread->GetThreadHandle());

            LOG((LF_CORDB, LL_INFO1000,
                 "[0x%x] SUSPEND: gc enabled.\n", thread->m_ThreadId));
        }
    }

    //
    // Return true if all threads are synchronized now, otherwise the
    // debugge must wait for the SuspendComplete, called from the last
    // thread to sync.
    //

    if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "SUSPEND: all threads sync before return.\n")); 
        return true;
    }
    else
        return false;
}

//
// This method is called by the debugger helper thread when it times out waiting for a set of threads to
// synchronize. Its used to chase down threads that are not syncronizing quickly. It returns true if all the threads are
// now synchronized and we sent a sync compelte event up. This also means that we own the thread store lock.
//
// If forceSync is true, then we're going to force any thread that isn't at a safe place to stop anyway, thus completing
// the sync. This is used when we've been waiting too long while in interop debugging mode to force a sync in the face
// of deadlocks.
//
bool Thread::SysSweepThreadsForDebug(bool forceSync)
{
    Thread *thread = NULL;

    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysSuspendForGC.  Please make
    // sure to make appropriate changes there as well.

    // This must be called from the debugger helper thread.
    _ASSERTE(dbgOnly_IsSpecialEEThread());

    ThreadStore::LockThreadStore(GCHeap::SUSPEND_FOR_DEBUGGER, FALSE);

    // Loop over the threads...
    while (((thread = ThreadStore::GetThreadList(thread)) != NULL) && (m_DebugWillSyncCount >= 0))
    {
        // Skip threads that we aren't waiting for to sync.
        if ((thread->m_State & TS_DebugWillSync) == 0)
            continue;

        // Suspend the thread
        DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

        if (dwSuspendCount == (DWORD) -1)
        {
            // If the thread has gone, we can't wait on it.
            if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
                // We own the thread store lock. We return true now, which indicates this to the caller.
                return true;
            continue;
        }


        // If the thread isn't at a safe place now, and if we're forcing a sync, then we mark the thread that we're
        // leaving it at a potentially bad place and leave it suspended.
        if (forceSync)
        {
            // Remove the will sync bit and mark that we're leaving it suspended specially.
            thread->UnmarkForSuspension(~TS_DebugSuspendPending);
            thread->SetSuspendEvent();
            FastInterlockAnd((ULONG *) &thread->m_State, ~(TS_DebugWillSync));

            // Note: we're adding bits into m_stateNC. The only reason we can do this is because we know the following:
            // 1) only the thread in question will modify bits besides this routine and SysResumeFromDebug. 2) the
            // thread is suspended and it will remain suspended until we remove these bits. This ensures that even if we
            // suspend this thread while its attempting to modify these bits that no bits will be lost.  Note also that
            // we mark that the thread is stopped in Runtime impl.
            thread->SetThreadStateNC(Thread::TSNC_DebuggerForceStopped);
            thread->SetThreadStateNC(Thread::TSNC_DebuggerStoppedInRuntime);
            
            LOG((LF_CORDB, LL_INFO1000, "Suspend Complete due to Force Sync case for tid=0x%x.\n", thread->m_ThreadId));
        }
        else
        {
            // If we didn't take the thread out of the set then resume it and give it another chance to reach a safe
            // point.
            ::ResumeThread(thread->GetThreadHandle());
            continue;
        }
        
        // Decrement the sync count. If we have no more threads to wait for, then we're done, so send SuspendComplete.
        if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
        {
            // We own the thread store lock. We return true now, which indicates this to the caller.
            return true;
        }
    }

    ThreadStore::UnlockThreadStore();

    return false;
}

void Thread::SysResumeFromDebug(AppDomain *pAppDomain)
{
    Thread  *thread = NULL;

    if (g_fProcessDetach)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "RESUME: skipping resume due to process detach.\n"));
        return;
    }

    LOG((LF_CORDB, LL_INFO1000, "RESUME: starting resume AD:0x%x.\n", pAppDomain)); 

    // Notify the client that it should release any threads that it had doing work
    // while the runtime was debugger-suspended.
    IDebuggerThreadControl *pIDTC = CorHost::GetDebuggerThreadControl();
    if (pIDTC)
    {
        LOG((LF_CORDB, LL_INFO1000, "RESUME: notifying IDebuggerThreadControl client.\n")); 
        pIDTC->ReleaseAllRuntimeThreads();
    }

    // Make sure we completed the previous sync
    _ASSERTE(m_DebugWillSyncCount == -1);

    // Caller is expected to be holding the ThreadStore lock
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach || g_fRelaxTSLRequirement);

    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        // Only consider resuming threads if they're in the correct appdomain
        if (pAppDomain != NULL && thread->GetDomain() != pAppDomain)
        {
            LOG((LF_CORDB, LL_INFO1000, "RESUME: Not resuming thread 0x%x, since it's "
                "in appdomain 0x%x.\n", thread, pAppDomain)); 
            continue;
        }
    
        // If the user wants to keep the thread suspended, then
        // don't release the thread.
        if (!(thread->m_StateNC & TSNC_DebuggerUserSuspend))
        {
            // If we are still trying to suspend this thread, forget about it.
            if (thread->m_State & TS_DebugSuspendPending)
            {
                LOG((LF_CORDB, LL_INFO1000,
                     "[0x%x] RESUME: TS_DebugSuspendPending was set, but will be removed\n",
                     thread->m_ThreadId));

                // Note: we unmark for suspension _then_ set the suspend event.
                thread->UnmarkForSuspension(~TS_DebugSuspendPending);
                thread->SetSuspendEvent();
            }

            // If this thread was forced to stop with PGC disabled then resume it.
            if (thread->m_StateNC & TSNC_DebuggerForceStopped)
            {
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] RESUME: resuming force sync suspension.\n", thread->m_ThreadId));

                thread->ResetThreadStateNC(Thread::TSNC_DebuggerForceStopped);
                thread->ResetThreadStateNC(Thread::TSNC_DebuggerStoppedInRuntime);

                // Don't go through ResumeUnderControl.  If the thread is single-stepping
                // in managed code, we don't want it to suddenly be executing code in our
                // ThrowControl method.
                ::ResumeThread(thread->GetThreadHandle());
            }
        }
        else
        {
            // Thread will remain suspended due to a request from the debugger.
            
            LOG((LF_CORDB,LL_INFO10000,"Didn't unsuspend thread 0x%x"
                "(ID:0x%x)\n", thread, thread->GetThreadId()));
            LOG((LF_CORDB,LL_INFO10000,"Suspending:0x%x\n",
                thread->m_State & TS_DebugSuspendPending));
            _ASSERTE((thread->m_State & TS_DebugWillSync) == 0);

            // If the thread holds the thread store lock and is blocking in RareEnablePreemptiveGC, then we have to wake
            // the thread up and let it release the lock. If we don't do this, then we leave a thread suspended while
            // holding the thread store lock and we can't stop the runtime again later.
            if ((thread->m_State & TS_DebugSuspendPending) &&
                (g_pThreadStore->m_HoldingThread == thread) &&
                (thread->m_StateNC & TSNC_DebuggerUserSuspendSpecial))
                thread->SetSuspendEvent();
        }
    }

    LOG((LF_CORDB, LL_INFO1000, "RESUME: resume complete. Trap count: %d\n", g_TrapReturningThreads)); 
}


// Suspend a thread at the system level.  We distinguish between user suspensions,
// and system suspensions so that a VB program cannot resume a thread we have
// suspended for GC.
//
// This service won't return until the suspension is complete.  This deserves some
// explanation.  The thread is considered to be suspended if it can make no further
// progress within the EE.  For example, a thread that has exited the EE via
// COM Interop or N/Direct is considered suspended -- if we've arranged it so that
// the thread cannot return back to the EE without blocking.
void Thread::UserSuspendThread()
{
    BOOL    mustUnlock = TRUE;

    // Read the general comments on thread suspension earlier, to understand why we
    // take these locks.

    // GC can occur in here:
    LOG((LF_SYNC, INFO3, "UserSuspendThread obtain lock\n"));
    ThreadStore::LockThreadStore();

    // User suspensions (e.g. from VB and C#) are distinguished from internal
    // suspensions so a poorly behaved program cannot resume a thread that the system
    // has suspended for GC.
    if (m_State & TS_UserSuspendPending)
    {
        // This thread is already experiencing a user suspension, so ignore the
        // new request.
        _ASSERTE(!ThreadStore::HoldingThreadStore(this));
    }
    else
    if (this != GetThread())
    {
        // First suspension of a thread other than the current one.
        if (m_State & TS_Unstarted)
        {
            // There is an important window in here.  T1 can call T2.Start() and then
            // T2.Suspend().  Suspend is disallowed on an unstarted thread.  But from T1's
            // point of view, T2 is started.  In reality, T2 hasn't been scheduled by the
            // OS, so it is still an unstarted thread.  We don't want to perform a normal
            // suspension on it in this case, because it is currently contributing to the
            // PendingThreadCount.  We want to get it fully started before we suspend it.
            // This is particularly important if its background status is changing
            // underneath us because otherwise we might not detect that the process should
            // be exited at the right time.
            //
            // It turns out that this is a simple situation to implement.  We are holding
            // the ThreadStoreLock.  TransferStartedThread will likewise acquire that
            // lock.  So if we detect it, we simply set a bit telling the thread to
            // suspend itself.  This is NOT the normal suspension request because we don't
            // want the thread to suspend until it has fully started.
            FastInterlockOr((ULONG *) &m_State, TS_SuspendUnstarted);
        }
        else
        {
            // Pause it so we can operate on it without it squirming under us.
            DWORD dwSuspendCount = ::SuspendThread(GetThreadHandle());

            // The only safe place to suspend a thread asynchronously is if it is in
            // fully interruptible cooperative JIT code.  Preemptive mode can hold all
            // kinds of locks that make it unsafe to suspend.  All other cases are
            // handled somewhat synchronously (e.g. through hijacks, GC mode toggles, etc.)
            //
            // For example, on a SMP if the thread is blocked waiting for the ThreadStore
            // lock, it can cause a deadlock if we suspend it (even though it is in
            // preemptive mode).
            //
            // If a thread is in preemptive mode (including the tricky optimized N/Direct
            // case), we can just mark it for suspension.  It will make no further progress
            // in the EE.
            if (dwSuspendCount == (DWORD) -1)
            {
                // Nothing to do if the thread has already terminated.
            }
            else if (!m_fPreemptiveGCDisabled)
            {
                // Clear the events for thread suspension and reaching a safe spot.
                ClearSuspendEvent();
    
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                if ((m_State & TS_GCSuspendPending) == 0)
                    ClearSafeEvent();
    
                // We just want to trap this thread if it comes back into cooperative mode
                MarkForSuspension(TS_UserSuspendPending);
    
                // Let the thread run until it reaches a safe spot.
                ::ResumeThread(GetThreadHandle());
            }
            else
            {
                // Clear the events for thread suspension and reaching a safe spot.
                ClearSuspendEvent();
    
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                if ((m_State & TS_GCSuspendPending) == 0)
                    ClearSafeEvent();
    
                // Thread is executing in cooperative mode.  We're going to have to
                // move it to a safe spot.
                MarkForSuspension(TS_UserSuspendPending);
    
                // Let the thread run until it reaches a safe spot.
                ::ResumeThread(GetThreadHandle());
    
                // wait until it leaves cooperative GC mode or is JIT suspended
                FinishSuspendingThread();
            }
        }
    }
    else
    {
        // first suspension of the current thread
        BOOL    ToggleGC = PreemptiveGCDisabled();

        if (ToggleGC)
            EnablePreemptiveGC();

        ClearSuspendEvent();
        MarkForSuspension(TS_UserSuspendPending);

        // prepare to block ourselves
        ThreadStore::UnlockThreadStore();
        mustUnlock = FALSE;
        _ASSERTE(!ThreadStore::HoldingThreadStore(this));

        WaitSuspendEvent();

        if (ToggleGC)
            DisablePreemptiveGC();
    }

    if (mustUnlock)
        ThreadStore::UnlockThreadStore();
}


// if the only suspension of this thread is user imposed, resume it.  But don't
// resume from any system suspensions (like GC).
BOOL Thread::UserResumeThread()
{
    // If we are attempting to resume when we aren't in a user suspension,
    // its an error.
    BOOL    res = FALSE;

    // Note that the model does not count.  In other words, you can call Thread.Suspend()
    // five times and Thread.Resume() once.  The result is that the thread resumes.

    LOG((LF_SYNC, INFO3, "UserResumeThread obtain lock\n"));
    ThreadStore::LockThreadStore();

    // If we have marked a thread for suspension, while that thread is still starting
    // up, simply remove the bit to resume it.
    if (m_State & TS_SuspendUnstarted)
    {
        _ASSERTE((m_State & TS_UserSuspendPending) == 0);
        FastInterlockAnd((ULONG *) &m_State, ~TS_SuspendUnstarted);
        res = TRUE;
    }

    // If we are still trying to suspend the thread, forget about it.
    if (m_State & TS_UserSuspendPending)
    {
        UnmarkForSuspension(~TS_UserSuspendPending);
        SetSuspendEvent();
        SetSafeEvent();
        res = TRUE;
    }

    ThreadStore::UnlockThreadStore();
    return res;
}


// We are asynchronously trying to suspend this thread.  Stay here until we achieve
// that goal (in fully interruptible JIT code), or the thread dies, or it leaves
// the EE (in which case the Pending flag will cause it to synchronously suspend
// itself later, or if the thread tells us it is going to synchronously suspend
// itself because of hijack activity, etc.
void Thread::FinishSuspendingThread()
{
    DWORD   res;

    // There are two threads of interest -- the current thread and the thread we are
    // going to wait for.  Since the current thread is about to wait, it's important
    // that it be in preemptive mode at this time.

#if _DEBUG
    DWORD   dbgTotalTimeout = 0;
#endif

    // Wait for us to enter the ping period, then check if we are in interruptible
    // JIT code.
    while (TRUE)
    {
        ThreadStore::UnlockThreadStore();
        res = ::WaitForSingleObject(m_SafeEvent, PING_JIT_TIMEOUT);
        LOG((LF_SYNC, INFO3, "FinishSuspendingThread obtain lock\n"));
        ThreadStore::LockThreadStore();

        if (res == WAIT_TIMEOUT)
        {
#ifdef _DEBUG
            if ((dbgTotalTimeout += PING_JIT_TIMEOUT) >= DETECT_DEADLOCK_TIMEOUT)
            {
                _ASSERTE(!"Timeout detected trying to synchronously suspend a thread");
                dbgTotalTimeout = 0;
            }
#endif
            // Suspend the thread and see if we are in interruptible code (placing
            // a hijack if warranted).
            DWORD dwSuspendCount = ::SuspendThread(GetThreadHandle());

            if (m_fPreemptiveGCDisabled && dwSuspendCount != (DWORD) -1)
            {
                // Keep trying...
                ::ResumeThread(GetThreadHandle());
            }
            else if (dwSuspendCount != (DWORD) -1)
            {
                // The thread has transitioned out of the EE.  It can't get back in
                // without synchronously suspending itself.  We can now return to our
                // caller since this thread cannot make further progress within the
                // EE.
                ::ResumeThread(GetThreadHandle());
                break;
            }
        }
        else
        {
            // SafeEvent has been set so we don't need to actually suspend.  Either
            // the thread died, or it will enter a synchronous suspension based on
            // the UserSuspendPending bit.
            _ASSERTE(res == WAIT_OBJECT_0);
            _ASSERTE(!ThreadStore::HoldingThreadStore(this));
            break;
        }
    }
}


void Thread::SetSafeEvent()
{
        if (m_SafeEvent != INVALID_HANDLE_VALUE)
                ::SetEvent(m_SafeEvent);
}


void Thread::ClearSafeEvent()
{
    _ASSERTE(g_fProcessDetach || ThreadStore::HoldingThreadStore());
    ::ResetEvent(m_SafeEvent);
}


void Thread::SetSuspendEvent()
{
    FastInterlockAnd((ULONG *) &m_State, ~TS_SyncSuspended);
    ::SetEvent(m_SuspendEvent);
}


void Thread::ClearSuspendEvent()
{
    ::ResetEvent(m_SuspendEvent);
}

// There's a bit of a hack here
void Thread::WaitSuspendEvent(BOOL fDoWait)
{
    _ASSERTE(!PreemptiveGCDisabled());
    _ASSERTE((m_State & TS_SyncSuspended) == 0);

    // Let us do some useful work before suspending ourselves.

    // If we're required to perform a wait, do so.  Typically, this is
    // skipped if this thread is a Debugger Special Thread.
    if (fDoWait)
    {
        //
        // We set these bits so that we can make a reasonable statement
        // about the state of the thread for COM+ users. We don't really
        // use these for synchronizaiton or control.
        //
        FastInterlockOr((ULONG *) &m_State, TS_SyncSuspended);

        ::WaitForSingleObject(m_SuspendEvent, INFINITE);

        // Bits are reset right here so that we can report our state properly.
        FastInterlockAnd((ULONG *) &m_State, ~TS_SyncSuspended);
    }
}


//
// InitRegDisplay: initializes a REGDISPLAY for a thread. If validContext
// is false, pRD is filled from the current context of the thread. The
// thread's current context is also filled in pctx. If validContext is true,
// pctx should point to a valid context and pRD is filled from that.
//
bool Thread::InitRegDisplay(const PREGDISPLAY pRD, PCONTEXT pctx,
                            bool validContext)
{
    if (!validContext)
    {
        if (GetFilterContext()!= NULL)
            pctx = GetFilterContext();
        else
        {
            pctx->ContextFlags = CONTEXT_FULL;

            BOOL ret = ::GetThreadContext(GetThreadHandle(), pctx);

            if (!ret)
            {
#ifdef _X86_
                pctx->Eip = 0;
                pRD->pPC  = (SLOT*)&(pctx->Eip);
#elif defined(_PPC_)
                pctx->Iar = 0;
                pRD->pPC = (SLOT*)&(pctx->Iar);
#else
                PORTABILITY_ASSERT("NYI for platform InitRegDisplay");
#endif 

                return false;
            }
        }
    }

    FillRegDisplay( pRD, pctx );
    
    return true;
}


void Thread::FillRegDisplay(const PREGDISPLAY pRD, PCONTEXT pctx)
{
    pRD->pContext = pctx;

#if defined(_X86_)

    pRD->pEdi = &(pctx->Edi);
    pRD->pEsi = &(pctx->Esi);
    pRD->pEbx = &(pctx->Ebx);
    pRD->pEbp = &(pctx->Ebp);
    pRD->pEax = &(pctx->Eax);
    pRD->pEcx = &(pctx->Ecx);
    pRD->pEdx = &(pctx->Edx);
    pRD->SP   = pctx->Esp;
    pRD->pPC  = (SLOT*)&(pctx->Eip);

#elif defined(_PPC_)

    int i;

    pRD->CR  = pctx->Cr;

    for (i = 0; i < NUM_CALLEESAVED_REGISTERS; i++) {
        pRD->pR[i] = (DWORD*)&(pctx->Gpr13) + i;
    }

    for (i = 0; i < NUM_FLOAT_CALLEESAVED_REGISTERS; i++) {
        pRD->pF[i] = (DOUBLE*)&(pctx->Fpr14) + i;
    }

    pRD->SP  = pctx->Gpr1;
    pRD->pPC  = (SLOT*)&(pctx->Iar);

#else
    PORTABILITY_ASSERT("@NYI Platform - InitRegDisplay (Threads.cpp)");
#endif
}




//                      Trip Functions
//                      ==============
// When a thread reaches a safe place, it will rendezvous back with us, via one of
// the following trip functions:

void __cdecl CommonTripThread()
{
    THROWSCOMPLUSEXCEPTION();

    Thread  *thread = GetThread();
    TRIGGERSGC();

    thread->HandleThreadAbort ();
    
    if (thread->CatchAtSafePoint() && !g_fFinalizerRunOnShutDown)
    {
        _ASSERTE(!ThreadStore::HoldingThreadStore(thread));

        // Give stopping a thread preference over suspending it (obviously).
        // Give stopping a thread preference over starting a GC, because we will
        // have one less stack to crawl.
        if ((thread->m_PreventAsync == 0) &&
            (thread->m_State & Thread::TS_StopRequested) != 0)
        {
            thread->ResetStopRequest();
            if (!(thread->m_State & Thread::TS_AbortRequested))
                    COMPlusThrow(kThreadStopException);
            // else must be a thread abort. Check that we are not already processing an abort
            // and that there are no pending exceptions
            if (!(thread->m_State & Thread::TS_AbortInitiated) &&   
                    (thread->GetThrowable() == NULL))
            {
                thread->SetAbortInitiated();
                COMPlusThrow(kThreadAbortException);
            }
        }
        // Trap
        thread->PulseGCMode();
    }
}


// A stub is returning an ObjectRef to its caller
EXTERN_C void * __cdecl OnStubObjectWorker(OBJECTREF oref)
{
    void    *retval;

    GCPROTECT_BEGIN(oref)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif

        // we can't return an OBJECTREF, or in the checked build it will return a
        // struct as a secret argument.
        retval = *((void **) &oref);
    }
    GCPROTECT_END();        // trashes oref here!

    return retval;
}


// A stub is returning an ObjectRef to its caller
EXTERN_C void * __cdecl OnStubInteriorPointerWorker(void* addr)
{
    void    *retval;

    GCPROTECT_BEGININTERIOR(addr)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif

        // we can't return an OBJECTREF, or in the checked build it will return a
        // struct as a secret argument.
        retval = addr;
    }
    GCPROTECT_END();        // trashes oref here!

    return retval;
}


#ifdef _DEBUG
EXTERN_C VOID __cdecl OnStubScalarWorker(VOID)
{
    BOOL GCOnTransition=FALSE;
    if (g_pConfig->FastGCStressLevel()) {
        GCOnTransition = GC_ON_TRANSITIONS (FALSE);
    }
    CommonTripThread();
    if (g_pConfig->FastGCStressLevel()) {
        GC_ON_TRANSITIONS (GCOnTransition);
    }
}
#endif //_DEBUG


//
// Either the interpreter is executing a break opcode or a break instruction
// has been caught by the exception handling. In either case, we want to
// have this thread wait before continuing to execute. We do this with a
// PulseGCMode, which will trip the tread and leave it waiting on its suspend
// event. This case does not call CommonTripThread because we don't want
// to give the thread the chance to exit or otherwise suspend itself.
//
VOID OnDebuggerTripThread(void)
{
    Thread  *thread = GetThread();

    if (thread->m_State & thread->TS_DebugSuspendPending)
    {
        _ASSERTE(!ThreadStore::HoldingThreadStore(thread));
        thread->PulseGCMode();
    }
}

void Thread::SetFilterContext(CONTEXT *pContext) 
{ 
    m_debuggerWord1 = pContext;
}

CONTEXT *Thread::GetFilterContext(void)
{
   return (CONTEXT*)m_debuggerWord1;
}

void Thread::SetDebugCantStop(bool fCantStop) 
{ 
    if (fCantStop)
    {
        m_debuggerCantStop++;
    }
    else
    {
        m_debuggerCantStop--;
    }
}

bool Thread::GetDebugCantStop(void) 
{
    return m_debuggerCantStop != 0;
}


#ifdef _DEBUG
VOID Thread::ValidateThrowable()
{
    OBJECTREF throwable = GetThrowable();
    if (throwable != NULL)
    {
        EEClass* pClass = throwable->GetClass();
        while (pClass != NULL)
        {
            if (pClass == g_pExceptionClass->GetClass())
            {
                return;
            }
            pClass = pClass->GetParentClass();
        }
    }
}
#endif


// Some simple helpers to keep track of the threads we are waiting for
void Thread::MarkForSuspension(ULONG bit)
{
    _ASSERTE(bit == TS_DebugSuspendPending ||
             bit == (TS_DebugSuspendPending | TS_DebugWillSync) ||
             bit == TS_UserSuspendPending);

    _ASSERTE(g_fProcessDetach || g_fRelaxTSLRequirement || ThreadStore::HoldingThreadStore());

    _ASSERTE((m_State & bit) == 0);

    FastInterlockOr((ULONG *) &m_State, bit);
    ThreadStore::TrapReturningThreads(TRUE);
}

void Thread::UnmarkForSuspension(ULONG mask)
{
    _ASSERTE(mask == ~TS_DebugSuspendPending ||
             mask == ~TS_UserSuspendPending);

    _ASSERTE(g_fProcessDetach || g_fRelaxTSLRequirement || ThreadStore::HoldingThreadStore());

    _ASSERTE((m_State & ~mask) != 0);

    FastInterlockAnd((ULONG *) &m_State, mask);
    ThreadStore::TrapReturningThreads(FALSE);
}

void Thread::SetExposedContext(Context *c)
{
    // Set the ExposedContext ... 
    
    // Note that we use GetxxRaw() here to cover our bootstrap case 
    // for AppDomain proxy creation
    // Leaving the exposed object NULL lets us create the default
    // managed context just before we marshal a new AppDomain in 
    // RemotingServices::CreateProxyForDomain.
    
    Thread* pThread = GetThread();
    if (!pThread)
        return;

    BEGIN_ENSURE_COOPERATIVE_GC();

    if(m_ExposedObject != NULL) {
        THREADBASEREF threadObj = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
        if(threadObj != NULL)
        if (!c)
            threadObj->SetExposedContext(NULL);
        else
            threadObj->SetExposedContext(c->GetExposedObjectRaw());
    
    }

    END_ENSURE_COOPERATIVE_GC();
}


void Thread::InitContext()
{
    // this should only be called when initializing a thread
    _ASSERTE(m_Context == NULL);
    _ASSERTE(m_pDomain == NULL);

    m_Context = SystemDomain::System()->DefaultDomain()->GetDefaultContext();
    SetExposedContext(m_Context);
    m_pDomain = m_Context->GetDomain();
    _ASSERTE(m_pDomain);
    m_pDomain->ThreadEnter(this, NULL);
}

void Thread::ClearContext()
{
    // if one is null, both must be
    _ASSERTE(m_pDomain && m_Context || ! (m_pDomain && m_Context));

    if (!m_pDomain)
        return;

    m_pDomain->ThreadExit(this, NULL);

    // must set exposed context to null first otherwise object verification
    // checks will fail AV when m_Context is null
    SetExposedContext(NULL);
    m_pDomain = NULL;
    m_Context = NULL;
    m_ADStack.ClearDomainStack();
}

// If we are entering from default context of default domain, we do not enter
// pContext as requested by the caller. Instead we enter the default context of the target
// domain and let the actuall call perform the context transition if any. This is done to
// prevent overhead of app-domain transition for thread-pool threads that actually have no domain to start with
void Thread::DoADCallBack(Context *pContext, Context::ADCallBackFcnType pTarget, LPVOID args)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    LPVOID espVal = GetSP();

    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::DoADCallBack Calling %p at esp %p in [%d]\n", 
            pTarget, espVal, pContext->GetDomain()->GetId()));
#endif
    _ASSERTE(GetThread()->GetContext() != pContext);
    Thread* pThread  = GetThread();

    if (pThread->GetContext() == SystemDomain::System()->DefaultDomain()->GetDefaultContext())
    {
        // use the target domain's default context as the target context
        // so that the actual call to a transparent proxy would enter the object into the correct context.
        Context* newContext = pContext->GetDomain()->GetDefaultContext();
        _ASSERTE(newContext);
        DECLARE_ALLOCA_CONTEXT_TRANSITION_FRAME(pFrame);
        pThread->EnterContext(newContext, pFrame, TRUE);
         (pTarget)(args);
        // unloadBoundary is cleared by ReturnToContext, so get it now.
        Frame *unloadBoundaryFrame = pThread->GetUnloadBoundaryFrame();
        pThread->ReturnToContext(pFrame, TRUE);

        // if someone caught the abort before it got back out to the AD transition (like DispatchEx_xxx does)
        // then need to turn the abort into an unload, as they're gonna keep seeing it anyway
        if (pThread->ShouldChangeAbortToUnload(pFrame, unloadBoundaryFrame))
        {
            LOG((LF_APPDOMAIN, LL_INFO10, "Thread::DoADCallBack turning abort into unload\n"));
            COMPlusThrow(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded_ThreadUnwound");
        }
    }
    else
    {
        Context::ADCallBackArgs callTgtArgs = {pTarget, args};
        Context::CallBackInfo callBackInfo = {Context::ADTransition_callback, (void*) &callTgtArgs};
        Context::RequestCallBack(pContext, (void*) &callBackInfo);
    }
    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::DoADCallBack Done at esp %p\n", espVal));
}

void Thread::EnterContext(Context *pContext, Frame *pFrame, BOOL fLinkFrame)
{
    EnterContextRestricted(pContext, pFrame, fLinkFrame);
}

void Thread::EnterContextRestricted(Context *pContext, Frame *pFrame, BOOL fLinkFrame)
{
    THROWSCOMPLUSEXCEPTION();   // Might throw OutOfMemory.

    _ASSERTE(GetThread() == this);
    _ASSERTE(pContext);     // should never enter a null context
    _ASSERTE(m_Context);    // should always have a current context    

    pFrame->SetReturnContext(m_Context);
    pFrame->SetReturnLogicalCallContext(NULL);
    pFrame->SetReturnIllogicalCallContext(NULL);

    if (m_Context == pContext) {
        _ASSERTE(m_Context->GetDomain() == pContext->GetDomain());
        return;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    AppDomain *pDomain = pContext->GetDomain();
    // and it should always have an AD set
    _ASSERTE(pDomain);

    if (m_pDomain != pDomain && !pDomain->CanThreadEnter(this))
    {
        DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
        pFrame->SetReturnContext(NULL);
        COMPlusThrow(kAppDomainUnloadedException);
    }


    LOG((LF_APPDOMAIN, LL_INFO1000, "%sThread::EnterContext from (%p) [%d] (count %d)\n", 
            GCHeap::IsCurrentThreadFinalizer() ? "FT: " : "",
            m_Context, m_Context->GetDomain()->GetId(), 
            m_Context->GetDomain()->GetThreadEnterCount()));
    LOG((LF_APPDOMAIN, LL_INFO1000, "                     into (%p) [%d] (count %d)\n", pContext, 
                m_Context->GetDomain()->GetId(),
                pContext->GetDomain()->GetThreadEnterCount()));

#ifdef _DEBUG_ADUNLOAD
    printf("Thread::EnterContext %x from (%8.8x) [%d]\n", GetThreadId(), m_Context, 
        m_Context ? m_Context->GetDomain()->GetId() : -1);
    printf("                     into (%8.8x) [%d] %S\n", pContext, 
                pContext->GetDomain()->GetId());
#endif

    m_Context = pContext;

    if (m_pDomain != pDomain)
    {
        _ASSERTE(pFrame);

        m_ADStack.PushDomain(pDomain);

        //
        // Push logical call contexts into frame to avoid leaks
        //

        if (IsExposedObjectSet())
        {
            THREADBASEREF ref = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
            _ASSERTE(ref != NULL);
            if (ref->GetLogicalCallContext() != NULL)
            {
                pFrame->SetReturnLogicalCallContext(ref->GetLogicalCallContext());
                ref->SetLogicalCallContext(NULL);
            }

            if (ref->GetIllogicalCallContext() != NULL)
            {
                pFrame->SetReturnIllogicalCallContext(ref->GetIllogicalCallContext());
                ref->SetIllogicalCallContext(NULL);
            }
        }

		if (fLinkFrame)
		{
			pFrame->Push();

			if (pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
			{
				((ContextTransitionFrame *)pFrame)->InstallExceptionHandler();
			}
		}

#ifdef _DEBUG_ADUNLOAD
        printf("Thread::EnterContext %x,%8.8x push? %d current frame is %8.8x\n", GetThreadId(), this, fLinkFrame, GetFrame());
#endif

        pDomain->ThreadEnter(this, pFrame);

        // Make the static data storage point to the current domain's storage
        SetStaticData(pDomain, NULL, NULL);

        m_pDomain = pDomain;
        TlsSetValue(gAppDomainTLSIndex, (VOID*) m_pDomain);
    }

    SetExposedContext(pContext);

    // Store off the Win32 Fusion context
    //
    pFrame->SetWin32Context(NULL);
    IApplicationContext* pFusion32 = m_pDomain->GetFusionContext();
    if(pFusion32) 
    {
        ULONG_PTR cookie;
        if(SUCCEEDED(pFusion32->SxsActivateContext(&cookie))) 
            pFrame->SetWin32Context(cookie);
    }

    END_ENSURE_COOPERATIVE_GC();
}

// main difference between EnterContext and ReturnToContext is that are allowed to return
// into a domain that is unloading but cannot enter a domain that is unloading
void Thread::ReturnToContext(Frame *pFrame, BOOL fLinkFrame)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(GetThread() == this);

    Context *pReturnContext = pFrame->GetReturnContext();
    _ASSERTE(pReturnContext);

    if (m_Context == pReturnContext) 
    {
        _ASSERTE(m_Context->GetDomain() == pReturnContext->GetDomain());
        return;
    }

    //
    // Return the Win32 Fusion Context
    //
    IApplicationContext* pFusion32 = m_pDomain->GetFusionContext();
    if(pFusion32 && pFrame) {
        ULONG_PTR cookie = pFrame->GetWin32Context();
        if(cookie != NULL) 
            pFusion32->SxsDeactivateContext(cookie);
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    LOG((LF_APPDOMAIN, LL_INFO1000, "%sThread::ReturnToContext from (%p) [%d] (count %d)\n", 
                GCHeap::IsCurrentThreadFinalizer() ? "FT: " : "",
                m_Context, m_Context->GetDomain()->GetId(), 
                m_Context->GetDomain()->GetThreadEnterCount()));
    LOG((LF_APPDOMAIN, LL_INFO1000, "                        into (%p) [%d] (count %d)\n", pReturnContext, 
                m_Context->GetDomain()->GetId(),
                pReturnContext->GetDomain()->GetThreadEnterCount()));

#ifdef _DEBUG_ADUNLOAD
    printf("Thread::ReturnToContext %x from (%p) [%d]\n", GetThreadId(), m_Context, 
                m_Context->GetDomain()->GetId(),
    printf("                        into (%p) [%d]\n", pReturnContext, 
                pReturnContext->GetDomain()->GetId(),
                m_Context->GetDomain()->GetThreadEnterCount());
#endif

    m_Context = pReturnContext;
    SetExposedContext(pReturnContext);

    AppDomain *pReturnDomain = pReturnContext->GetDomain();

    if (m_pDomain != pReturnDomain)
    {

        if (fLinkFrame && pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
        {
            ((ContextTransitionFrame *)pFrame)->UninstallExceptionHandler();
        }

#ifdef _DEBUG
        AppDomain *pADOnStack =
#endif
        m_ADStack.PopDomain();
        _ASSERTE(!pADOnStack || pADOnStack == m_pDomain);

        _ASSERTE(pFrame);
        //_ASSERTE(!fLinkFrame || pThread->GetFrame() == pFrame);

        // Set the static data store to point to the returning domain's store
        SafeSetStaticData(pReturnDomain, NULL, NULL);

		AppDomain *pCurrentDomain = m_pDomain;
        m_pDomain = pReturnDomain;
        TlsSetValue(gAppDomainTLSIndex, (VOID*) pReturnDomain);

        if (fLinkFrame)
        {
            if (pFrame == m_pUnloadBoundaryFrame)
                m_pUnloadBoundaryFrame = NULL;
            pFrame->Pop();
        }

        //
        // Pop logical call contexts from frame if applicable
        //

        if (IsExposedObjectSet())
        {
            THREADBASEREF ref = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
            _ASSERTE(ref != NULL);
            ref->SetLogicalCallContext(pFrame->GetReturnLogicalCallContext());
            ref->SetIllogicalCallContext(pFrame->GetReturnIllogicalCallContext());
        }

		// Do this last so that thread is not labeled as out of the domain until all cleanup is done.
        pCurrentDomain->ThreadExit(this, pFrame);

#ifdef _DEBUG_ADUNLOAD
        printf("Thread::ReturnToContext %x,%8.8x pop? %d current frame is %8.8x\n", GetThreadId(), this, fLinkFrame, GetFrame());
#endif
    }

    END_ENSURE_COOPERATIVE_GC();
    return;
}

struct FindADCallbackType {
    AppDomain *pSearchDomain;
    AppDomain *pPrevDomain;
    Frame *pFrame;
    int count;
    enum TargetTransition 
        {fFirstTransitionInto, fMostRecentTransitionInto} 
    fTargetTransition;

    FindADCallbackType() : pSearchDomain(NULL), pPrevDomain(NULL), pFrame(NULL) {}
};

StackWalkAction StackWalkCallback_FindAD(CrawlFrame* pCF, void* data)
{
    FindADCallbackType *pData = (FindADCallbackType *)data;

    Frame *pFrame = pCF->GetFrame();
    
    if (!pFrame)
        return SWA_CONTINUE;

    AppDomain *pReturnDomain = pFrame->GetReturnDomain();
    if (!pReturnDomain || pReturnDomain == pData->pPrevDomain)
        return SWA_CONTINUE;

    LOG((LF_APPDOMAIN, LL_INFO100, "StackWalkCallback_FindAD transition frame %8.8x into AD [%d]\n", 
            pFrame, pReturnDomain->GetId()));

    if (pData->pPrevDomain == pData->pSearchDomain) {
                ++pData->count;
        // this is a transition into the domain we are unloading, so save it in case it is the first
        pData->pFrame = pFrame;
        if (pData->fTargetTransition == FindADCallbackType::fMostRecentTransitionInto)
            return SWA_ABORT;   // only need to find last transition, so bail now
    }

    pData->pPrevDomain = pReturnDomain;
    return SWA_CONTINUE;
}

// This determines if a thread is running in the given domain at any point on the stack
Frame *Thread::IsRunningIn(AppDomain *pDomain, int *count)
{
    FindADCallbackType fct;
    fct.pSearchDomain = pDomain;
    // set prev to current so if are currently running in the target domain, 
    // we will detect the transition
    fct.pPrevDomain = m_pDomain;
    fct.fTargetTransition = FindADCallbackType::fMostRecentTransitionInto;
    fct.count = 0;

    // when this returns, if there is a transition into the AD, it will be in pFirstFrame
    StackWalkFrames(StackWalkCallback_FindAD, (void*) &fct);
    if (count)
        *count = fct.count;
    return fct.pFrame;
}

// This finds the very first frame on the stack where the thread transitioned into the given domain
Frame *Thread::GetFirstTransitionInto(AppDomain *pDomain, int *count)
{
    FindADCallbackType fct;
    fct.pSearchDomain = pDomain;
    // set prev to current so if are currently running in the target domain, 
    // we will detect the transition
    fct.pPrevDomain = m_pDomain;
    fct.fTargetTransition = FindADCallbackType::fFirstTransitionInto;
    fct.count = 0;

    // when this returns, if there is a transition into the AD, it will be in pFirstFrame
    StackWalkFrames(StackWalkCallback_FindAD, (void*) &fct);
    if (count)
        *count = fct.count;
    return fct.pFrame;
}

// Get outermost (oldest) AppDomain for this thread (not counting the default
// domain every one starts in).
AppDomain *Thread::GetInitialDomain()
{
    AppDomain *pDomain = m_pDomain;
    AppDomain *pPrevDomain = NULL;
    Frame *pFrame = GetFrame();
    while (pFrame != FRAME_TOP)
    {
        if (pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
        {
            if (pPrevDomain)
                pDomain = pPrevDomain;
            pPrevDomain = pFrame->GetReturnDomain();
        }
        pFrame = pFrame->Next();
    }
    return pDomain;
}

BOOL Thread::ShouldChangeAbortToUnload(Frame *pFrame, Frame *pUnloadBoundaryFrame)
{
    if (! pUnloadBoundaryFrame)
        pUnloadBoundaryFrame = GetUnloadBoundaryFrame();

    // turn the abort request into an AD unloaded exception when go past the boundary.
    if (pFrame != pUnloadBoundaryFrame)
        return FALSE;

    // Only time have an unloadboundaryframe is when have specifically marked that thread for aborting
    // during unload processing, so this won't trigger UnloadedException if have simply thrown a ThreadAbort 
    // past an AD transition frame
    if (IsAbortRequested())
    {
        UserResetAbort();
        return TRUE;
    }

    // abort may have been reset, so check for AbortException as a backup
    OBJECTREF pThrowable = GetThrowable();

    if (pThrowable == NULL)
        return FALSE;

    DefineFullyQualifiedNameForClass();
    LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
    if (szClass && strcmp(g_ThreadAbortExceptionClassName, szClass) == 0)
        return TRUE;

    return FALSE;

}


BOOL Thread::HaveExtraWorkForFinalizer()
{
    return m_ThreadTasks || GCInterface::IsCacheCleanupRequired();
}

void Thread::DoExtraWorkForFinalizer()
{
    _ASSERTE(GetThread() == this);
    _ASSERTE(this == GCHeap::GetFinalizerThread());


    if (RequireSyncBlockCleanup())
    {
        SyncBlockCache::GetSyncBlockCache()->CleanupSyncBlocks();
    }
    if (GCInterface::IsCacheCleanupRequired() && GCHeap::GetCondemnedGeneration()==1) {
        GCInterface::CleanupCache();
    }
}



//+----------------------------------------------------------------------------
//
//  Method:     Thread::GetStaticFieldAddress   private
//
//  Synopsis:   Get the address of the field relative to the current thread.
//              If an address has not been assigned yet then create one.
//+----------------------------------------------------------------------------
LPVOID Thread::GetStaticFieldAddress(FieldDesc *pFD)
{
    THROWSCOMPLUSEXCEPTION();
    
    BOOL fThrow = FALSE;
    STATIC_DATA *pData;
    LPVOID pvAddress = NULL;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetThreadStaticOffset();
    WORD currElem = 0; 
    Thread *pThread = GetThread();
     
    // NOTE: if you change this method, you must also change
    // GetStaticFieldAddrForDebugger below.
   
    _ASSERTE(NULL != pThread);
    if(!fIsShared)
    {
        pData = pThread->GetUnsharedStaticData();
    }
    else
    {
        pData = pThread->GetSharedStaticData();
    }
    

    if(NULL != pData)
    {
        currElem = pData->cElem;
    }

    // Check whether we have allocated space for storing a pointer to
    // this class' thread static store    
    if(wClassOffset >= currElem)
    {
        // Allocate space for storing pointers 
        WORD wNewElem = (currElem == 0 ? 4 : currElem*2);

        // Ensure that we grow to a size beyond the index we intend to use
        while (wNewElem <= wClassOffset)
        {
            wNewElem = wNewElem*2;
        }

        STATIC_DATA *pNew = (STATIC_DATA *)new BYTE[sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID)];
        if(NULL != pNew)
        {
            memset(pNew, 0x00, sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID));
            if(NULL != pData)
            {
                // Copy the old data into the new data
                memcpy(pNew, pData, sizeof(STATIC_DATA) + currElem*sizeof(LPVOID));
            }
            pNew->cElem = wNewElem;

            // Delete the old data
            delete pData;

            // Update the locals
            pData = pNew;

            // Reset the pointers in the thread object to point to the 
            // new memory
            if(!fIsShared)
            {
                pThread->SetStaticData(pThread->GetDomain(), NULL, pData);
            }
            else
            {
                pThread->SetStaticData(pThread->GetDomain(), pData, NULL);
            }            
        }
        else
        {
            fThrow = TRUE;
        }
    }

    // Check whether we have to allocate space for 
    // the thread local statics of this class
    if(!fThrow && (NULL == pData->dataPtr[wClassOffset]))        
    {
        // Allocate memory for thread static fields with extra space in front for the class owning the storage.
        // We stash the class at the front of the allocated storage so that we can use
        // it to interpret the data on unload in DeleteThreadStaticClassData.
        pData->dataPtr[wClassOffset] = (LPVOID)(new BYTE[pMT->GetClass()->GetThreadStaticsSize()+sizeof(EEClass*)] + sizeof(EEClass*));
        if(NULL != pData->dataPtr[wClassOffset])
        {
            // Initialize the memory allocated for the fields
            memset(pData->dataPtr[wClassOffset], 0x00, pMT->GetClass()->GetThreadStaticsSize());
            *(EEClass**)((BYTE*)(pData->dataPtr[wClassOffset]) - sizeof(EEClass*)) = pMT->GetClass();
        }
        else
        {
            fThrow = TRUE;
        }
    }

    if(!fThrow)
    {
        _ASSERTE(NULL != pData->dataPtr[wClassOffset]);
        // We have allocated static storage for this data
        // Just return the address by getting the offset into the data
        pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

        // For object and value class fields we have to allocate storage in the
        // __StaticContainer class in the managed heap. Instead of pvAddress being
        // the actual address of the static, for such objects it holds the slot index
        // to the location in the __StaticContainer member.
        if(pFD->IsObjRef() || pFD->IsByValue())
        {
            // _ASSERTE(FALSE);
            // in this case *pvAddress == bucket|index
            int *pSlot = (int*)pvAddress;
            pvAddress = NULL;

            fThrow = GetStaticFieldAddressSpecial(pFD, pMT, pSlot, &pvAddress);

            if (pFD->IsByValue())
            {
                _ASSERTE(pvAddress != NULL);
                pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
            }

            // ************************************************
            // ************** WARNING *************************
            // Do not provoke GC from here to the point JIT gets
            // pvAddress back
            // ************************************************
            _ASSERTE(*pSlot > 0);
        }
    }
    else
    {
        COMPlusThrowOM();
    }

    _ASSERTE(NULL != pvAddress);

    return pvAddress;
}

//+----------------------------------------------------------------------------
//       
//  Method:     Thread::GetStaticFieldAddrForDebugger   private
//
//  Synopsis:   Get the address of the field relative to the current thread.
//              If an address has not been assigned, return NULL.
//              No creating is allowed.
//+----------------------------------------------------------------------------
LPVOID Thread::GetStaticFieldAddrForDebugger(FieldDesc *pFD)
{
    STATIC_DATA *pData;
    LPVOID pvAddress = NULL;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetThreadStaticOffset();

    // Note: this function operates on 'this' Thread, not the
    // 'current' thread.

    // NOTE: if you change this method, you must also change
    // GetStaticFieldAddress above.
   
    if (!fIsShared)
        pData = GetUnsharedStaticData();
    else
        pData = GetSharedStaticData();


    if (NULL != pData)
    {
        // Check whether we have allocated space for storing a pointer
        // to this class' thread static store.
        if ((wClassOffset < pData->cElem) && (NULL != pData->dataPtr[wClassOffset]))
        {
            // We have allocated static storage for this data.  Just
            // return the address by getting the offset into the data.
            pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

            // For object and value class fields we have to allocate
            // storage in the __StaticContainer class in the managed
            // heap. If its not already allocated, return NULL
            // instead.
            if (pFD->IsObjRef() || pFD->IsByValue())
            {
                if (NULL == *(LPVOID *)pvAddress)
                {
                    pvAddress = NULL;
                    LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = NULL"));
                }
                else
                {
                    pvAddress = CalculateAddressForManagedStatic(*(int*)pvAddress, this);
                    LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = %lx", pvAddress));
                    if (pFD->IsByValue())
                    {
                        _ASSERTE(pvAddress != NULL);
                        pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
                    }
                }
            }
        }                
    }

    return pvAddress;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::AllocateStaticFieldObjRefPtrs   private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//+----------------------------------------------------------------------------
void Thread::AllocateStaticFieldObjRefPtrs(FieldDesc *pFD, MethodTable *pMT, LPVOID pvAddress)
{
    THROWSCOMPLUSEXCEPTION();

    // Retrieve the object ref pointers from the app domain.
    OBJECTREF *pObjRef = NULL;

    // Reserve some object ref pointers.
    GetAppDomain()->AllocateStaticFieldObjRefPtrs(1, &pObjRef);


    // to a boxed version of the value class.  This allows the standard GC
    // algorithm to take care of internal pointers in the value class.
    if (pFD->IsByValue())
    {

        // Extract the type of the field
        TypeHandle  th;        
        PCCOR_SIGNATURE pSig;
        DWORD       cSig;
        pFD->GetSig(&pSig, &cSig);
        FieldSig sig(pSig, pFD->GetModule());

        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        th = sig.GetTypeHandle(&throwable);
        if (throwable != NULL)
            COMPlusThrow(throwable);
        GCPROTECT_END();

        OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
        SetObjectReference( pObjRef, obj, GetAppDomain() );                      
    }

    *(ULONG_PTR *)pvAddress =  (ULONG_PTR)pObjRef;
}


MethodDesc* Thread::GetMDofReserveSlot()
{
    if (s_pReserveSlotMD == NULL)
    {
        s_pReserveSlotMD = g_Mscorlib.GetMethod(METHOD__THREAD__RESERVE_SLOT);
    }
    _ASSERTE(s_pReserveSlotMD != NULL);
    return s_pReserveSlotMD;
}

// This is used for thread relative statics that are object refs
// These are stored in a structure in the managed thread. The first
// time over an index and a bucket are determined and subsequently
// remembered in the location for the field in the per-thread-per-class
// data structure.
// Here we map back from the index to the address of the object ref.

LPVOID Thread::CalculateAddressForManagedStatic(int slot, Thread *pThread)
{
    OBJECTREF *pObjRef;
    BEGINFORBIDGC();
    int bucket = (slot>>16);
    int index = (0x0000ffff&slot);
    // Now determine the address of the static field
    PTRARRAYREF bucketRef = NULL;
    THREADBASEREF threadRef = NULL;
    if (pThread == NULL)
    {
        // pThread is NULL except when the debugger calls this on behalf of
        // some thread
        pThread = GetThread();
        _ASSERTE(pThread!=NULL);
    }
    // We come here only after a slot is allocated for the static
    // which means we have already faulted in the exposed thread object
    threadRef = (THREADBASEREF) pThread->GetExposedObjectRaw();
    _ASSERTE(threadRef != NULL);

    bucketRef = threadRef->GetThreadStaticsHolder();
    // walk the chain to our bucket
    while (bucket--)
    {
        bucketRef = (PTRARRAYREF) bucketRef->GetAt(0);
    }

    // Index 0 is used to point to the next bucket!
    _ASSERTE(index > 0);
    pObjRef = ((OBJECTREF*)bucketRef->GetDataPtr())+index;
    ENDFORBIDGC();
    return (LPVOID) pObjRef;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::GetStaticFieldAddressSpecial private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//+----------------------------------------------------------------------------

// NOTE: At one point we used to allocate these in the long lived handle table
// which is per-appdomain. However, that causes them to get rooted and not 
// cleaned up until the appdomain gets unloaded. This is not very desirable 
// since a thread static object may hold a reference to the thread itself 
// causing a whole lot of garbage to float around.
// Now (2/13/01) these are allocated from a managed structure rooted in each
// managed thread.

BOOL Thread::GetStaticFieldAddressSpecial(
    FieldDesc *pFD, MethodTable *pMT, int *pSlot, LPVOID *ppvAddress)
{
    BOOL fThrow = FALSE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();    

    COMPLUS_TRY 
    {
        OBJECTREF *pObjRef = NULL;
        BOOL bNewSlot = (*pSlot == 0);
        if (bNewSlot)
        {
            // ! this line will trigger a GC, don't move it down
            // ! without protecting the args[] and other OBJECTREFS
            MethodDesc * pMD = GetMDofReserveSlot();
            
            // We need to assign a location for this static field. 
            // Call the managed helper
            ARG_SLOT args[1] = {
                ObjToArgSlot(GetThread()->GetExposedObject())
            };

            _ASSERTE(args[0] != 0);

            *pSlot = (int) pMD->Call(
                            args, 
                            METHOD__THREAD__RESERVE_SLOT);

            _ASSERTE(*pSlot>0);

            // to a boxed version of the value class.  This allows the standard GC
            // algorithm to take care of internal pointers in the value class.
            if (pFD->IsByValue())
            {
                // Extract the type of the field
                TypeHandle  th;        
                PCCOR_SIGNATURE pSig;
                DWORD       cSig;
                pFD->GetSig(&pSig, &cSig);
                FieldSig sig(pSig, pFD->GetModule());
    
                OBJECTREF throwable = NULL;
                GCPROTECT_BEGIN(throwable);
                th = sig.GetTypeHandle(&throwable);
                if (throwable != NULL)
                    COMPlusThrow(throwable);
                GCPROTECT_END();

                OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
                SetObjectReference( pObjRef, obj, GetAppDomain() );                      
            }
            else
            {
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
            }
        }
        else
        {
            // If the field already has a location assigned we go through here
            pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
        }
        *(ULONG_PTR *)ppvAddress =  (ULONG_PTR)pObjRef;
    } 
    COMPLUS_CATCH
    {
        fThrow = TRUE;
    }            
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return fThrow;
}


 
//+----------------------------------------------------------------------------
//
//  Method:     Thread::SetStaticData   private
//
//  Synopsis:   Finds or creates an entry in the list which has the same domain
//              as the one given. These entries have pointers to the thread 
//              local static data in each appdomain.
//              This function is called in two situations
//              (1) We are switching domains and need to set the pointers
//              to the static storage for the domain we are entering.
//              (2) We are accessing thread local storage for the current
//              domain and we need to set the pointer to the static storage
//              that we have created.
//+----------------------------------------------------------------------------
STATIC_DATA_LIST *Thread::SetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData,
                                        STATIC_DATA *pUnsharedData)
{   
    THROWSCOMPLUSEXCEPTION();

    // we need to make sure no preemptive mode threads get in here. Otherwise an appdomain unload
    // cannot simply stop the EE and delete entries from this list, assuming there are no threads 
    // touching these structures. If preemptive mode threads get in here, we will have to do a 
    // deferred cleanup like for the codemanager.
    _ASSERTE (GetThread()->PreemptiveGCDisabled());

    STATIC_DATA_LIST *pCurr = m_pStaticDataList;
    while(NULL != pCurr)
    {
        if(pCurr->m_pDomain == pDomain)
        {
            // Found a match
            if(NULL == pSharedData)
            {
                m_pSharedStaticData = pCurr->m_pSharedStaticData;
            }
            else
            {
                m_pSharedStaticData = pCurr->m_pSharedStaticData = pSharedData;
            }

            if(NULL == pUnsharedData)
            {
                m_pUnsharedStaticData = pCurr->m_pUnsharedStaticData;
            }
            else
            {
                m_pUnsharedStaticData = pCurr->m_pUnsharedStaticData = pUnsharedData;
            }

            break;
        }

        pCurr = pCurr->m_pNext;
    }

    // Create a new entry 
    if(pCurr == NULL)
    {
        // This is the first time that this thread is entering the given domain
        pCurr = (STATIC_DATA_LIST *)new BYTE[sizeof(STATIC_DATA_LIST)];
        if(NULL == pCurr)
        {
            COMPlusThrowOM();
        }
        
        // Initialize the list element
        memset(pCurr, 0x00, sizeof(STATIC_DATA_LIST));
        
        // Add to the list
        pCurr->m_pNext = m_pStaticDataList;
        m_pStaticDataList = pCurr;
        m_pSharedStaticData = pCurr->m_pSharedStaticData = pSharedData;
        m_pUnsharedStaticData = pCurr->m_pUnsharedStaticData = pUnsharedData;
        pCurr->m_pDomain = pDomain;
    }

    return pCurr;
}

// A version of SetStaticData that is guaranteed not to throw.  This can be used in
// ReturnToContext where we're sure we don't have to allocate.
STATIC_DATA_LIST *Thread::SafeSetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData,
                                        STATIC_DATA *pUnsharedData)
{   
    STATIC_DATA_LIST *result = NULL;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        result = SetStaticData(pDomain, pSharedData, pUnsharedData);
    }
    COMPLUS_CATCH
    {
        _ASSERTE(!"Thread::SafeSetStaticData() got an unexpected exception");
        result = NULL;
    }
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return result;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::DeleteThreadStaticData   private
//
//  Synopsis:   Delete the static data for each appdomain that this thread
//              visited.
//              
//+----------------------------------------------------------------------------
void Thread::DeleteThreadStaticData()
{
    STATIC_DATA_LIST *pCurr = m_pStaticDataList;

    // we want to delete the current domain data even if it doesn't
    // get deleted in the main list. 
    _STATIC_DATA* mySharedStaticData = m_pSharedStaticData;
    _STATIC_DATA* myUnsharedStaticData = m_pUnsharedStaticData;

    while(NULL != pCurr)
    {
        STATIC_DATA_LIST *pPrev = pCurr;

        if (pCurr->m_pSharedStaticData == mySharedStaticData)
            mySharedStaticData = 0;

        _STATIC_DATA* pData = pCurr->m_pSharedStaticData;

        DeleteThreadStaticClassData(pData, FALSE);

        if (pCurr->m_pUnsharedStaticData == myUnsharedStaticData)
            myUnsharedStaticData = 0;

        pData = pCurr->m_pUnsharedStaticData;
        DeleteThreadStaticClassData(pData, FALSE);

        pCurr = pCurr->m_pNext;

        delete pPrev;
    }

    m_pStaticDataList = NULL;

    // We have not deleted the current domain's static store
    delete mySharedStaticData;
    delete myUnsharedStaticData;
    

    m_pSharedStaticData = NULL;
    m_pUnsharedStaticData = NULL;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::DeleteThreadStaticData   protected
//
//  Synopsis:   Delete the static data for the given appdomain. This is called
//              when the appdomain unloads.
//              
//+----------------------------------------------------------------------------
void Thread::DeleteThreadStaticData(AppDomain *pDomain)
{
    STATIC_DATA_LIST *pPrev = NULL;
    STATIC_DATA_LIST *pCurr = m_pStaticDataList;

    // Loop through the list looking for the thread local store 
    // for the given domain
    while(NULL != pCurr)
    {
        if(pCurr->m_pDomain == pDomain)
        {
            // Delete the shared static data
            _STATIC_DATA* pData = pCurr->m_pSharedStaticData;
            if(pData == m_pSharedStaticData)
            {
                m_pSharedStaticData = NULL;
            }
            DeleteThreadStaticClassData(pData, TRUE);
            
            // Delete the unshared static data
            pData = pCurr->m_pUnsharedStaticData;
            if(pData == m_pUnsharedStaticData)
            {
                m_pUnsharedStaticData = NULL;
            }
            DeleteThreadStaticClassData(pData, TRUE);

            // Adjust the list pointers 
            if(NULL != pPrev)
            {
                pPrev->m_pNext = pCurr->m_pNext;
            }
            else
            {
                m_pStaticDataList = pCurr->m_pNext;
            }

            // delete the given domain's entry
            delete pCurr;

            // Since we have found a match we break out of the loop
            break;
        }
        
        // Advance the pointers to the next element
        pPrev = pCurr;
        pCurr = pCurr->m_pNext;
    }
}

// for valuetype and reference thread statics, we use the entry in the pData->dataPtr array for
// the class to hold an index of a slot to index into the managed array hung off the thread where
// such statics are rooted. We need to find those objects and null out their slots so that they
// will be collected properly on an unload.
void Thread::DeleteThreadStaticClassData(_STATIC_DATA* pData, BOOL fClearFields)
{
    if (pData == NULL)
        return;

    for(WORD i = 0; i < pData->cElem; i++)
    {
        void *dataPtr = (void *)pData->dataPtr[i];
        if (! dataPtr)
            continue;

        // if thread doesn't have an ExposedObject (eg. could be dead), then nothing to clean up.
        if (fClearFields && GetExposedObjectRaw() != NULL)
        {
            EEClass *pClass = *(EEClass**)(((BYTE*)dataPtr) - sizeof(EEClass*));
            _ASSERTE(pClass->GetMethodTable()->GetClass() == pClass);

            // iterate through each static field and get it's address in the managed thread 
            // structure and clear it out.

            // get a field iterator
            FieldDescIterator fdIterator(pClass, FieldDescIterator::STATIC_FIELDS);
            FieldDesc *pFD;

            while ((pFD = fdIterator.Next()) != NULL)
            {        
                if (! (pFD->IsThreadStatic() && (pFD->IsObjRef() || pFD->IsByValue())))
                    continue;

                int *pSlot = (int*)((LPBYTE)dataPtr + pFD->GetOffset());
                if (*pSlot == 0)
                    continue;

                // clear out the object in the managed structure rooted in the thread.
                OBJECTREF *pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, this);
                _ASSERT(pObjRef != 0);
                SetObjectReferenceUnchecked( pObjRef, NULL);
            }
        }
        delete ((BYTE*)(dataPtr) - sizeof(EEClass*));
    }
    delete pData;
}


// See Description in UtilCode.h for details on interface.
#define PROP_CULTURE_NAME "Name"
#define PROP_THREAD_UI_CULTURE "CurrentUICulture"
#define PROP_THREAD_CULTURE "CurrentCulture"
#define PROP_CULTURE_ID "LCID"

ARG_SLOT Thread::CallPropertyGet(BinderMethodID id, OBJECTREF pObject) 
{
    if (!pObject) {
        return 0;
    }

    MethodDesc *pMD;

    GCPROTECT_BEGIN(pObject);
    pMD = g_Mscorlib.GetMethod(id);
    GCPROTECT_END();

    // Set up the Stack.
    ARG_SLOT pNewArgs = ObjToArgSlot(pObject);

    // Make the actual call.
    ARG_SLOT retVal = pMD->Call(&pNewArgs, id);

    return retVal;
}

ARG_SLOT Thread::CallPropertySet(BinderMethodID id, OBJECTREF pObject, OBJECTREF pValue) 
{
    if (!pObject) {
        return 0;
    }

    MethodDesc *pMD;

    GCPROTECT_BEGIN(pObject);
    GCPROTECT_BEGIN(pValue);
    pMD = g_Mscorlib.GetMethod(id);
    GCPROTECT_END();
    GCPROTECT_END();

    // Set up the Stack.
    ARG_SLOT pNewArgs[] = {
        ObjToArgSlot(pObject),
        ObjToArgSlot(pValue)
    };

    // Make the actual call.
    ARG_SLOT retVal = pMD->Call(pNewArgs, id);

    return retVal;
}

OBJECTREF Thread::GetCulture(BOOL bUICulture)
{

    FieldDesc *         pFD;

    _ASSERTE(PreemptiveGCDisabled());

    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        return NULL;
    }

    // Get the actual thread culture.
    OBJECTREF pCurThreadObject = GetExposedObject();
    _ASSERTE(pCurThreadObject!=NULL);

    THREADBASEREF pThreadBase = (THREADBASEREF)(pCurThreadObject);
    OBJECTREF pCurrentCulture = bUICulture ? pThreadBase->GetCurrentUICulture() : pThreadBase->GetCurrentUserCulture();

    if (pCurrentCulture==NULL) {
        GCPROTECT_BEGIN(pThreadBase); 
        if (bUICulture) {
            // Call the Getter for the CurrentUICulture.  This will cause it to populate the field.
            ARG_SLOT retVal = CallPropertyGet(METHOD__THREAD__GET_UI_CULTURE,
                                           (OBJECTREF)pThreadBase);
            pCurrentCulture = ArgSlotToObj(retVal);
        } else {
            //This is  faster than calling the property, because this is what the call does anyway.
            pFD = g_Mscorlib.GetField(FIELD__CULTURE_INFO__CURRENT_CULTURE);
            _ASSERTE(pFD);
            pCurrentCulture = pFD->GetStaticOBJECTREF();
            _ASSERTE(pCurrentCulture!=NULL);
        }
        GCPROTECT_END();
    }

    return pCurrentCulture;
}



// copy culture name into szBuffer and return length
int Thread::GetParentCultureName(LPWSTR szBuffer, int length, BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL) {
        WCHAR *tempName = L"en";
        INT32 tempLength = (INT32)wcslen(tempName);
        _ASSERTE(length>=tempLength);
        memcpy(szBuffer, tempName, tempLength*sizeof(WCHAR));
        return tempLength;
    }

    ARG_SLOT Result = 0;
    INT32 retVal=0;
    Thread *pCurThread=NULL;
    WCHAR *buffer=NULL;
    INT32 bufferLength=0;
    STRINGREF cultureName = NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    struct _gc {
        OBJECTREF pCurrentCulture;
        OBJECTREF pParentCulture;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    GCPROTECT_BEGIN(gc);

    COMPLUS_TRYEX(pCurThread)
    {
        gc.pCurrentCulture = GetCulture(bUICulture);
        if (gc.pCurrentCulture != NULL) {
            Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_PARENT, gc.pCurrentCulture);               
        }

        if (Result==0) {
            COMPLUS_LEAVE;
        }

        gc.pParentCulture = (OBJECTREF)(ArgSlotToObj(Result));
        if (gc.pParentCulture != NULL)
        {
            Result = 0;
            Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_NAME, gc.pParentCulture);
        }
    }
    COMPLUS_CATCH 
    {
    }
    COMPLUS_END_CATCH

    GCPROTECT_END();

    if (Result==0) {
        retVal = 0;
        goto Exit;
    }


    // Extract the data out of the String.
    cultureName = (STRINGREF)(ArgSlotToObj(Result));
    RefInterpretGetStringValuesDangerousForGC(cultureName, (WCHAR**)&buffer, &bufferLength);

    if (bufferLength<length) {
        memcpy(szBuffer, buffer, bufferLength * sizeof (WCHAR));
        szBuffer[bufferLength]=0;
        retVal = bufferLength;
    }

Exit:
    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return retVal;
}




// copy culture name into szBuffer and return length
int Thread::GetCultureName(LPWSTR szBuffer, int length, BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        WCHAR *tempName = L"en-US";
        INT32 tempLength = (INT32)wcslen(tempName);
        _ASSERTE(length>=tempLength);
        memcpy(szBuffer, tempName, tempLength*sizeof(WCHAR));
        return tempLength;
    }

    ARG_SLOT Result = 0;
    INT32 retVal=0;
    Thread *pCurThread=NULL;
    WCHAR *buffer=NULL;
    INT32 bufferLength=0;
    STRINGREF cultureName = NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    OBJECTREF pCurrentCulture = NULL;
    GCPROTECT_BEGIN(pCurrentCulture)
    {
        COMPLUS_TRY
        {
            pCurrentCulture = GetCulture(bUICulture);
            if (pCurrentCulture != NULL)
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_NAME, pCurrentCulture);
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (Result==0) {
        retVal = 0;
        goto Exit;
    }

    // Extract the data out of the String.
    cultureName = (STRINGREF)(ArgSlotToObj(Result));
    RefInterpretGetStringValuesDangerousForGC(cultureName, (WCHAR**)&buffer, &bufferLength);

    if (bufferLength<length) {
        memcpy(szBuffer, buffer, bufferLength * sizeof (WCHAR));
        szBuffer[bufferLength]=0;
        retVal = bufferLength;
    }

Exit:
    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return retVal;
}

// Return a language identifier.
LCID Thread::GetCultureId(BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        return (LCID) UICULTUREID_DONTCARE;
    }

    ARG_SLOT Result = (ARG_SLOT) UICULTUREID_DONTCARE;
    Thread *pCurThread=NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    OBJECTREF pCurrentCulture = NULL;
    GCPROTECT_BEGIN(pCurrentCulture)
    {
        COMPLUS_TRY
        {
            pCurrentCulture = GetCulture(bUICulture);
            if (pCurrentCulture != NULL)
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_ID, pCurrentCulture);
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return (INT32)Result;
}

void Thread::SetCultureId(LCID lcid, BOOL bUICulture)
{
    OBJECTREF CultureObj = NULL;
    GCPROTECT_BEGIN(CultureObj)
    {
        // Convert the LCID into a CultureInfo.
        GetCultureInfoForLCID(lcid, &CultureObj);

        // Set the newly created culture as the thread's culture.
        SetCulture(CultureObj, bUICulture);
    }
    GCPROTECT_END();
}

void Thread::SetCulture(OBJECTREF CultureObj, BOOL bUICulture)
{
    // Retrieve the exposed thread object.
    OBJECTREF pCurThreadObject = GetExposedObject();
    _ASSERTE(pCurThreadObject!=NULL);

    // Set the culture property on the thread.
    THREADBASEREF pThreadBase = (THREADBASEREF)(pCurThreadObject);
    CallPropertySet(bUICulture 
                    ? METHOD__THREAD__SET_UI_CULTURE
                    : METHOD__THREAD__SET_CULTURE,
                    (OBJECTREF)pThreadBase, CultureObj);
}

// The DLS hash lock should already have been taken before this call
LocalDataStore *Thread::RemoveDomainLocalStore(int iAppDomainId)
{
    HashDatum Data = NULL;
    if (m_pDLSHash) {
        if (m_pDLSHash->GetValue(iAppDomainId, &Data))
            m_pDLSHash->DeleteValue(iAppDomainId);
    }

    return (LocalDataStore*) Data;
}

void Thread::RemoveAllDomainLocalStores()
{
    // Don't bother cleaning this up if we're detaching
    if (!g_fProcessDetach)
    {
        Thread *pCurThread = GetThread();
        BOOL toggleGC = pCurThread->PreemptiveGCDisabled();
        
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();
    
        ThreadStore::LockDLSHash();

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        if (!m_pDLSHash) {
            ThreadStore::UnlockDLSHash();
            return;
        }
    }
    // The 'if' if we are in a process detach
    if (!m_pDLSHash)
        return;

    EEHashTableIteration iter;
    m_pDLSHash->IterateStart(&iter);
    while (m_pDLSHash->IterateNext(&iter))
    {
        LocalDataStore* pLDS = (LocalDataStore*) m_pDLSHash->IterateGetValue(&iter);
         _ASSERTE(pLDS);
         if (!g_fProcessDetach)
            RemoveDLSFromList(pLDS);
         delete pLDS;
    }
        
    delete m_pDLSHash;
    m_pDLSHash = NULL;

    if (!g_fProcessDetach)
        ThreadStore::UnlockDLSHash();
}

// The DLS hash lock should already have been taken before this call
void Thread::RemoveDLSFromList(LocalDataStore* pLDS)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc *removeThreadDLStoreMD = NULL;

    if (!g_fProcessDetach)
    {
        ARG_SLOT args[1] = {
            ObjToArgSlot(pLDS->GetRawExposedObject())
        };
        if (!removeThreadDLStoreMD)
            removeThreadDLStoreMD = g_Mscorlib.GetMethod(METHOD__THREAD__REMOVE_DLS);
        _ASSERTE(removeThreadDLStoreMD);
        removeThreadDLStoreMD->Call(args, METHOD__THREAD__REMOVE_DLS);
    }
}

void Thread::SetHasPromotedBytes ()
{
    m_fPromoted = TRUE;

    _ASSERTE(g_pGCHeap->IsGCInProgress() && 
             (g_pGCHeap->GetGCThread() == GetThread() ||
              GetThread() == NULL ||
              dbgOnly_IsSpecialEEThread())); // or a regular gc thread can call this API.

    if (!m_fPreemptiveGCDisabled)
    {
        if (FRAME_TOP == GetFrame())
            m_fPromoted = FALSE;
    }
}

BOOL ThreadStore::HoldingThreadStore(Thread *pThread)
{
    if (pThread)
    {
        return pThread == g_pThreadStore->m_HoldingThread
            || (g_fFinalizerRunOnShutDown
                && pThread == g_pGCHeap->GetFinalizerThread());
    }
    else
    {
        return g_pThreadStore->m_holderthreadid == GetCurrentThreadId();
    }
}
