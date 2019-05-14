#ifndef GUID_MP3P_H
#define GUID_MP3P_H

///////////////////////////////////////////////////////////////////////////////
// Interface GUIDs
///////////////////////////////////////////////////////////////////////////////

/*メインのGUID*/
// {13CEBFE0-256B-44ba-A27B-9F85CBDB972D}
//DEFINE_GUID(CLSID_CMp3Parser, 
//0x13cebfe0, 0x256b, 0x44ba, 0xa2, 0x7b, 0x9f, 0x85, 0xcb, 0xdb, 0x97, 0x2d);
// {6E303006-90B2-422F-91B3-1061A2129DF4}

DEFINE_GUID(CLSID_CMp3Parser, 
0x6e303006, 0x90b2, 0x422f, 0x91, 0xb3, 0x10, 0x61, 0xa2, 0x12, 0x9d, 0xf4);


/*インターフェースのGUID*/
// {0FF200D8-4679-4ca6-ADCD-914433C09717}
//DEFINE_GUID(IID_IMp3Parser, 
//0xff200d8, 0x4679, 0x4ca6, 0xad, 0xcd, 0x91, 0x44, 0x33, 0xc0, 0x97, 0x17);
// {E23FB5C7-EB54-4B61-9278-166B11FAF056}
DEFINE_GUID(IID_IMp3Parser, 
0xe23fb5c7, 0xeb54, 0x4b61, 0x92, 0x78, 0x16, 0x6b, 0x11, 0xfa, 0xf0, 0x56);



///////////////////////////////////////////////////////////////////////////////
// DATA GUIDs
///////////////////////////////////////////////////////////////////////////////

/*入力のサブタイプCLSID(MEDIASUBTYPE_MPEG1 Audio)*/
// {E436EB87-524F-11CE-9F53-0020AF0BA770}
DEFINE_GUID(CLSID_mpeg1audio,
0xE436EB87, 0x524F, 0x11CE, 0x9F, 0x53, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70);

/*出力のサブタイプCLSID(MEDIASUBTYPE_MPEG1 Packet)*/
// {E436EB80-524F-11CE-9F53-0020AF0BA770}
DEFINE_GUID(CLSID_mpeg1packet,
0xE436EB80, 0x524F, 0x11CE, 0x9F, 0x53, 0x00, 0x20, 0xAF, 0x0B, 0xA7, 0x70);

/*出力のサブタイプCLSID(MEDIASUBTYPE_MPEG1 AudioPayload)*/
// {00000050-0000-0010-8000-00AA00389B71}
DEFINE_GUID(CLSID_mpeg1audiopayload,
0x00000050, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

/*出力のサブタイプCLSID(MP3)*/
// {00000055-0000-0010-8000-00aa00389b71}
DEFINE_GUID(CLSID_mp3,
0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

/*
//プロパティページから呼び出すものはここで宣言するらしい。
DECLARE_INTERFACE_(Imp3parse, IUnknown)
{

    STDMETHOD(get_id3tag) (THIS_
    				  id3tag *_id3
				 ) PURE;

};
*/

#endif
