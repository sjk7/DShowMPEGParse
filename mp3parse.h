#ifndef MP3PARSE_H
#define MP3PARSE_H

#include <streams.h>
#include <mmreg.h>
#include "parser.h"
#include "mp3.h"
#include <VECTOR>
#include "my_buffer.h"

typedef std::vector<FRAMEINFO> frame_vec_t;


class CMP3ParseFilter : public CParserFilter
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *pHr);
	
	frame_vec_t m_vec_frames;
	my::buffer<BYTE> m_filebuf;

	CMP3ParseFilter(LPUNKNOWN pUnk, HRESULT *pHr);
	~CMP3ParseFilter();
	
	DECLARE_IUNKNOWN;

	HRESULT CheckInputType(const CMediaType* mtIn) ;
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);

	HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin) ;
	HRESULT BreakConnect(PIN_DIRECTION dir);
	
	HRESULT FillSample(IAsyncReader* pReader, IMediaSample* pSample);

	// This has to set:
	// m_rtStart to the point the user has selected (not necessarily the start of a frame)
	// m_lCurrentFrame to the last frame that starts on or before m_rtStart
	// m_llCurrent to the start of m_lCurrentFrame
	HRESULT Seek(REFERENCE_TIME rtStart);

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **p);
private:
	long read_tags(mp3info& info, IAsyncReader *reader, const LONGLONG Total);
	void BuildWaveFormatEx(PWAVEFORMATEX * ppwfe, DWORD * pdwLength);

	void Init();
//	LONG m_lCurrentFrame;				//現在のフレームナンバー
//	REFERENCE_TIME m_rtStart;			//シーク後の開始位置
//	REFERENCE_TIME m_rtDuration;		//全体の長さ
//	int m_tagsize;
//	AACInfo m_info;
//	DWORD * m_seek_table;
//	int m_seek_table_len;
//	LONGLONG m_llSize;					//ファイルサイズ

	//ソースフィルタを同期読み込みするための関数
	IAsyncReader * ReaderOpen(void);
	HRESULT ReaderLength(LONGLONG *pAvailable,IAsyncReader *reader);
	HRESULT ReaderAvailableLength(LONGLONG *pAvailable,IAsyncReader *reader);
	HRESULT ReaderRead(LONGLONG llPosition,LONG llength,void *pBuffer,IAsyncReader *reader);
	HRESULT ReaderClose(IAsyncReader *reader);

	// finds the first frame, and gets tags and file size info
	HRESULT getmp3info(void);

	//MP3 の情報を得る その２＆フレームの情報を得る
	HRESULT getframeinfo(void);

//	LONGLONG m_llCurrent;				//ソースファイル上の現在の位置
	FRAMEINFO * currentframe;			//現在再生中のフレーム

	REFERENCE_TIME m_rtStart;			//シーク後の開始位置

	LONGLONG rtMediaStart;				//処理しているサンプルの最初の時間
	LONGLONG rtMediaStop;				//処理しているサンプルの終わりの時間

	mp3info info;
	id3tag id3;

	bool checked_input;					//getmp3infoでチェックされたか。
										//これが通っていなければ、getframeinfoは呼ばれない。
	bool frameinfo_inited;				//frameinfoが初期化されているか否か。
										//DirectShowがなぜか接続後も複数回呼び出してくるので、
										//その対策。
	
	bool seek_occured;					//シークが起こった:true 起こってない:false

//	REFERENCE_TIME m_rtStart;			//シーク後の開始位置
//	REFERENCE_TIME m_rtDuration;		//全体の長さ

public:

//	/*プロパティページで参照されるもの*/
//	STDMETHODIMP get_id3tag(id3tag *_id3)
//	{
//		_id3 = &id3;
//	    return NOERROR;
//	};

};


#endif
