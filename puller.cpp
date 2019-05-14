// a modification of
//------------------------------------------------------------------------------
// File: PullPin.cpp
//
// Desc: DirectShow base classes - implements CPullPin class that pulls data
//       from IAsyncReader.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <streams.h>
#include "parser.h"
#include "puller.h"

CPuller::CPuller(CParserFilter * parent)
  : m_pReader(NULL),
    m_pAlloc(NULL),
    m_State(TM_Exit),
	m_parent(parent)
{

}

CPuller::~CPuller() {
    Disconnect();
}

// returns S_OK if successfully connected to an IAsyncReader interface
// from this object
// Optional allocator should be proposed as a preferred allocator if
// necessary
HRESULT
CPuller::Connect(IUnknown* pUnk, IMemAllocator* pAlloc, BOOL bSync) {
    CAutoLock lock(&m_AccessLock);

    if(m_pReader) {
        return VFW_E_ALREADY_CONNECTED;
    }

    HRESULT hr = pUnk->QueryInterface(IID_IAsyncReader, (void**)&m_pReader);
    if(FAILED(hr)) {
        return(hr);
    }

    hr = DecideAllocator(pAlloc, NULL);
    if(FAILED(hr)) {
        Disconnect();
        return hr;
    }

    return S_OK;
}

// disconnect any connection made in Connect
HRESULT
CPuller::Disconnect() {
    CAutoLock lock(&m_AccessLock);

    StopThread();

    if(m_pReader) {
        m_pReader->Release();
        m_pReader = NULL;
    }

    if(m_pAlloc) {
        m_pAlloc->Release();
        m_pAlloc = NULL;
    }

    return S_OK;
}

// agree an allocator using RequestAllocator - optional
// props param specifies your requirements (non-zero fields).
// returns an error code if fail to match requirements.
// optional IMemAllocator interface is offered as a preferred allocator
// but no error occurs if it can't be met.
HRESULT
CPuller::DecideAllocator(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * pProps) {
    ALLOCATOR_PROPERTIES *pRequest;
    ALLOCATOR_PROPERTIES Request;
    if (pProps == NULL) {
		HRESULT hr = m_parent->DecideBufferSize(pAlloc, &Request); 
		if FAILED(hr)
			return hr;
        pRequest = &Request;
    }
    else {
        pRequest = pProps;
    }
    HRESULT hr = m_pReader->RequestAllocator(pAlloc,
        pRequest,
        &m_pAlloc);
    return hr;
}

// start pulling data
HRESULT
CPuller::Active(void) {
    ASSERT(!ThreadExists());
    return StartThread();
}

// stop pulling data
HRESULT
CPuller::Inactive(void) {
    StopThread();

    return S_OK;
}

int CPuller::StartSeek() 
{
    CAutoLock lock(&m_AccessLock);

    ThreadMsg AtStart = m_State;

    if(AtStart == TM_Start) 
	{
        BeginFlush();
        PauseThread();
        EndFlush();
    }

    return AtStart;
}

HRESULT CPuller::FinishSeek(int AtStart) 
{
    CAutoLock lock(&m_AccessLock);

    HRESULT hr = S_OK;
    if(AtStart == TM_Start) 
	{
        hr = StartThread();
    }

    return hr;
}

HRESULT
CPuller::StartThread() {
    CAutoLock lock(&m_AccessLock);

    if(!m_pAlloc || !m_pReader) {
        return E_UNEXPECTED;
    }

    HRESULT hr;
    if(!ThreadExists()) {

        // commit allocator
        hr = m_pAlloc->Commit();
        if(FAILED(hr)) {
            return hr;
        }

        // start thread
        if(!Create()) {
            return E_FAIL;
        }
    }

    m_State = TM_Start;
    hr = (HRESULT) CallWorker(m_State);
    return hr;
}

HRESULT
CPuller::PauseThread() {
    CAutoLock lock(&m_AccessLock);

    if(!ThreadExists()) {
        return E_UNEXPECTED;
    }

    // need to flush to ensure the thread is not blocked
    // in WaitForNext
    HRESULT hr = m_pReader->BeginFlush();
    if(FAILED(hr)) {
        return hr;
    }

    m_State = TM_Pause;
    hr = CallWorker(TM_Pause);

    m_pReader->EndFlush();
    return hr;
}

HRESULT
CPuller::StopThread() {
    CAutoLock lock(&m_AccessLock);

    if(!ThreadExists()) {
        return S_FALSE;
    }

    // need to flush to ensure the thread is not blocked
    // in WaitForNext
    HRESULT hr = m_pReader->BeginFlush();
    if(FAILED(hr)) {
        return hr;
    }

    m_State = TM_Exit;
    hr = CallWorker(TM_Exit);

    m_pReader->EndFlush();

    // wait for thread to completely exit
    Close();

    // decommit allocator
    if(m_pAlloc) {
        m_pAlloc->Decommit();
    }

    return S_OK;
}


DWORD
CPuller::ThreadProc(void) {
    while(1) {
        DWORD cmd = GetRequest();
        switch(cmd) {
            case TM_Exit:
                Reply(S_OK);
                return 0;

            case TM_Pause:
                // we are paused already
                Reply(S_OK);
                break;

            case TM_Start:
                Reply(S_OK);
                Process();
                break;
        }

        // at this point, there should be no outstanding requests on the
        // upstream filter.
        // We should force begin/endflush to ensure that this is true.
        // !!!Note that we may currently be inside a BeginFlush/EndFlush pair
        // on another thread, but the premature EndFlush will do no harm now
        // that we are idle.
        m_pReader->BeginFlush();
        CleanupCancelled();
        m_pReader->EndFlush();
    }
}

void CPuller::Process() 
{
    BOOL bDiscontinuity = TRUE;

    DWORD dwRequest;
	bool bContinue(true);
    while (bContinue) 
	{
        // Break out without calling EndOfStream if we're asked to
        // do something different
        if(CheckRequest(&dwRequest)) {
            return;
        }

        IMediaSample* pSample;

        HRESULT hr = m_pAlloc->GetBuffer(&pSample, NULL, NULL, 0);
        if(FAILED(hr)) {
            OnError(hr);
            return;
        }

        if(bDiscontinuity) {
            pSample->SetDiscontinuity(TRUE);
            bDiscontinuity = FALSE;
        }

        hr = m_parent->FillSample(m_pReader, pSample);
        if(FAILED(hr)) {
            pSample->Release();
            OnError(hr);
            return;
        }
		if (hr == S_FALSE)
			bContinue = false;

	    hr = Receive(pSample);
		pSample->Release();

        if(hr != S_OK) {
            if(FAILED(hr)) {
                OnError(hr);
            }
            return;
        }
    }

    EndOfStream();
}

// after a flush, cancelled i/o will be waiting for collection
// and release
void
CPuller::CleanupCancelled(void) {
    while(1) {
        IMediaSample * pSample;
        DWORD_PTR dwUnused;

        HRESULT hr = m_pReader->WaitForNext(0,          // no wait
            &pSample,
            &dwUnused);
        if(pSample) {
            pSample->Release();
        }
        else {
            // no more samples
            return;
        }
    }
}

// =================================================================
// Implements the CPuller class
// =================================================================

HRESULT CPuller::Receive(IMediaSample *pIn)
{
    return m_parent->Receive(pIn);
}

HRESULT CPuller::EndOfStream()
{
	//Use this method to call IPin::EndOfStream on each downstream input pin that receives data from this object. 
	//If your filter's output pin(s) derive from CBaseOutputPin, call the CBaseOutputPin::DeliverEndOfStream method.
    return m_parent->EndOfStream();
}

HRESULT CPuller::BeginFlush()
{
	// The CPuller::Seek method calls this method. Implement this method to call the IPin::BeginFlush method on 
	// each downstream input pin that receives data from this object. If your filter's output pin(s) derive from 
	// CBaseOutputPin, call the CBaseOutputPin::DeliverBeginFlush method.
	// This design enables the filter to seek the stream simply by calling Seek on the CPuller object.
	return m_parent->BeginFlush();
}

HRESULT CPuller::EndFlush()
{
	// The CPuller::Seek method calls this method. Implement this method to call the IPin::EndFlush method on 
	// each downstream input pin that receives data from this object. If your filter's output pin(s) derive from 
	// CBaseOutputPin, call the CBaseOutputPin::DeliverEndFlush method.
	// This design enables the filter to seek the stream simply by calling Seek on the CPuller object.
	return m_parent->EndFlush(); 
}

void CPuller::OnError(HRESULT hr)
{
	// The object calls this method whenever an error occurs that halts the data-pulling thread. The filter can use 
	// this method to recover from streaming errors gracefully. In most cases, the error is returned from the upstream 
	// filter, so the upstream filter is responsible for reporting it to the Filter Graph Manager. If the error occurs 
	// inside the CPuller::Receive method, your filter should send an EC_ERRORABORT event. (See IMediaEventSink::Notify.)
	// TO DO
	return; 
}

