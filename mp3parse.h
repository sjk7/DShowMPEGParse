#ifndef MP3PARSE_H
#define MP3PARSE_H

#include <streams.h>
#include <mmreg.h>
#include "parser.h"
#include "mp3.h"
#include <VECTOR>


// typedef std::vector<FRAMEINFO> frame_vec_t;

// vbr seek helper
struct header_hint
{
	header_hint(LONGLONG pos, LONGLONG dur, size_t index) 
		: hpos(pos), dur(dur), index(index){}
	
	LONGLONG hpos;
	LONGLONG dur;
	size_t index;

};

typedef std::vector<header_hint> header_hints;


class CMP3ParseFilter : public CParserFilter
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *pHr);
	
	header_hints m_vbr_header_hints;
	std::vector<BYTE> m_filebuf;

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
//	LONG m_lCurrentFrame;				//���݂̃t���[���i���o�[
//	REFERENCE_TIME m_rtStart;			//�V�[�N��̊J�n�ʒu
//	REFERENCE_TIME m_rtDuration;		//�S�̂̒���
//	int m_tagsize;
//	AACInfo m_info;
//	DWORD * m_seek_table;
//	int m_seek_table_len;
//	LONGLONG m_llSize;					//�t�@�C���T�C�Y

	//�\�[�X�t�B���^�𓯊��ǂݍ��݂��邽�߂̊֐�
	IAsyncReader * ReaderOpen(void);
	HRESULT ReaderLength(LONGLONG *pAvailable,IAsyncReader *reader);
	HRESULT ReaderAvailableLength(LONGLONG *pAvailable,IAsyncReader *reader);
	HRESULT ReaderRead(LONGLONG llPosition,LONG llength,void *pBuffer,IAsyncReader *reader);
	HRESULT ReaderClose(IAsyncReader *reader);

	// finds the first frame, and gets tags and file size info
	HRESULT getmp3info(IAsyncReader* reader, int full_scan_starts_at = -1);

	//MP3 �̏��𓾂� ���̂Q���t���[���̏��𓾂�
	HRESULT getframeinfo(void);

//	LONGLONG m_llCurrent;				//�\�[�X�t�@�C����̌��݂̈ʒu
	FRAMEINFO * currentframe;			//���ݍĐ����̃t���[��

	REFERENCE_TIME m_rtStart;			//�V�[�N��̊J�n�ʒu

	LONGLONG rtMediaStart;				//�������Ă���T���v���̍ŏ��̎���
	LONGLONG rtMediaStop;				//�������Ă���T���v���̏I���̎���

	mp3info m_info;
	id3tag m_id3;

	bool checked_input;					//getmp3info�Ń`�F�b�N���ꂽ���B
										//���ꂪ�ʂ��Ă��Ȃ���΁Agetframeinfo�͌Ă΂�Ȃ��B
	bool frameinfo_inited;				//frameinfo������������Ă��邩�ۂ��B
										//DirectShow���Ȃ����ڑ����������Ăяo���Ă���̂ŁA
										//���̑΍�B
	
	bool seek_occured;					//�V�[�N���N������:true �N�����ĂȂ�:false

//	REFERENCE_TIME m_rtStart;			//�V�[�N��̊J�n�ʒu
//	REFERENCE_TIME m_rtDuration;		//�S�̂̒���

public:

//	/*�v���p�e�B�y�[�W�ŎQ�Ƃ�������*/
//	STDMETHODIMP get_id3tag(id3tag *_id3)
//	{
//		_id3 = &id3;
//	    return NOERROR;
//	};

};


#endif
