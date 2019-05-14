// a modification of
//------------------------------------------------------------------------------
// File: PullPin.h
//
// Desc: DirectShow base classes - defines CPullPin class.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef __PULLER_H__
#define __PULLER_H__

class CParserFilter;

//
// CPuller
//
// object supporting pulling data from an IAsyncReader interface.
//
// This is essentially for use in a MemInputPin when it finds itself
// connected to an IAsyncReader pin instead of a pushing pin.
//

class CPuller : public CAMThread
{
    IAsyncReader*       m_pReader;

    enum ThreadMsg {
        TM_Pause,       // stop pulling and wait for next message
        TM_Start,       // start pulling
        TM_Exit,        // stop and exit
    };

    ThreadMsg m_State;

    // override pure thread proc from CAMThread
    DWORD ThreadProc(void);

    // running pull method 
    void Process(void);

    // clean up any cancelled i/o after a flush
    void CleanupCancelled(void);

    // suspend thread from pulling, eg during seek
    HRESULT PauseThread();

    // start thread pulling - create thread if necy
    HRESULT StartThread();

    // stop and close thread
    HRESULT StopThread();

protected:
    IMemAllocator * m_pAlloc;
	CParserFilter * m_parent;

public:
    CPuller(CParserFilter * parent);
    virtual ~CPuller();

    // returns S_OK if successfully connected to an IAsyncReader interface
    // from this object
    // Optional allocator should be proposed as a preferred allocator if
    // necessary
    // bSync is TRUE if we are to use sync reads instead of the
    // async methods.
    HRESULT Connect(IUnknown* pUnk, IMemAllocator* pAlloc, BOOL bSync);

    // disconnect any connection made in Connect
    HRESULT Disconnect();

    // agree an allocator using RequestAllocator - optional
    // props param specifies your requirements (non-zero fields).
    // returns an error code if fail to match requirements.
    // optional IMemAllocator interface is offered as a preferred allocator
    // but no error occurs if it can't be met.
    virtual HRESULT DecideAllocator(
        IMemAllocator* pAlloc,
        ALLOCATOR_PROPERTIES * pProps);

    int StartSeek(); //pause thread and flush
    HRESULT FinishSeek(int AtStart); //restart thread

    // start pulling data
    HRESULT Active(void);

    // stop pulling data
    HRESULT Inactive(void);

    // helper functions
    LONGLONG AlignDown(LONGLONG ll, LONG lAlign) {
        // aligning downwards is just truncation
        return ll & ~(lAlign-1);
    };

    LONGLONG AlignUp(LONGLONG ll, LONG lAlign) {
        // align up: round up to next boundary
        return (ll + (lAlign -1)) & ~(lAlign -1);
    };

    // GetReader returns the (addrefed) IAsyncReader interface
    // for SyncRead etc
    IAsyncReader* GetReader() {
        m_pReader->AddRef();
        return m_pReader;
    };

    // -- pure --

    // override this to handle data arrival
    // return value other than S_OK will stop data
    virtual HRESULT Receive(IMediaSample*) ;

    // override this to handle end-of-stream
    virtual HRESULT EndOfStream(void) ;

    // called on runtime errors that will have caused pulling
    // to stop
    // these errors are all returned from the upstream filter, who
    // will have already reported any errors to the filtergraph.
    virtual void OnError(HRESULT hr) ;

    // flush this pin and all downstream
    virtual HRESULT BeginFlush() ;
    virtual HRESULT EndFlush() ;
};

#endif //__PULLER_H__
