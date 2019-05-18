#include <streams.h>

/*MP3情報構造体(mpg123よりパクリ)*/
#define         MPG_MD_STEREO           0
#define         MPG_MD_JOINT_STEREO     1
#define         MPG_MD_DUAL_CHANNEL     2
#define         MPG_MD_MONO             3
#define			MPG_HEADER_SIZE			4


static __forceinline unsigned long BSwap(unsigned long n)
{
	static const size_t argsz = sizeof(n);
	ASSERT(argsz == 4 && "BSwap only works on 32-bit values");
	__asm mov eax, n __asm bswap eax __asm mov n, eax	
	
	return n;
}


/*/
#define BSwap(N) __asm      \
      \
{                         \
		__asm mov eax, N \
		__asm bswap eax \
		__asm mov Num, eax	\
}


#ifndef BSwap
#define BSwap(n) ((unsigned long) (	(_asm {\
		mov eax, Num\
		bswap eax\
		mov Num, eax\
	}))
#endif
/*/

int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }


  // static constexpr int V2L2L3BitRates[MAX_MPEG_BITRATES] =
  //{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1};


};
long freqs[9] = { 44100, 48000, 32000,
                  22050, 24000, 16000 ,
                  11025 , 12000 , 8000 };
typedef struct mp3info {
	int freq,nch;
	int lsf,frames, framesize, bitrate, vbr;
	LONGLONG payload_size;
	LONGLONG payload_end_postiion;
	LONGLONG nbytes,hpos;
	REFERENCE_TIME duration;
} mp3info;

/*id3TAG用構造体*/
typedef struct id3tag {
	char tag[3];
	char title[30];
	char artist[30];
	char album[30];
	char year[4];
	char comment[30];
	unsigned char genre;
} id3tag;
int genre_last=147;
char *genre_list[]={
	"Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
	"Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
	"Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
	"Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
	"Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
	"Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
	"Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
	"Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
	"Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
	"Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
	"Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
	"New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
	"Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
	"Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
	"Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
	"Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
	"Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
	"Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
	"Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
	"Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
	"Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
	"Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
	"Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
	"Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
	"Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
	"SynthPop",
};


/*フレームの時間とファイル上のオフセットを記録する構造体。*/
typedef struct tagFRAMEINFO
{
	REFERENCE_TIME time;
	LONGLONG offset;
	LONG size;
	struct tagFRAMEINFO * before;
	struct tagFRAMEINFO * next;
} FRAMEINFO;

extern FRAMEINFO frameinfo = {0,0,NULL};

LONGLONG address2time(LONGLONG address);
LONGLONG time2address(LONGLONG time);
int head_check(unsigned long head);
int ExtractI4(unsigned char *buf);
// unsigned int BSwap(unsigned int Num);

int ExtractI4(unsigned char *buf)
{
	int x;
	x = buf[0];
	x <<= 8;
	x |= buf[1];
	x <<= 8;
	x |= buf[2];
	x <<= 8;
	x |= buf[3];
	return x;
}


// whats this he had commented out: 
//	if (!((head>>17)&0x3)) 
//		return FALSE;

int head_check(unsigned long head)
{
	
	
		/* first 11 bits are set to 1 for frame sync */
		if ((head & 0xffe00000) != 0xffe00000)
		return FALSE;
		/* layer: 01,10,11 is 1,2,3; 00 is reserved */
		if (!((head>>17)&3))
		return FALSE;
		/* 1111 means bad bitrate */
		if (((head>>12)&0xf) == 0xf)
		return FALSE;
		/* 0000 means free format... which should be supported in future. */
		if(((head>>12)&0xf) == 0x0)
		return FALSE;
		/* sampling freq: 11 is reserved */
		if(((head>>10)&0x3) == 0x3 )
			return FALSE;
		/* here used to be a mpeg 2.5 check... re-enabled 2.5 decoding due to lack of evidence that it is really not good */
return TRUE;
}


REFERENCE_TIME address2time(LONGLONG address)
{
	FRAMEINFO * _frameinfo;

	_frameinfo = frameinfo.next;
	
	for(;;){
		if(_frameinfo->offset >= address) break;
		if(_frameinfo->next == NULL) break;
		_frameinfo = _frameinfo->next;
	}

	return _frameinfo->time;
}

LONGLONG time2address(REFERENCE_TIME time)
{
	FRAMEINFO * _frameinfo;

	_frameinfo = frameinfo.next;

	for(;;){
		if(_frameinfo->time >= time) break;
		if(_frameinfo->next == NULL) break;
		_frameinfo = _frameinfo->next;
	}

	return _frameinfo->offset;
}

FRAMEINFO * time2frame(REFERENCE_TIME time)
{
	FRAMEINFO * _frameinfo;

	_frameinfo = frameinfo.next;

	for(;;){
		if(_frameinfo->time >= time) break;
		if(_frameinfo->next == NULL) break;
		_frameinfo = _frameinfo->next;
	}

	return _frameinfo;
}
