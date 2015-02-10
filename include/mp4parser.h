/*****************************************************************************
模块名      : HpTransAdapter
文件名      : hptransadapter.h
相关文件    : hpTransadapter.cpp
文件实现功能: CMediaSend CMediaRecv类定义
-----------------------------------------------------------------------------
修改记录:
日  期      版本        修改人      修改内容
******************************************************************************/

#ifndef _MP4PARSER_H_
#define _MP4PARSER_H_

/*==========================================================================================
---- 头文件 ----
==========================================================================================*/
#include "XThreadBase.h"
#include "stdio.h"

/*==========================================================================================
---- 宏定义  ----
==========================================================================================*/
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#ifndef MP4V2_USE_STATIC_LIB
#define MP4V2_USE_STATIC_LIB
#endif

/*==========================================================================================
 ---- 类型定义  ----
==========================================================================================*/
#define MP4PARSER_NO_ERROR            (unsigned short)0
#define MP4PARSER_ERROR_INIT          (unsigned short)1
#define MP4PARSER_ERROR_PARAMETER     (unsigned short)2
#define MP4PARSER_ERROR_READFILE      (unsigned short)3
#define MP4PARSER_ERROR_INVALIDTRACK  (unsigned short)4
#define MP4PARSER_ERROR_INVALIDAVCC   (unsigned short)5
#define MP4PARSER_ERROR_INVALIDSAMPLE (unsigned short)6
#define MP4PARSER_ERROR_CREATETHREAD  (unsigned short)7
#define MP4PARSER_ERROR_BUFALLOCATE   (unsigned short)8
#define MP4PARSER_ERROR_INVALIDESDS   (unsigned short)9

#define MP4PARSER_SATUS_UNUSED        (unsigned short)0
#define MP4PARSER_SATUS_PARSING       (unsigned short)1
#define MP4PARSER_SATUS_PARSED        (unsigned short)2
#define MP4PARSER_SATUS_PARSEFAIL     (unsigned short)3


#define MP4_AUDIO_ADTSHDR_LEN  7

/*=========================================================================================
---- 全局函数 ----
==========================================================================================*/
/*=============================================================================
	函数名		：HpMp4ParserInit
	功能		：MP4解析器初始化
	算法实现	：（可选项）
	引用全局变量：
	输入参数说明：

	返回值说明：
=============================================================================*/
unsigned short HpMp4ParserInit();

/*=============================================================================
	函数名		：HpMp4ParserInit
	功能		：MP4解析器退出
	算法实现	：（可选项）
	引用全局变量：
	输入参数说明：

	返回值说明：
=============================================================================*/
unsigned short HpMp4ParserQuit();

/*=============================================================================
	函数名		：HpMp4ParserGetVersion
	功能		：获取Mp4Parser模块的版本信息
	算法实现	：（可选项）
	引用全局变量：
	输入参数说明：

	返回值说明：
=============================================================================*/
void HpMp4ParserGetVersion(char** ppVerionString);

typedef struct _tagMp4VideoInfo {
	unsigned short frame_rate;
	unsigned short width;
	unsigned short height;
	unsigned short bitrate;			//unit: kbps
	unsigned int duration;		//sec

	_tagMp4VideoInfo()
		:frame_rate(0)
		,width(0)
		,height(0)
		,bitrate(0)
		,duration(0)
	{
	}
} Mp4VideoInfo;


/*==========================================================================================
---- 类定义 ----
==========================================================================================*/
class CMp4File;
class CMp4FileCallBack
{
public:
    virtual void OnParseStatus(const CMp4File *pcParser, const unsigned short wStatus, const unsigned short wExtStatus=0)=0;
    virtual void OnVideoCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen, const bool bKeyFrame)=0;
    virtual void OnAudioCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen)=0;
};


class CMp4File : public XThreadBase
{
public:
    CMp4File(CMp4FileCallBack *pcCallBack);
    virtual ~CMp4File();

    unsigned short SetFile(const char *pInFile, const char *pOutVideoFile=NULL, const char *pOutAudioFile=NULL);

    //unsigned short SetParseParam();
	Mp4VideoInfo* GetVideoInfo() { return &m_stVideoInfo; }

    unsigned short StartParse();

    unsigned short StopParse();

    virtual void ThreadProcMain(void);

private:
    bool GetH264Param(unsigned char *pchSPS, unsigned short wMaxSPSLen, unsigned short *pwRealSPSLen, 
                      unsigned char *pchPPS, unsigned short wMaxPPSLen, unsigned short *pwRealPPSLen);
    
    bool ConstructAudioHeader(unsigned char *pHdr,unsigned char ucProfile, 
                      unsigned char ucFreqIdx, unsigned char ucChanCfg, unsigned short wPacketLen);

    bool ProcessVideoSamples(unsigned long dwStartMs, unsigned int dwVideoId, unsigned int *pdwNextVideoId);
    bool ProcessAudioSamples(unsigned long dwStartMs, unsigned int dwAudioId, unsigned int *pdwNextAudioId);

private:
    bool m_bRunning;
    CMp4FileCallBack *m_pcCallBack;
    unsigned int m_dwVideoTrackID;  //only support H.264 now
    unsigned int m_dwAudioTrackID;  //only support MP4A/AAC now
    void *m_pMp4FileHandle;

    char *m_pInFile;
    char *m_pOutVideoFile;
    char *m_pOutAudioFile;
    FILE *m_pVideoFileHandle;
    FILE *m_pAudioFileHandle;

    unsigned short m_wSPSLen;
    unsigned char *m_pchSPS;
    unsigned short m_wPPSLen;
    unsigned char *m_pchPPS;
	Mp4VideoInfo m_stVideoInfo;

    unsigned char *m_pchVideoWriteSample;
    unsigned char *m_pchAudioWriteSample;  

    unsigned char m_ucAudioChanNum;
    unsigned char m_ucAudioType;
    unsigned char m_ucAudioFrequencies;
    unsigned char m_achAudioHeader[MP4_AUDIO_ADTSHDR_LEN];
};



#endif //_MP4PARSER_H_

