//------------------------------------------------------------------------------
// File: Parser.cpp
//
// Desc: implements base class for simple file parsers with single output pin
//------------------------------------------------------------------------------
// a modification of
//------------------------------------------------------------------------------
// File: Transfrm.cpp
//
// Desc: DirectShow base classes - implements class for simple transform
//       filters such as video decompressors.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <streams.h>
#include <measure.h>
#include "parser.h"

// =================================================================
// Implements the CParserFilter class
// =================================================================

CParserFilter::CParserFilter(TCHAR *pName, LPUNKNOWN pUnk, REFCLSID  clsid) :
    CBaseFilter(pName,pUnk,&m_csFilter, clsid),
    m_pInput(NULL),
    m_pOutput(NULL),
    m_bEOSDelivered(FALSE),
    m_bQualityChanged(FALSE),
    m_bSampleSkipped(FALSE) 
{
#ifdef PERF
    RegisterPerfId();
#endif //  PERF
}

// destructor
CParserFilter::~CParserFilter() 
{
    // Delete the pins
    delete m_pInput;
    delete m_pOutput;
}

// return the number of pins we provide
int CParserFilter::GetPinCount() 
{
    return 2;
}

// return a non-addrefed CBasePin * for the user to addref if he holds onto it
// for longer than his pointer to us. We create the pins dynamically when they
// are asked for rather than in the constructor. This is because we want to
// give the derived class an oppportunity to return different pin objects

// We return the objects as and when they are needed. If either of these fails
// then we return NULL, the assumption being that the caller will realise the
// whole deal is off and destroy us - which in turn will delete everything.

CBasePin * CParserFilter::GetPin(int n) 
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if(m_pInput == NULL) {

        m_pInput = new CParserInputPin(NAME("Transform input pin"),
            this,              // Owner filter
            &hr,               // Result code
            L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if(m_pInput == NULL) {
            return NULL;
        }
        m_pOutput = (CParserOutputPin *)
            new CParserOutputPin(NAME("Transform output pin"),
            this,            // Owner filter
            &hr,             // Result code
            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if(m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if(n == 0) {
        return m_pInput;
    }
    else
        if(n == 1) {
        return m_pOutput;
    }
    else {
        return NULL;
    }
}

//
// FindPin
//
// If Id is In or Out then return the IPin* for that pin
// creating the pin if need be.  Otherwise return NULL with an error.
STDMETHODIMP CParserFilter::FindPin(LPCWSTR Id, IPin **ppPin) 
{
    CheckPointer(ppPin,E_POINTER);
    ValidateReadWritePtr(ppPin,sizeof(IPin *));

    if(0==lstrcmpW(Id,L"In")) {
        *ppPin = GetPin(0);
    }
    else if(0==lstrcmpW(Id,L"Out")) {
        *ppPin = GetPin(1);
    }
    else {
        *ppPin = NULL;
        return VFW_E_NOT_FOUND;
    }

    HRESULT hr = NOERROR;
    //  AddRef() returned pointer - but GetPin could fail if memory is low.
    if(*ppPin) {
        (*ppPin)->AddRef();
    }
    else {
        hr = E_OUTOFMEMORY;  // probably.  There's no pin anyway.
    }
    return hr;
}

// override these two functions if you want to inform something
// about entry to or exit from streaming state.
HRESULT CParserFilter::StartStreaming() 
{
    return NOERROR;
}

HRESULT CParserFilter::StopStreaming() 
{
    return NOERROR;
}

// override this to grab extra interfaces on connection
HRESULT CParserFilter::CheckConnect(PIN_DIRECTION dir,IPin *pPin) 
{
    UNREFERENCED_PARAMETER(dir);
    UNREFERENCED_PARAMETER(pPin);
    return NOERROR;
}

// place holder to allow derived classes to release any extra interfaces
HRESULT CParserFilter::BreakConnect(PIN_DIRECTION dir) {
    UNREFERENCED_PARAMETER(dir);
    return NOERROR;
}

// Let derived classes know about connection completion
HRESULT CParserFilter::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin) 
{
    UNREFERENCED_PARAMETER(direction);
    UNREFERENCED_PARAMETER(pReceivePin);
    return NOERROR;
}

// override this to know when the media type is really set
HRESULT CParserFilter::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt) {
    UNREFERENCED_PARAMETER(direction);
    UNREFERENCED_PARAMETER(pmt);
    return NOERROR;
}

// override this to customize the transform process
HRESULT CParserFilter::Receive(IMediaSample *pSample) 
{
	return m_pOutput->m_pInputPin->Receive(pSample);
}

// Return S_FALSE to mean "pass the note on upstream"
// Return NOERROR (Same as S_OK)
// to mean "I've done something about it, don't pass it on"
HRESULT CParserFilter::AlterQuality(Quality q) 
{
    UNREFERENCED_PARAMETER(q);
    return S_FALSE;
}

// EndOfStream received. Default behaviour is to deliver straight
// downstream, since we have no queued data. If you overrode Receive
// and have queue data, then you need to handle this and deliver EOS after
// all queued data is sent
HRESULT CParserFilter::EndOfStream() 
{
    HRESULT hr = NOERROR;
    if(m_pOutput != NULL) {
        hr = m_pOutput->DeliverEndOfStream();
    }

    return hr;
}

// enter flush state. Receives already blocked
// must override this if you have queued data or a worker thread
HRESULT CParserFilter::BeginFlush() 
{
    HRESULT hr = NOERROR;
    if(m_pOutput != NULL) {
        // block receives -- done by caller (CBaseInputPin::BeginFlush)

        // discard queued data -- we have no queued data

        // free anyone blocked on receive - not possible in this filter

        // call downstream
        hr = m_pOutput->DeliverBeginFlush();
    }
    return hr;
}

// leave flush state. must override this if you have queued data
// or a worker thread
HRESULT CParserFilter::EndFlush() 
{
    // sync with pushing thread -- we have no worker thread

    // ensure no more data to go downstream -- we have no queued data

    // call EndFlush on downstream pins
    ASSERT(m_pOutput != NULL);
    return m_pOutput->DeliverEndFlush();

    // caller (the input pin's method) will unblock Receives
}

// override these so that the derived filter can catch them
STDMETHODIMP CParserFilter::Stop() 
{
    CAutoLock lck1(&m_csFilter);
    if(m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected

    ASSERT(m_pInput == NULL || m_pOutput != NULL);
    if(m_pInput == NULL || m_pInput->IsConnected() == FALSE ||
        m_pOutput->IsConnected() == FALSE) {
        m_State = State_Stopped;
        m_bEOSDelivered = FALSE;
        return NOERROR;
    }

    ASSERT(m_pInput);
    ASSERT(m_pOutput);

    // decommit the input pin before locking or we can deadlock
    m_pInput->Inactive();

    // synchronize with Receive calls

    CAutoLock lck2(&m_csReceive);
    m_pOutput->Inactive();

    // allow a class derived from CParserFilter
    // to know about starting and stopping streaming

    HRESULT hr = StopStreaming();
    if(SUCCEEDED(hr)) {
        // complete the state transition
        m_State = State_Stopped;
        m_bEOSDelivered = FALSE;
    }
    return hr;
}

STDMETHODIMP CParserFilter::Pause() 
{
    CAutoLock lck(&m_csFilter);
    HRESULT hr = NOERROR;

    if(m_State == State_Paused) {
        // (This space left deliberately blank)
    }

    // If we have no input pin or it isn't yet connected then when we are
    // asked to pause we deliver an end of stream to the downstream filter.
    // This makes sure that it doesn't sit there forever waiting for
    // samples which we cannot ever deliver without an input connection.

    else if(m_pInput == NULL || m_pInput->IsConnected() == FALSE) {
        if(m_pOutput && m_bEOSDelivered == FALSE) {
            m_pOutput->DeliverEndOfStream();
            m_bEOSDelivered = TRUE;
        }
        m_State = State_Paused;
    }

    // We may have an input connection but no output connection
    // However, if we have an input pin we do have an output pin

    else if(m_pOutput->IsConnected() == FALSE) {
        m_State = State_Paused;
    }
    else {
        if(m_State == State_Stopped) {
            // allow a class derived from CParserFilter
            // to know about starting and stopping streaming
            CAutoLock lck2(&m_csReceive);
            hr = StartStreaming();
        }
        if(SUCCEEDED(hr)) {
            hr = CBaseFilter::Pause();
        }
    }

    m_bSampleSkipped = FALSE;
    m_bQualityChanged = FALSE;
    return hr;
}

// =================================================================
// Implements the CParserInputPin class
// =================================================================

// constructor
CParserInputPin::CParserInputPin(TCHAR *pObjectName, CParserFilter * pTransformFilter, HRESULT *phr, LPCWSTR pName) :
	m_pTransformFilter(pTransformFilter), m_puller(pTransformFilter), 
	// CBaseInputPin(pObjectName, pTransformFilter, &pTransformFilter->m_csFilter, phr, pName) 
	CBasePin(pObjectName, pTransformFilter, &m_Lock, phr, pName, PINDIR_INPUT)
{
    DbgLog((LOG_TRACE,2,TEXT("CParserInputPin::CParserInputPin")));
}

// provides derived filter a chance to grab extra interfaces
HRESULT CParserInputPin::CheckConnect(IPin *pPin) 
{
    HRESULT hr = m_pTransformFilter->CheckConnect(PINDIR_INPUT,pPin);
    if(FAILED(hr)) {
        return hr;
    }
    //return CBaseInputPin::CheckConnect(pPin);
	return m_puller.Connect(pPin, NULL, TRUE);
}

// provides derived filter a chance to release it's extra interfaces
HRESULT CParserInputPin::BreakConnect() 
{
	//  Can't disconnect unless stopped
	ASSERT(IsStopped());
	m_pTransformFilter->BreakConnect(PINDIR_INPUT);
	//return CBaseInputPin::BreakConnect();
	return m_puller.Disconnect();
}

// Let derived class know when the input pin is connected
HRESULT CParserInputPin::CompleteConnect(IPin *pReceivePin) 
{
    HRESULT hr = m_pTransformFilter->CompleteConnect(PINDIR_INPUT,pReceivePin);
    if(FAILED(hr)) {
        return hr;
    }
    //return CBaseInputPin::CompleteConnect(pReceivePin);
    return CBasePin::CompleteConnect(pReceivePin);
}

// check that we can support a given media type
HRESULT CParserInputPin::CheckMediaType(const CMediaType* pmt) 
{
    // Check the input type

    HRESULT hr = m_pTransformFilter->CheckInputType(pmt);
    if(S_OK != hr) {
        return hr;
    }

    // if the output pin is still connected, then we have
    // to check the transform not just the input format

    if((m_pTransformFilter->m_pOutput != NULL) &&
        (m_pTransformFilter->m_pOutput->IsConnected())) {
        return m_pTransformFilter->CheckTransform(pmt,
            &m_pTransformFilter->m_pOutput->CurrentMediaType());
    }
    else {
        return hr;
    }
}

// set the media type for this connection
HRESULT CParserInputPin::SetMediaType(const CMediaType* mtIn) {
    // Set the base class media type (should always succeed)
    HRESULT hr = CBasePin::SetMediaType(mtIn);
    if(FAILED(hr)) {
        return hr;
    }

    // check the transform can be done (should always succeed)
    ASSERT(SUCCEEDED(m_pTransformFilter->CheckInputType(mtIn)));

    return m_pTransformFilter->SetMediaType(PINDIR_INPUT,mtIn);
}

// enter flushing state. Call default handler to block Receives, then
// pass to overridable method in filter
STDMETHODIMP CParserInputPin::BeginFlush() 
{
    ASSERT(0);
    return m_pTransformFilter->BeginFlush();
}

// leave flushing state.
// Pass to overridable method in filter, then call base class
// to unblock receives (finally)
STDMETHODIMP CParserInputPin::EndFlush() 
{
    ASSERT(0);
	return m_pTransformFilter->EndFlush();
}

// =================================================================
// Implements the CParserOutputPin class
// =================================================================

// constructor
CParserOutputPin::CParserOutputPin(
    TCHAR *pObjectName,
    CParserFilter *pTransformFilter,
    HRESULT * phr,
    LPCWSTR pPinName): 
	CBaseOutputPin(pObjectName, pTransformFilter, &pTransformFilter->m_csFilter, phr, pPinName),
    CSourceSeeking(NAME("AAC Source"), pTransformFilter->GetOwner(), phr, CBasePin::m_pLock)
{
    DbgLog((LOG_TRACE,2,TEXT("CParserOutputPin::CParserOutputPin")));
    m_pTransformFilter = pTransformFilter;
}

// destructor
CParserOutputPin::~CParserOutputPin() 
{
    DbgLog((LOG_TRACE,2,TEXT("CParserOutputPin::~CParserOutputPin")));
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP CParserOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) 
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    // See what interface the caller is interested in.
    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) 
	{
        return CSourceSeeking::NonDelegatingQueryInterface(riid, ppv);
    } 
	else
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CParserOutputPin::ChangeStart()
{
	return m_pTransformFilter->Seek(m_rtStart);
}

HRESULT CParserOutputPin::ChangeStop()
{
	return S_OK;
}

HRESULT CParserOutputPin::ChangeRate()
{
	return S_OK;
}

// provides derived filter a chance to grab extra interfaces
HRESULT CParserOutputPin::CheckConnect(IPin *pPin) 
{
    // we should have an input connection first

    ASSERT(m_pTransformFilter->m_pInput != NULL);
    if((m_pTransformFilter->m_pInput->IsConnected() == FALSE)) {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pTransformFilter->CheckConnect(PINDIR_OUTPUT,pPin);
    if(FAILED(hr)) {
        return hr;
    }
    return CBaseOutputPin::CheckConnect(pPin);
}

// provides derived filter a chance to release it's extra interfaces
HRESULT CParserOutputPin::BreakConnect() 
{
    //  Can't disconnect unless stopped
    ASSERT(IsStopped());
    m_pTransformFilter->BreakConnect(PINDIR_OUTPUT);
    return CBaseOutputPin::BreakConnect();
}

// Let derived class know when the output pin is connected
HRESULT CParserOutputPin::CompleteConnect(IPin *pReceivePin) 
{
    HRESULT hr = m_pTransformFilter->CompleteConnect(PINDIR_OUTPUT,pReceivePin);
    if(FAILED(hr)) {
        return hr;
    }
    return CBaseOutputPin::CompleteConnect(pReceivePin);
}

// check a given transform - must have selected input type first
HRESULT CParserOutputPin::CheckMediaType(const CMediaType* pmtOut)
{
	// must have selected input first
	ASSERT(m_pTransformFilter->m_pInput != NULL);
	if((m_pTransformFilter->m_pInput->IsConnected() == FALSE))
		return E_INVALIDARG;
	return m_pTransformFilter->CheckTransform(&m_pTransformFilter->m_pInput->CurrentMediaType(), pmtOut);
}

// called after we have agreed a media type to actually set it in which case
// we run the CheckTransform function to get the output format type again
HRESULT CParserOutputPin::SetMediaType(const CMediaType* pmtOut) 
{
    HRESULT hr = NOERROR;
    ASSERT(m_pTransformFilter->m_pInput != NULL);

    ASSERT(m_pTransformFilter->m_pInput->CurrentMediaType().IsValid());

    // Set the base class media type (should always succeed)
    hr = CBasePin::SetMediaType(pmtOut);
    if(FAILED(hr)) {
        return hr;
    }

#ifdef DEBUG
    if(FAILED(m_pTransformFilter->CheckTransform(&m_pTransformFilter->
        m_pInput->CurrentMediaType(),pmtOut))) {
        DbgLog((LOG_ERROR,0,TEXT("*** This filter is accepting an output media type")));
        DbgLog((LOG_ERROR,0,TEXT("    that it can't currently transform to.  I hope")));
        DbgLog((LOG_ERROR,0,TEXT("    it's smart enough to reconnect its input.")));
    }
#endif

    return m_pTransformFilter->SetMediaType(PINDIR_OUTPUT,pmtOut);
}

// pass the buffer size decision through to the main transform class

HRESULT
CParserOutputPin::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES* pProp) {
    return m_pTransformFilter->DecideBufferSize(pAllocator, pProp);
}

// return a specific media type indexed by iPosition

HRESULT
CParserOutputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType) {
    ASSERT(m_pTransformFilter->m_pInput != NULL);

    //  We don't have any media types if our input is not connected

    if(m_pTransformFilter->m_pInput->IsConnected()) {
        return m_pTransformFilter->GetMediaType(iPosition,pMediaType);
    }
    else {
        return VFW_S_NO_MORE_ITEMS;
    }
}


// Override this if you can do something constructive to act on the
// quality message.  Consider passing it upstream as well

// Pass the quality mesage on upstream.

STDMETHODIMP
CParserOutputPin::Notify(IBaseFilter * pSender, Quality q) {
    UNREFERENCED_PARAMETER(pSender);
    ValidateReadPtr(pSender,sizeof(IBaseFilter));

    // First see if we want to handle this ourselves
    HRESULT hr = m_pTransformFilter->AlterQuality(q);
    if(hr!=S_FALSE) {
        return hr;        // either S_OK or a failure
    }

    // S_FALSE means we pass the message on.
    // Find the quality sink for our input pin and send it there

    ASSERT(m_pTransformFilter->m_pInput != NULL);

    //return m_pTransformFilter->m_pInput->PassNotify(q);
	return S_OK;
} // Notify

void CParserOutputPin::SetDuration(LONGLONG rtDuration)
{
	m_rtDuration = rtDuration;
	m_rtStart = 0;
	m_rtStop = rtDuration;
	m_tStop = rtDuration;//????
}
