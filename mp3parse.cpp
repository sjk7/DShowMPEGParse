#include "mp3parse.h"
#include <initguid.h>
#include "guidmp3p.h"

#include <atlbase.h>


//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
//extern CComModule _Module; // prevent compiler ,moaning!
//#include <atlcom.h>


///////////////////////////////////////////////////////////////////////////////
const AMOVIESETUP_MEDIATYPE sudInputTypes[] = {
	{ &MEDIATYPE_Stream, &CLSID_mpeg1audio },
};

const AMOVIESETUP_MEDIATYPE sudAudioTypes[] = {
	{ &MEDIATYPE_Audio, &CLSID_mp3 },
};

const AMOVIESETUP_PIN sudPins[] = {
	{
		NULL,                // unused
		FALSE,               // bRendered
		FALSE,               // bOutput
		FALSE,               // bZero
		FALSE,               // bMany
		NULL,                // unused
		NULL,                // unused
		1,                   // nTypes
		sudInputTypes,       // lpTypes
	},{     
		NULL,                // unused
		FALSE,               // bRendered
		TRUE,                // bOutput
		TRUE,                // bZero
		TRUE,                // bMany
		NULL,                // unused
		NULL,                // unused
		1,                   // nTypes
		sudAudioTypes,
	},
}; 

const AMOVIESETUP_FILTER sudMp3Parser = {
	&CLSID_CMp3Parser,
	L"MP3 Parser: Improved",
	MERIT_NORMAL+1,
	sizeof(sudPins)/sizeof(AMOVIESETUP_PIN),
        sudPins,
};                     

CFactoryTemplate g_Templates[] = {
	{
		L"MP3 Parser Filter: Improved",
		&CLSID_CMp3Parser,
		CMP3ParseFilter::CreateInstance,
		NULL,
		&sudMp3Parser,
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );
}
///////////////////////////////////////////////////////////////////////////////

HRESULT CMP3ParseFilter::Seek(REFERENCE_TIME rtStart)
{
	// FIXME
	return S_OK;
	if (frameinfo_inited != true){
		return S_FALSE;
	}

	if (m_pInput == NULL){
		return E_UNEXPECTED;
	}

	if (m_pInput->IsConnected() == FALSE){
		return E_UNEXPECTED;
	}

	int AtStart = m_pInput->StartSeek();

	rtMediaStart = rtMediaStop = m_rtStart = rtStart;
//	m_llCurrent = time2address(m_rtStart);
	currentframe = time2frame(m_rtStart);

	seek_occured = true;

	return m_pInput->FinishSeek(AtStart);
}

HRESULT CMP3ParseFilter::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin) 
{ 
	if (1/*direction == PINDIR_INPUT*/){

		if(checked_input == false){			//getmp3infoでチェックされてない。
			return S_OK;
		}
		
		//すでに初期化済みなら正常終了させる
		if(frameinfo_inited == true){
//			m_pOutput->SetDuration(info.duration);
//			Seek(0);
			return S_OK;
		}
		
		// FIXME
		// getframeinfo();
		frameinfo_inited = true;		//frameinfoは初期化された。
		if (m_pOutput->IsConnected()){
		
			HRESULT hr = m_pGraph->Reconnect(m_pOutput);
			if (FAILED(hr)){
				return hr;
			}
		
		}
		m_pOutput->SetDuration(info.duration);
		Seek(0);

	}

	return S_OK; 
}
HRESULT CMP3ParseFilter::FillSample(IAsyncReader* pReader, IMediaSample* pSample)
{
	unsigned long head;
	int bitrate_index,lay,padding;
	long framesize;
	long data_used = 0;
	double time_used = 0.0;
	
	long sample_max = pSample->GetSize();
	PBYTE sampleBuffer(NULL);
	HRESULT hr = pSample->GetPointer(&sampleBuffer);
	if (FAILED(hr) || sampleBuffer == NULL)
		return E_FAIL;

	int isEOF = 0;
	return S_FALSE;

//	j = 0;
//	LONGLONG fpos = 0;
	while(1){

		ASSERT(currentframe);
		if(ReaderRead(currentframe->offset,4,(BYTE *)&head,pReader) != S_OK){
			isEOF = 1;
			break;
		}

		if(!head_check(head = BSwap(head))){
			isEOF = 1;
			break;
		}
		
	    bitrate_index = ((head>>12)&0xf);
		padding       = ((head>>9)&0x1);
		lay           = 4-((head>>17)&3);
		framesize = tabsel_123[info.lsf][lay-1][bitrate_index]*144000/(info.freq<<info.lsf)+padding;
		
		if(sample_max - data_used < currentframe->size) {
			break;
		}
		
		if(ReaderRead(currentframe->offset,currentframe->size,(BYTE *)(sampleBuffer+data_used),pReader) == S_FALSE){
			isEOF = 1;
			break;
		}

		data_used += currentframe->size;
		time_used += (double)(framesize * 8) / (tabsel_123[info.lsf][lay-1][bitrate_index] * 1000);

		if(currentframe->next != NULL){
			currentframe = currentframe->next;
		}
		else {
			isEOF = 1;
			break;
		}
	}

//	m_llCurrent += data_used;
	pSample->SetActualDataLength(data_used);

	rtMediaStart = rtMediaStop;
	rtMediaStop  = (LONGLONG)(time_used * UNITS) + rtMediaStart;
	pSample->SetMediaTime(&rtMediaStart, &rtMediaStop);
	REFERENCE_TIME rtStart = rtMediaStart - m_rtStart;
	REFERENCE_TIME rtStop  = rtMediaStop  - m_rtStart;
	pSample->SetTime(&rtStart, &rtStop);

	if(seek_occured == true){
		pSample->SetDiscontinuity(TRUE);
		seek_occured = false;
	}
	else {
		pSample->SetDiscontinuity(FALSE);
	}
	pSample->SetSyncPoint(TRUE);
	pSample->SetPreroll(FALSE);

	return isEOF ? S_FALSE : S_OK;
}


// ------------------------------------------------------------------------
// constructor
//
CMP3ParseFilter::CMP3ParseFilter(LPUNKNOWN pUnk, HRESULT *phr) :
	CParserFilter(NAME("MP3 Parser Filter: Improved"), pUnk, CLSID_CMp3Parser)
{
	Init();
}

// ------------------------------------------------------------------------
// destructor
//
CMP3ParseFilter::~CMP3ParseFilter()
{
}

void CMP3ParseFilter::Init()
{
//	m_llCurrent = 0;				//ソースファイル上の現在の位置
	currentframe = NULL;			//現在再生中のフレームの情報を入れた構造体へのアドレス。
	m_rtStart = 0;					//シーク後の開始位置

	rtMediaStart = 0;				//処理しているサンプルの最初の時間
	rtMediaStop = 0;				//処理しているサンプルの終わりの時間

	frameinfo_inited = false;		//frameinfoはまだ初期化していない。
	checked_input = false;			//getmp3infoでチェックされてない。
	seek_occured = false;
}

CUnknown * WINAPI CMP3ParseFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    
	return new CMP3ParseFilter(pUnk, phr);
}

//
// CMP3ParseFilter::CheckTransform
//
HRESULT CMP3ParseFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    HRESULT hr;
    if (FAILED(hr = CheckInputType(mtIn))) 
	{
		return hr;
    }

    return S_OK;
} // CheckTransform

//
// CheckInputType
//
HRESULT CMP3ParseFilter::CheckInputType(const CMediaType* mtIn)
{
    HRESULT hr;

	if (IsEqualGUID(*mtIn->Type(), MEDIATYPE_Stream) && IsEqualGUID(*mtIn->Subtype(), MEDIASUBTYPE_MPEG1Audio)){

		//すでに初期化済みなら正常終了させる
		if(checked_input == true){			//getmp3infoでチェック済み
			return S_OK;
		}
	
		hr = getmp3info();
		if (hr != S_OK){
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		checked_input = true;
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

void CMP3ParseFilter::BuildWaveFormatEx(PWAVEFORMATEX * ppwfe,DWORD * pdwLength)
{
	PWAVEFORMATEX p = PWAVEFORMATEX(malloc(sizeof(WAVEFORMATEX)+MPEGLAYER3_FLAG_PADDING_ON));

	p->wFormatTag = 0x55;
	p->nChannels = info.nch;
	p->nSamplesPerSec = info.freq;
	p->nBlockAlign = 4;
	p->nAvgBytesPerSec = (DWORD)(info.nbytes * UNITS / info.duration);
	p->wBitsPerSample = 16;
	p->cbSize = MPEGLAYER3_FLAG_PADDING_ON;

	*pdwLength = (sizeof(WAVEFORMATEX)+MPEGLAYER3_FLAG_PADDING_ON);
	*ppwfe = p;
}

//
// GetMediaType
//
HRESULT CMP3ParseFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	if (iPosition < 0){
		return E_INVALIDARG;
	}

	if (iPosition > 0){
		return VFW_S_NO_MORE_ITEMS;
	}

	PWAVEFORMATEX pwfe;
	DWORD dwLength;
	BuildWaveFormatEx(&pwfe, &dwLength);
	pMediaType->SetType(&MEDIATYPE_Audio);
	pMediaType->SetSubtype(&CLSID_mp3);
//	pMediaType->SetSubtype(&FOURCCMap(WAVE_FORMAT_MPEGLAYER3));
	pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
	pMediaType->SetFormat((BYTE *)pwfe, dwLength);
	::free(pwfe);

	return NOERROR;
}

//
// DecideBufferSize
//
HRESULT CMP3ParseFilter::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
/*	//入力が接続されていなければ強制終了
	if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }
*/
	pProperties->cbAlign = 1;
	pProperties->cbBuffer = 8000;
	pProperties->cbPrefix = 0;
	pProperties->cBuffers = 10;

if (pAlloc)
{
	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;



	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer)
	{
		return E_FAIL;
	}
}

	return NOERROR;
}

HRESULT CMP3ParseFilter::BreakConnect(PIN_DIRECTION dir)
{
	if (dir == PINDIR_INPUT){
		if(frameinfo_inited == true){
			FRAMEINFO * _frameinfo,* nextframeinfo;

			_frameinfo = frameinfo.next;

			while(1){
				if(_frameinfo == NULL) break;
				nextframeinfo = _frameinfo->next;
				delete _frameinfo;
				_frameinfo = nextframeinfo;
			}

			frameinfo.next = NULL;

			Init();
		}
	}

	return S_OK;
}

IAsyncReader * CMP3ParseFilter::ReaderOpen(void)
{
	//IAsyncReader * pIAsyncReader(NULL);
	//m_pInput->GetReader(&pIAsyncReader);
	//ASSERT(m_pInput);
	IAsyncReader* p = NULL;
	m_pInput->GetReader(&p);
	ASSERT(p);
	return p;

}
HRESULT CMP3ParseFilter::ReaderLength(LONGLONG *pTotal,IAsyncReader *reader)
{
	LONGLONG pAvailable;
	return reader->Length(pTotal,&pAvailable);
}
HRESULT CMP3ParseFilter::ReaderRead(LONGLONG llPosition,LONG llength,void *pBuffer,IAsyncReader *reader)
{
	return reader->SyncRead(llPosition,llength,(BYTE *)pBuffer);
}
HRESULT CMP3ParseFilter::ReaderClose(IAsyncReader *reader)
{
	reader->Release();
	return S_OK;
}




// return the position of the first mp3 audio header
long CMP3ParseFilter::read_tags(mp3info& info, IAsyncReader *reader, const LONGLONG Total){
	
	long hpos = 0;
	// getid3v2:
	{
		unsigned char c1,c2,c3,c4;

		ReaderRead(hpos,1,&c1,reader);	hpos ++;
		ReaderRead(hpos,1,&c2,reader);	hpos ++;
		ReaderRead(hpos,1,&c3,reader);	hpos ++;
		ReaderRead(hpos,1,&c4,reader);	hpos ++;

		if (c1 == 'I' && c2 == 'D' && c3 == '3'/* && c4 == 2*/) {
			hpos = 6;
			ReaderRead(hpos,1,&c1,reader);	hpos ++;
			ReaderRead(hpos,1,&c2,reader);	hpos ++;
			ReaderRead(hpos,1,&c3,reader);	hpos ++;
			ReaderRead(hpos,1,&c4,reader);	hpos ++;
			hpos = c1*2097152+c2*16384+c3*128+c4;
			static const int ID3_HEADER_SIZE = 10;
			hpos += ID3_HEADER_SIZE;
		}
		else hpos = 0;
	}

	// get id3v1:
		// read ID3 tag
	ReaderRead(Total-128,128,(BYTE *)&id3,reader);

	if (strncmp(id3.tag,"TAG",3) == 0) {
		info.nbytes = info.nbytes - 128;
		info.payload_size -= 128;
		ASSERT(info.payload_size > 0);
		
		info.payload_end_postiion -= 128;
		ASSERT(info.payload_end_postiion > 0);
		
	}

	return hpos;
}

enum parse_mp3_info_result{
	PARSE_MP_ERROR_NONE = 0,
	PARSE_MP_ERROR_FAILED_FIRST_CHECK,
	PARSE_MP_ERROR_NOT_MP3,
	PARSE_MP_ERROR_HEAD_CHECK_FAILED,
	PARSE_MP_ERROR_BAD_FREQ_INDEX,
	PARSE_MP_ERROR_BAD_LSF,
	PARSE_MP_ERROR_BAD_SECOND_INDEX,
	PARSE_MP_ERROR_BAD_BITRATE_INDEX

};

// returns a positive frame size, or some negative parse_mp3_info_result
int parse_mp3_header(unsigned long hpos, mp3info& info, const unsigned long head, const unsigned long consec_frames)
{
	

	info.hpos = hpos;
	info.framesize = 0;

	const long PICTURE_START_CODE = 0x00000100;
	// MPEG ヘッダ検出で終了。
	// なぜかMPEGシステムの時も接続されそうになるのでその対策
	{
		if((head & 0xffffff00) == 0x00000100){
			return -PARSE_MP_ERROR_FAILED_FIRST_CHECK;
		}
	}

	{
		//MP3 のヘッダとよく似ているが、Layer-4は存在しない。
		if ((head & 0xfff00000) == 0xfff00000 && 4-((head>>17)&0x3) == 4){
			if (consec_frames >= 2)
				ASSERT("NOT LAYER THREE" == NULL);
			return -PARSE_MP_ERROR_NOT_MP3;
		}
	}

	if (!head_check(head)){
		return -PARSE_MP_ERROR_HEAD_CHECK_FAILED;
	}


	
	int bitrate_index,mode,nch,lay,extension,mpeg25,padding, lsf,srate;

	
	

    if( head & (1<<20) ) {
		lsf = (head & (1<<19)) ? 0x0 : 0x1;
		mpeg25 = 0;
    } else {
		lsf = 1;
		mpeg25 = 1;
    }

    bitrate_index = ((head>>12)&0xf);
	padding       = ((head>>9)&0x1);
    mode          = ((head>>6)&0x3);
    nch           = (mode == MPG_MD_MONO) ? 1 : 2;
	lay           = 4-((head>>17)&3);
	extension     = ((head>>8)&0x1);

	if(mpeg25) srate = 6 + ((head>>10)&0x3);
    else srate = ((head>>10)&0x3) + (lsf*3);

	if (lay != 3)
	{
		ASSERT("MPEG Layer is not THREE" == NULL);
		return -PARSE_MP_ERROR_NOT_MP3;

	}

	info.lsf  = lsf;
	static const unsigned int FREQ_MAX = 9;
	// freq[9]
	if (srate >= FREQ_MAX){
		return -PARSE_MP_ERROR_BAD_FREQ_INDEX;
	}
	info.freq = freqs[srate];
	info.nch  = nch;
	
	
	unsigned int tablindex_1 = lsf;
	unsigned int tablindex_2 = 3-1;
	unsigned int tablindex_3 = bitrate_index;

	if (tablindex_1 >=2){
		return -PARSE_MP_ERROR_BAD_LSF;
	}
	if (tablindex_2 >= 3){
		return -PARSE_MP_ERROR_BAD_SECOND_INDEX;
	}
	if (tablindex_2 >= 16){
		return -PARSE_MP_ERROR_BAD_BITRATE_INDEX;
	}

	info.framesize = tabsel_123[tablindex_1][tablindex_2][tablindex_3]*144000/(info.freq<<lsf)+padding;
	if (info.duration == 0){
		// FIXME: each frame should have its *own* duration, not just the first frame!
		double dur = (double)info.framesize * 8.0 / (tabsel_123[lsf][3-1][bitrate_index] * 1000);
		info.duration = dur * UNITS;
	}
	info.frames++;
	return info.framesize;
}


HRESULT CMP3ParseFilter::getmp3info(void)
{
	unsigned long head = 0;
	memset(&info, 0, sizeof(info));
	mp3info first_header = {0};


	IAsyncReader *reader = ReaderOpen();
	if (!reader){
		return E_OUTOFMEMORY;
	}
	
	LONGLONG Total = 0;
	HRESULT hr = ReaderLength(&Total,reader);
	if (FAILED(hr)) {
		return hr;
	}
	
	info.nbytes = Total;
	info.payload_size = Total;
	info.payload_end_postiion = Total;

	LONGLONG hpos = (LONGLONG)read_tags(info, reader, Total);

	bool FindHeader = false;
	long approx_frames = -1;
	

	info.hpos = hpos;

	int consec_frames = 0;
	static const int NFRAMES_CHECK = 4;
	static const int SCAN_BYTES = 65536;
	int i = 0;
	int tries = 0;

	while (i < SCAN_BYTES) {
		hr = ReaderRead(hpos+i,4,(BYTE *)&head,reader);
		if(hr == S_FALSE) 
		{
			break; // eos
		}

		head = BSwap(head);
		
		int parse_result = parse_mp3_header(hpos+i, info, head, consec_frames);
		if (parse_result > 0){
			consec_frames++;
			i += info.framesize;
			if (info.frames == 1){
				first_header = info;
				info.payload_size -=hpos;
				ASSERT(info.payload_size > 0);
				
			}
			
			ASSERT(info.framesize && "here comes a divide by zero fatal error ...");
			if (approx_frames == 0){
				approx_frames = info.payload_size / info.framesize;
			}
			if (consec_frames >= NFRAMES_CHECK){
				FindHeader = true;
				if (tries -1 > NFRAMES_CHECK){
					ATLTRACE("Finally! Found first header at header position: %ld after %ld tries.\n", (long)first_header.hpos, (long)tries);
				}
				break;
			}
			
		}else{

			ATLTRACE("frame, at index %d and header file position: %ld not where expected\n", info.frames, (long)hpos+i); 
			approx_frames = 0;
			i++;
			consec_frames = 0;
			first_header.framesize = 0;
			info.frames = 0;
		}
		
		tries++;
		
	}; // for loop

	if(FindHeader == false) {
		ReaderClose(reader);
		return S_FALSE;
	}
	
	approx_frames = info.payload_size / first_header.framesize;
	LONGLONG duration = info.duration * approx_frames;
	info.duration = duration;
	hpos = first_header.hpos;
	ReaderClose(reader);

	return S_OK; 
}



HRESULT CMP3ParseFilter::getframeinfo(void)
{
	/*フレームを走査し、時間とオフセットを記録する。*/
	IAsyncReader *reader;
	LONGLONG fpos;
	DWORD framesize;
	double duration;
	FRAMEINFO * _frameinfo,* preframeinfo;
	unsigned long head;
	int i;
	HRESULT hr;
	int lsf,srate;
	int bitrate_index,mode,nch,extension,mpeg25,padding;

	LONGLONG filesize = -1;
	LONGLONG avail = -1;


	reader = ReaderOpen();

	hr = reader->Length(&filesize, &avail); 
	ASSERT(hr == S_OK);
	if (FAILED(hr)) return hr;

	fpos = info.hpos;
	duration = 0;

	/*先頭にあたる frameinfo はダミー*/
	frameinfo.time = -1;
	frameinfo.offset = -1;
	frameinfo.size = -1;
	frameinfo.before = NULL;
	frameinfo.next = NULL;
	
	FRAMEINFO first_frame = {0};


	preframeinfo = &frameinfo;

	int j = 0 ;						//エンコーダの不正でフレームサイズが
								//正しくない場合の補正カウンタ。
								//-1 ~ 1
	bool supplementary_framesize = false;

	for(i=0;;i++){
		hr = ReaderRead(fpos,4,&head,reader);
		if(hr != S_OK){ 
			if (hr == S_FALSE){
				//eos
			}
			break;
		}

		if(!head_check(head = BSwap(head))){//フレームヘッダが見つからない時は、
											// +-1の範囲をチェック
			if(supplementary_framesize == false){
				supplementary_framesize = true;
				fpos -= 1;
				j = -1;
				continue;
			}
			if(j > 1) {
				break;
			}
			fpos ++;
			j ++;
			continue;
		}

	    if( head & (1<<20) ) {
			lsf = (head & (1<<19)) ? 0x0 : 0x1;
			mpeg25 = 0;
		} else {
			lsf = 1;
			mpeg25 = 1;
	    }
	    bitrate_index = ((head>>12)&0xf);
		padding       = ((head>>9)&0x1);
	    mode          = ((head>>6)&0x3);
	    nch           = (mode == MPG_MD_MONO) ? 1 : 2;
		extension     = ((head>>8)&0x1);

		if(mpeg25) srate = 6 + ((head>>10)&0x3);
	    else srate = ((head>>10)&0x3) + (lsf*3);

		framesize = tabsel_123[lsf][3-1][bitrate_index]*144000/(freqs[srate]<<lsf)+padding;
		duration += (double)framesize * 8.0 / (tabsel_123[lsf][3-1][bitrate_index] * 1000);
		//if (first_frame.size == 0){
		//	first_frame.size = framesize;
		//	info.duration = first_frame.
		//}
		/*/
		_frameinfo = new FRAMEINFO;
		_frameinfo->time = (REFERENCE_TIME)(duration * 10000000);
		_frameinfo->offset = fpos;
		_frameinfo->size = framesize;
		_frameinfo->before = preframeinfo;
		if(supplementary_framesize == true)
			preframeinfo->size += j;
		preframeinfo->next = _frameinfo;
		preframeinfo = _frameinfo;

		supplementary_framesize = false;
								//補正カウンタフラグを初期化。
		/*/
		fpos += framesize;		//次のフレームの先頭オフセットをセット。
	}

	info.duration = (REFERENCE_TIME)(duration * 10000000);

	info.frames = i;
	this->m_vec_frames.resize(info.frames);

	ReaderClose(reader);

//	m_llCurrent = info.hpos;

	return S_OK; 
}


STDMETHODIMP CMP3ParseFilter::NonDelegatingQueryInterface(REFIID riid, void **p)
{
//	if(riid == IID_ISpecifyPropertyPages){
//		return GetInterface((CLSID_CMp3ParserPropertyPage *)(this), p);
//	}
	return CBaseFilter::NonDelegatingQueryInterface(riid, p);
}
