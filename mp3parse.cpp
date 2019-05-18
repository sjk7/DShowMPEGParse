#include "mp3parse.h"
#include <initguid.h>
#include "guidmp3p.h"
#include <algorithm>
#include <atlbase.h>

#ifndef TRACE
#define TRACE ATLTRACE
#endif


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
			m_pOutput->SetDuration(m_info.duration);
//			// FIXME
			//Seek(0);
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
		m_pOutput->SetDuration(m_info.duration);
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
		framesize = tabsel_123[m_info.lsf][lay-1][bitrate_index]*144000/(m_info.freq<<m_info.lsf)+padding;
		
		if(sample_max - data_used < currentframe->size) {
			break;
		}
		
		if(ReaderRead(currentframe->offset,currentframe->size,(BYTE *)(sampleBuffer+data_used),pReader) == S_FALSE){
			isEOF = 1;
			break;
		}

		data_used += currentframe->size;
		time_used += (double)(framesize * 8) / (tabsel_123[m_info.lsf][lay-1][bitrate_index] * 1000);

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
	memset(&m_info, 0, sizeof(m_info));
	memset(&m_id3, 0, sizeof(m_id3));

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
		
		IAsyncReader *reader = ReaderOpen();
		if (!reader){
			return E_OUTOFMEMORY;
		}
		hr = getmp3info(reader);
		
		ReaderClose(reader);
		
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
	mp3info& info = m_info;

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
	
	long hpos = info.hpos;;
	info.payload_size = Total;
	info.payload_end_postiion = info.payload_size;
	id3tag& id3 = m_id3;
	// getid3v2:
	int trying_again = 0;
	{
again:
		unsigned char c1,c2,c3,c4;
	LONGLONG hpos_b4 = hpos;

		ReaderRead(hpos,1,&c1,reader);	hpos ++;
		ReaderRead(hpos,1,&c2,reader);	hpos ++;
		ReaderRead(hpos,1,&c3,reader);	hpos ++;
		ReaderRead(hpos,1,&c4,reader);	hpos ++;
		
		if (c1 == 'I' && c2 == 'D' && c3 == '3'/* && c4 == 2*/) {
			 
				hpos = hpos_b4+6;;
			
				
			ReaderRead(hpos,1,&c1,reader);	hpos ++;
			ReaderRead(hpos,1,&c2,reader);	hpos ++;
			ReaderRead(hpos,1,&c3,reader);	hpos ++;
			ReaderRead(hpos,1,&c4,reader);	hpos ++;
			hpos += c1*2097152+c2*16384+c3*128+c4;
			static const int ID3_HEADER_SIZE = 10;
			hpos += ID3_HEADER_SIZE;
			info.payload_size -= hpos;
			if (info.payload_size < 0){
				return hpos_b4;
			}
		}
		else{
			
			if (!trying_again){
			std::string s;;
			s.resize(65535);
			HRESULT	hr = ReaderRead(0, 65535, &s[0], reader);
				if (hr == S_OK){
					std::transform(s.begin(), s.end(),s.begin(), ::toupper);
					int pos = s.find("ID3");
					if (pos != std::string::npos){
						hpos = pos;
						trying_again = 1;
						goto again;
					}else{
						hpos = 0;
					}
				}
			}
			hpos = 0;
		}
	}

	// get id3v1:
		// read ID3 tag
	ReaderRead(Total-128,128,(BYTE *)&id3,reader);

	if (strncmp(id3.tag,"TAG",3) == 0) {
		info.nbytes = info.nbytes - 128;
		info.payload_size -= 128;
		info.payload_end_postiion -= 128;
		
		
	}

	ASSERT(info.payload_end_postiion > 0);
	ASSERT(info.payload_size > 0);
		
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
	PARSE_MP_ERROR_BAD_BITRATE_INDEX,
	PARSE_MP_ERROR_GENERAL,
	PARSE_MP_ERROR_BAD_LAYER

};

// returns a positive frame size, or some negative parse_mp3_info_result
int parse_mp3_header(unsigned long hpos, mp3info& info, const unsigned long head, const unsigned long consec_frames)
{
	

	info.hpos = hpos;
	info.framesize = 0;


	if (!head_check(head)){
		return -PARSE_MP_ERROR_HEAD_CHECK_FAILED;
	}

	
	int bitrate_index,mode,nch,lay,extension,mpeg25,padding, lsf,srate;

	lay = 4-((head>>17)&3);
	if (lay <= 0 || lay > 3){
		return -PARSE_MP_ERROR_BAD_LAYER;
	}
		

    if( head & (1<<20) ) {
		lsf = (head & (1<<19)) ? 0x0 : 0x1;
		mpeg25 = 0;
    } else {
		lsf = 1;
		mpeg25 = 1;
    }
	info.lsf  = lsf;
		
	if (lsf >=2)
		return -PARSE_MP_ERROR_BAD_LSF;
    
	bitrate_index = ((head>>12)&0xf);
	padding       = ((head>>9)&0x1);
    mode          = ((head>>6)&0x3);
    nch           = (mode == MPG_MD_MONO) ? 1 : 2;
	extension     = ((head>>8)&0x1);


	
	static const unsigned int FREQ_MAX = 9;
	if(mpeg25) {
		srate = 6 + ((head>>10)&0x3);
	}
    else { 
		srate = ((head>>10)&0x3) + (lsf*3);
	}

	if (srate >= FREQ_MAX){
		return -PARSE_MP_ERROR_BAD_FREQ_INDEX;
	}

	info.freq = freqs[srate];
	info.nch  = nch;

	if (bitrate_index >= 16)
		return -PARSE_MP_ERROR_BAD_BITRATE_INDEX;
	
	long sz = 0;
	switch(lay)
	{
#ifndef NO_LAYER1
		case 1:
			info.bitrate = (long) tabsel_123[lsf][0][bitrate_index] ;
			info.framesize  = info.bitrate * 12000;
			info.framesize /= freqs[srate];
			info.framesize  = ((info.framesize+padding)<<2)-4;
		break;
#endif
#ifndef NO_LAYER2
		case 2:
			// fr->do_layer = do_layer2;
			info.bitrate = tabsel_123[lsf][1][bitrate_index];
			info.framesize = (144000 * info.bitrate / info.freq) + padding;

		break;
#endif
#ifndef NO_LAYER3
		case 3:
			//fr->do_layer = do_layer3;
			/*/
			if(lsf)
				fr->ssize = (fr->stereo == 1) ? 9 : 17;
			else
				fr->ssize = (fr->stereo == 1) ? 17 : 32;
			/*/
			// if(fr->error_protection)
			// fr->ssize += 2;
			info.bitrate = (long) tabsel_123[lsf][2][bitrate_index] ;
			info.framesize  = (long) info.bitrate * 144000;
			info.framesize /= freqs[srate]<<(lsf);
			info.framesize = info.framesize + padding;
		break;
#endif 
		default:
			//if(NOQUIET) error1("Layer type %i not supported in this build!", fr->lay); 
			return -PARSE_MP_ERROR_GENERAL;
	}


	if (info.duration == 0){
		// FIXME: each frame should have its *own* duration, not just the first frame!
		double dur = (double)(info.framesize * 8.0) / (info.bitrate * 1000);
		info.duration = dur * UNITS;
	}
	//
	info.frames++;
	return info.framesize;
}


HRESULT CMP3ParseFilter::getmp3info(IAsyncReader* reader, const int full_scan_starts_at)
{
	unsigned long head = 0;
	mp3info info = {0};
	mp3info first_header = {0};
	if (full_scan_starts_at >= 0){
		info.hpos = full_scan_starts_at;
	}
	
	ASSERT(reader);
	
	
	LONGLONG Total = 0;
	HRESULT hr = ReaderLength(&Total,reader);
	if (FAILED(hr)) {
		return hr;
	}
	
	info.nbytes = Total;
	info.payload_size = Total;
	info.payload_end_postiion = Total;
	LONGLONG hpos = 0;
	hpos = (LONGLONG)read_tags(info, reader, Total);
	
	if (info.payload_size < 0){

		info.payload_size = Total;
		info.payload_end_postiion = Total;
	}
	

	bool FindHeader = false;
	long approx_frames = -1;
	

	info.hpos = hpos;

	int consec_frames = 0;
	static const int NFRAMES_CHECK = 4;
	LONGLONG SCAN_BYTES = full_scan_starts_at >= 0 ? info.payload_size : 65535;
	LONGLONG i = 0;
	int tries = 0;
	int bitrate = 0;
	int fails = 0;
	
	LONGLONG vbr_dur = 0;
	int prev_bitrate =0 ;
	LONGLONG frame_accum = 0;

	while (i < SCAN_BYTES) {
		if (hpos +i >= info.payload_end_postiion){
			break;
		}
		hr = ReaderRead(hpos+i,4,(BYTE *)&head,reader);
		if(hr == S_FALSE) 
		{
			break; // eos
		}

		head = BSwap(head);
		
		int parse_result = parse_mp3_header(hpos+i, info, head, consec_frames);
		if (parse_result > 0){
			consec_frames++;
			
			if (!bitrate){
				bitrate = info.bitrate;
			}else{
				if (bitrate != prev_bitrate){
					m_info.vbr = true;	
					if (full_scan_starts_at == -1){
						return getmp3info(reader, first_header.hpos);
						
					}
				}
			}
			prev_bitrate = info.bitrate;
			i += info.framesize;
			if (info.frames == 1){
				first_header = info;
				
				
				if (full_scan_starts_at == -1){
					
					char first_frame_data[64] = {0};
					hr = ReaderRead(first_header.hpos + MPG_HEADER_SIZE, 64, &first_frame_data[0], reader);
					if (hr != S_OK){
						return hr;
					}
					std::string s(first_frame_data, 64);
					std::transform(s.begin(), s.end(),s.begin(), ::toupper);
					
					if (s.find("XING") != std::string::npos){
						m_info.vbr = true;
					}else{
						if  (s.find("VBRI") != std::string::npos){
							m_info.vbr = true;
						}
					}
					
					if (m_info.vbr){
						return getmp3info(reader, first_header.hpos);
					}
						
				}
				
			}
			
			if (full_scan_starts_at == -1){
				if (approx_frames <= 0){
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

				info.frames++;
				approx_frames = info.frames;
				vbr_dur += info.duration;
				frame_accum += info.framesize;
			}
			
		}else{

			ATLTRACE("frame, at index %d and header file position: %ld not where expected\n", info.frames, (long)hpos+i); 
			LONGLONG sz = info.payload_size;
			ATLTRACE("payload end position is: %lld\n");
			ATLTRACE("Payload size is %lld\n", sz);
			LONGLONG distance_from_end = info.payload_end_postiion - (hpos+i);
			ATLTRACE("Distance from end is: %lld\n", distance_from_end); 
			fails++;
			if (fails >= 256 && info.frames >= 4){
				return E_FAIL;
			}

			
			
			int avg_framesize = 0;
			if (info.frames){
				avg_framesize = frame_accum / info.frames;

			}

			if (avg_framesize > 0){
				if (distance_from_end < avg_framesize * fails){
					ATLTRACE("Found truncated final frame\n");
					break;
				}
			}
			if (distance_from_end < 8192){
				break;
			}
			approx_frames = 0;
			i++;
			consec_frames = 0;
			first_header.framesize = 0;
			info.frames = 0;
			frame_accum = 0;

		}
		
		tries++;
		
	}; // for loop

	if(FindHeader == false && info.frames <= 1) {

		return S_FALSE;
	}
	
	
	hpos = first_header.hpos;
	int vbr = m_info.vbr;

	if(!m_info.vbr){
		
		approx_frames = info.payload_size / first_header.framesize;
		LONGLONG duration = info.duration * approx_frames;
		info.duration = duration;
		
	}else{
		info.duration = vbr_dur;
	}
	
	m_info = info;
	m_info.vbr = vbr;
	
	const double d = (double)info.duration / (double)UNITS;
	ATLTRACE("have %f seconds\n", d);

	
	return S_OK; 
}


#ifdef USE_GET_FRAME_INFO
HRESULT CMP3ParseFilter::getframeinfo(void)
{

	IAsyncReader *reader;
	mp3info& info = m_info;
	LONGLONG fpos;
	DWORD framesize;
	double duration;
	FRAMEINFO * _frameinfo,* preframeinfo;
	(void)_frameinfo;
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


	frameinfo.time = -1;
	frameinfo.offset = -1;
	frameinfo.size = -1;
	frameinfo.before = NULL;
	frameinfo.next = NULL;
	
	FRAMEINFO first_frame = {0};


	preframeinfo = &frameinfo;

	int j = 0 ;						
	bool supplementary_framesize = false;

	for(i=0;;i++){
		hr = ReaderRead(fpos,4,&head,reader);
		if(hr != S_OK){ 
			if (hr == S_FALSE){
				//eos
			}
			break;
		}

		if(!head_check(head = BSwap(head))){
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
	// this->m_vec_frames.resize(info.frames);
	
	ReaderClose(reader);

//	m_llCurrent = info.hpos;

	return S_OK; 
}
#endif

STDMETHODIMP CMP3ParseFilter::NonDelegatingQueryInterface(REFIID riid, void **p)
{
//	if(riid == IID_ISpecifyPropertyPages){
//		return GetInterface((CLSID_CMp3ParserPropertyPage *)(this), p);
//	}
	return CBaseFilter::NonDelegatingQueryInterface(riid, p);
}
