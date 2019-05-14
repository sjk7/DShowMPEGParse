//------------------------------------------------------------------------------
// File: Parser.h
//
// Desc: defines base class for simple file parsers with single output pin
//------------------------------------------------------------------------------
// a modification of
//------------------------------------------------------------------------------
// File: Transfrm.h
//
// Desc: DirectShow base classes - defines classes from which simple 
//       transform codecs may be derived.
//
// Copyright (c) 1992-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// It assumes the codec has one input and one output stream, and has no
// interest in memory management, interface negotiation or anything else.
//
// derive your class from this, and supply Transform and the media type/format
// negotiation functions. Implement that class, compile and link and
// you're done.


#ifndef __xPARSER__
#define __xPARSER__

#include "puller.h"

// ======================================================================
// This is the com object that represents a simple transform filter. It
// supports IBaseFilter, IMediaFilter and two pins through nested interfaces
// ======================================================================

// ==================================================
// Implements the input pin
// ==================================================
class CParserInputPin : public CBasePin
{
    friend class CParserFilter;
public:
	CParserInputPin(TCHAR *pObjectName, CParserFilter * parent, HRESULT *phr, LPCWSTR pName);
	int StartSeek(){return m_puller.StartSeek();}
	HRESULT FinishSeek(int AtStart){return m_puller.FinishSeek(AtStart);}
	HRESULT CheckConnect(IPin *pPin);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin *pReceivePin);
	virtual HRESULT CheckMediaType(const CMediaType *pmt);
	// set the connection media type
	HRESULT SetMediaType(const CMediaType* mt);
	
	void GetReader(IAsyncReader ** ppIAsyncReader)
	{
		*ppIAsyncReader = m_puller.GetReader();
	}

	// required as pure virtual in CBasePin, but never called
	STDMETHODIMP BeginFlush();

	// required as pure virtual in CBasePin, but never called
	STDMETHODIMP EndFlush();

	HRESULT Active()
	{
		return m_puller.Active();
	}

	HRESULT Inactive()
	{
		return m_puller.Inactive();
	}

	// Media type
	CMediaType& CurrentMediaType() { return m_mt; };

protected:
	CParserFilter *m_pTransformFilter;
	CCritSec m_Lock;
	CPuller m_puller;
};

// ==================================================
// Implements the output pin
// ==================================================

class CParserOutputPin : public CBaseOutputPin, public CSourceSeeking 
{
    friend class CParserFilter;

protected:
    CParserFilter *m_pTransformFilter;

public:
    CParserOutputPin(
        TCHAR *pObjectName,
        CParserFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);

	~CParserOutputPin();

    // override to expose IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // --- CBaseOutputPin ------------

    STDMETHODIMP QueryId(LPWSTR * Id)
    {
        return AMGetWideString(L"Out", Id);
    }

    // Grab and release extra interfaces if required

    HRESULT CheckConnect(IPin *pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
    HRESULT DecideBufferSize(
                IMemAllocator * pAlloc,
                ALLOCATOR_PROPERTIES *pProp);

    // returns the preferred formats for a pin
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // inherited from IQualityControl via CBasePin
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // --- CSourceSeeking ------------

	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();

    // --- My own stuff ------------
	void SetDuration(LONGLONG rtDuration);

	// Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };
};

class AM_NOVTABLE CParserFilter : public CBaseFilter
{

public:

	// map getpin/getpincount for base enum of pins to owner
	// override this to return more specialised pin objects

	virtual int GetPinCount();
	virtual CBasePin * GetPin(int n);
	STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);

	// override state changes to allow derived transform filter
	// to control streaming start/stop
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

public:

	CParserFilter(TCHAR *, LPUNKNOWN, REFCLSID clsid);
	~CParserFilter();

	// =================================================================
	// ----- override these bits ---------------------------------------
	// =================================================================

	// These must be supplied in a derived class

	// check if you can support mtIn
	virtual HRESULT CheckInputType(const CMediaType* mtIn) PURE;

	// check if you can support the transform from this input to this output
	virtual HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut) PURE;

	// this goes in the factory template table to create new instances
	// static CCOMObject * CreateInstance(LPUNKNOWN, HRESULT *);

	// call the SetProperties function with appropriate arguments
	virtual HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop) PURE;

	// override to suggest OUTPUT pin media types
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) PURE;

	virtual	HRESULT FillSample(IAsyncReader* pReader, IMediaSample* pSample) PURE;

	virtual HRESULT Seek(REFERENCE_TIME rtStart) PURE;

	// =================================================================
	// ----- Optional Override Methods           -----------------------
	// =================================================================

	// you can also override these if you want to know about streaming
	virtual HRESULT StartStreaming();
	virtual HRESULT StopStreaming();

	// override if you can do anything constructive with quality notifications
	virtual HRESULT AlterQuality(Quality q);

	// override this to know when the media type is actually set
	virtual HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

	// chance to grab extra interfaces on connection
	virtual HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
	virtual HRESULT BreakConnect(PIN_DIRECTION dir);
	virtual HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);

	// chance to customize the transform process
	virtual HRESULT Receive(IMediaSample *pSample);

	// if you override Receive, you may need to override these three too
	virtual HRESULT EndOfStream(void);
	virtual HRESULT BeginFlush(void);
	virtual HRESULT EndFlush(void);

#ifdef PERF
	// Override to register performance measurement with a less generic string
	// You should do this to avoid confusion with other filters
	virtual void RegisterPerfId()
	{
		m_idTransform = MSR_REGISTER(TEXT("Parser"));
	}
#endif // PERF


// implementation details

protected:


#ifdef PERF
    int m_idTransform;                 // performance measuring id
#endif
	BOOL m_bEOSDelivered;              // have we sent EndOfStream
	BOOL m_bSampleSkipped;             // Did we just skip a frame
	BOOL m_bQualityChanged;            // Have we degraded?

	// critical section protecting filter state.

	CCritSec m_csFilter;

	// critical section stopping state changes (ie Stop) while we're
	// processing a sample.
	//
	// This critical section is held when processing
	// events that occur on the receive thread - Receive() and EndOfStream().
	//
	// If you want to hold both m_csReceive and m_csFilter then grab
	// m_csFilter FIRST - like CParserFilter::Stop() does.

	CCritSec m_csReceive;

	// these hold our input and output pins

	friend class CParserInputPin;
	friend class CParserOutputPin;
	CParserInputPin *m_pInput;
	CParserOutputPin *m_pOutput;
};

#endif /* __xPARSER__ */


