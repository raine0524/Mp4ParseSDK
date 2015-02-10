
#include "mp4parser.h"
#include "mp4utils.h"
#include "mp4v2.h"

#include <string.h>
#include <stdlib.h>

#ifdef _LINUX_
  #include <arpa/inet.h>
#endif

#define MP4PARSER_MODULE_VERSION  (const char*)"MP4Parser 20141216"
#define MP4_MAX_SAMPLE_LEN  512<<10
#define MP4_MAX_SPS_LEN  256
#define MP4_MAX_PPS_LEN  256
#define H264_NALHDR_LEN    4

#ifndef UINT64_C
#define UINT64_C(x)  x ## ULL
#endif

unsigned short HpMp4ParserInit()
{
    return MP4PARSER_NO_ERROR;
}

unsigned short HpMp4ParserQuit()
{
    return MP4PARSER_NO_ERROR;
}

char g_achMp4ParserVersion[256] = {0};
void HpMp4ParserGetVersion(char** ppVerionString)
{
    sprintf(g_achMp4ParserVersion, "MP4Parser Version:%s, time:%s, date:%s\n",
        MP4PARSER_MODULE_VERSION, __TIME__, __DATE__);
    if( ppVerionString != 0 )
    {
        *ppVerionString = g_achMp4ParserVersion;
    }
}


CMp4File::CMp4File(CMp4FileCallBack *pcCallBack)
: m_bRunning(false)
, m_pcCallBack(pcCallBack)
, m_pInFile(NULL)
, m_pOutVideoFile(NULL)
, m_pOutAudioFile(NULL)
, m_pVideoFileHandle(NULL)
, m_pAudioFileHandle(NULL)
, m_dwVideoTrackID(0)
, m_dwAudioTrackID(0)
, m_pMp4FileHandle(NULL)
, m_wSPSLen(0)
, m_pchSPS(NULL)
, m_wPPSLen(0)
, m_pchPPS(NULL)
, m_pchVideoWriteSample(NULL)
, m_pchAudioWriteSample(NULL)
, m_ucAudioChanNum(0)
, m_ucAudioType(0)
, m_ucAudioFrequencies(0)
{
    m_pchSPS = new unsigned char[MP4_MAX_SPS_LEN];
    m_pchPPS = new unsigned char[MP4_MAX_PPS_LEN];
	memset(&m_stVideoInfo, 0, sizeof(Mp4VideoInfo));

    m_pchVideoWriteSample = new unsigned char[MP4_MAX_SAMPLE_LEN];
    m_pchAudioWriteSample = new unsigned char[MP4_MAX_SAMPLE_LEN];
}

CMp4File::~CMp4File()
{
	if (m_pchSPS)
    {
        delete [] m_pchSPS;
        m_pchSPS = NULL;
    }

    if (m_pchPPS)
    {
        delete [] m_pchPPS;
        m_pchPPS = NULL;
    }

    if (m_pchVideoWriteSample)
    {
        delete [] m_pchVideoWriteSample;
        m_pchVideoWriteSample = NULL;
    }

    if (m_pchAudioWriteSample)
    {
        delete [] m_pchAudioWriteSample;
        m_pchAudioWriteSample = NULL;
    }

    if (NULL != m_pMp4FileHandle)
    {
        MP4Close( (void*)m_pMp4FileHandle );
        m_pMp4FileHandle = NULL;
	}

	delete m_pInFile;
	delete m_pOutVideoFile;
	delete m_pOutAudioFile;
	m_pInFile = NULL;
	m_pOutVideoFile = NULL;
	m_pOutAudioFile = NULL;
}

unsigned short CMp4File::SetFile(const char *pInFile, const char *pOutVideoFile, const char *pOutAudioFile)
{
	if (NULL == pInFile)
	{
		return MP4PARSER_ERROR_PARAMETER;
	}

	delete m_pInFile;
	delete m_pOutVideoFile;
	delete m_pOutAudioFile;
	m_pInFile = NULL;
	m_pOutVideoFile = NULL;
	m_pOutAudioFile = NULL;

	if (pInFile != NULL)
	{
		m_pInFile = strdup(pInFile);
	}
	if (pOutVideoFile != NULL)
	{
		m_pOutVideoFile = strdup(pOutVideoFile);
	}
	if (pOutAudioFile != NULL)
	{
		m_pOutAudioFile = strdup(pOutAudioFile);
	}

	char* pMP4FileInfo = MP4FileInfo(m_pInFile);
	if (pMP4FileInfo)
	{
		printf("Get mp4 file info: \n%s\n", pMP4FileInfo);
		char* pch = strstr(pMP4FileInfo, "video");
		pch = strstr(pch, ",");
		pch += 2;	//skip ", " which contains 2 characters
		m_stVideoInfo.duration = atoi(pch);
		pch = strstr(pch, ",");
		pch += 2;
		m_stVideoInfo.bitrate = atoi(pch);
		pch = strstr(pch, ",");
		pch += 2;
		m_stVideoInfo.width = atoi(pch);
		pch = strstr(pch, "x");
		pch ++;
		m_stVideoInfo.height = atoi(pch);
		pch = strstr(pch, "@");
		pch += 2;
		m_stVideoInfo.frame_rate = atoi(pch);
		free(pMP4FileInfo);
	}
	return MP4PARSER_NO_ERROR;
}

//unsigned short CMp4File::SetParseParam()
//{
//    return 0;
//}

unsigned short CMp4File::StartParse()
{
	printf("StartParse this:0x%p \n", this);

    if(true == m_bRunning)
    {
        return MP4PARSER_NO_ERROR;
    }

    // read file 
    m_pMp4FileHandle = MP4Read(m_pInFile);
    if ( !m_pMp4FileHandle ) 
    {
        return MP4PARSER_ERROR_READFILE;
    }

    // validate type
    unsigned int dwNumTracks = MP4GetNumberOfTracks( m_pMp4FileHandle );
    for ( unsigned int i = 0; i < dwNumTracks; i++ ) 
    {
        const char *media_data_name = NULL;
        unsigned int dwTrackId = MP4FindTrackId( m_pMp4FileHandle, i );

        const char *pchTrackType  = MP4GetTrackType(m_pMp4FileHandle, dwTrackId);
        if (pchTrackType == NULL) 
        {
            continue;
        }

        if (!strcmp(pchTrackType, MP4_VIDEO_TRACK_TYPE)) 
        {
            
            media_data_name = MP4GetTrackMediaDataName(m_pMp4FileHandle, dwTrackId);
            if (media_data_name == NULL) 
            {
                continue;
            }

            if (0 == strcmp(media_data_name, "avc1"))
            {
                m_dwVideoTrackID = dwTrackId;
            }
        }

        if (!strcmp(pchTrackType, MP4_AUDIO_TRACK_TYPE)) 
        {
            media_data_name = MP4GetTrackMediaDataName(m_pMp4FileHandle, dwTrackId);
            if (media_data_name == NULL) 
            {
                continue;
            }

            if (0 == strcmp(media_data_name, "mp4a"))
            {
                m_dwAudioTrackID = dwTrackId;
            }
        }
    }

    if ( (0 == m_dwVideoTrackID) || (0 == m_dwAudioTrackID) )
    {
        MP4Close( (void*)m_pMp4FileHandle );
        m_pMp4FileHandle = NULL;
        return MP4PARSER_ERROR_INVALIDTRACK;
    }


    // start thread
    bool ret = StartThread();
    if (false == ret)
    {
        return MP4PARSER_ERROR_CREATETHREAD;
    }
    else
    {
        m_bRunning = true;
    }

    return MP4PARSER_NO_ERROR;
}

unsigned short CMp4File::StopParse()
{
	if(true == m_bRunning)
    {
        m_bRunning = false;
        WaitForStop();
    }

    if (NULL != m_pMp4FileHandle)
    {
        MP4Close( (void*)m_pMp4FileHandle );
        m_pMp4FileHandle = NULL;
    }
   
    return MP4PARSER_NO_ERROR;
}


bool CMp4File::GetH264Param(unsigned char *pchSPS, unsigned short wMaxSPSLen, unsigned short *pwRealSPSLen, 
                  unsigned char *pchPPS, unsigned short wMaxPPSLen, unsigned short *pwRealPPSLen)
{
	if ( (NULL==m_pMp4FileHandle) || (0==m_dwVideoTrackID))
    {
        return false;
    }

    unsigned char nalheader[H264_NALHDR_LEN] = {0x00, 0x00, 0x00, 0x01};
    unsigned char  **seqheader, **pictheader;
    unsigned int *pictheadersize, *seqheadersize;
    unsigned int ix;
    bool bGet264Param = MP4GetTrackH264SeqPictHeaders(m_pMp4FileHandle, m_dwVideoTrackID,
        &seqheader, &seqheadersize, &pictheader, &pictheadersize);
    if (false == bGet264Param)
    {
        return false;
    }

    for (ix = 0; seqheadersize[ix] != 0; ix++) 
    {
        if (pchSPS)
        {
            unsigned short size = (seqheadersize[ix]<(wMaxSPSLen-H264_NALHDR_LEN)) ? (seqheadersize[ix]) : (wMaxSPSLen-H264_NALHDR_LEN);
            memcpy(pchSPS, nalheader, H264_NALHDR_LEN);
            memcpy(pchSPS+H264_NALHDR_LEN, seqheader[ix], size);
            (*pwRealSPSLen) = H264_NALHDR_LEN+size;
        }

        free(seqheader[ix]);
    }
    free(seqheader);
    free(seqheadersize);

    for (ix = 0; pictheadersize[ix] != 0; ix++) 
    {
        if (pchPPS)
        {
            unsigned short size = (pictheadersize[ix]<wMaxPPSLen-H264_NALHDR_LEN) ? (pictheadersize[ix]) : wMaxPPSLen-H264_NALHDR_LEN;
            memcpy(pchPPS, nalheader, H264_NALHDR_LEN);
            memcpy(pchPPS+H264_NALHDR_LEN, pictheader[ix], size);
            (*pwRealPPSLen) = H264_NALHDR_LEN+size;
        }

        free(pictheader[ix]);
    }
    free(pictheader);
    free(pictheadersize);

    return true;
}

bool CMp4File::ProcessVideoSamples(unsigned long dwStartMs, unsigned int dwVideoId, unsigned int *pdwNextVideoId)
{
	if ( (NULL==m_pMp4FileHandle) || (0==m_dwVideoTrackID))
    {
        return false;
    }

    if ( (NULL==m_pchSPS) || (0==m_wSPSLen) || (NULL==m_pchPPS) || (0==m_wPPSLen) )
    {
        return false;
    }

    if (NULL == m_pchVideoWriteSample)
    {
        return false;
    }
    else
    {
        memset(m_pchVideoWriteSample, 0, MP4_MAX_SAMPLE_LEN);
    }

    // static方式可以减少读取次数,但会降低可重入性,暂不使用
    unsigned char *pSample = NULL;
    unsigned int sampleSize = 0;    
    unsigned long long  msectime = 0;
    MP4Timestamp sampleTime;
    MP4Duration sampleDuration;
    unsigned int timescale = MP4GetTrackTimeScale( m_pMp4FileHandle, m_dwVideoTrackID );

    unsigned char *pchTempPtr = m_pchVideoWriteSample;
    unsigned int  dwWriteSampleSize = 0;
    

    //读取Sample数据
    unsigned int sampleId = dwVideoId+1;
    bool bReadRet = MP4ReadSample( m_pMp4FileHandle, m_dwVideoTrackID, sampleId, &pSample, &sampleSize, &sampleTime, &sampleDuration );
    if( (false==bReadRet) || (sampleSize<5) || (sampleSize>MP4_MAX_SAMPLE_LEN) || (NULL==pSample))
    {
        return false;
    }

    //获取当前Sample起始时间
    msectime = sampleTime;
    msectime *= UINT64_C( 1000 );
    msectime /= timescale;

    unsigned long dwCurTick = mpTickGet();
    unsigned long dwCurMs   = mpTickToMs(dwCurTick);
    if ( (dwCurMs - dwStartMs) >= msectime)
    {
        //printf("[VID id=%d]callback startms:%u, curms:%u, diff:%u, msectime:%u \n", sampleId, dwStartMs, dwCurMs, (dwCurMs-dwStartMs), msectime );

        //遍历整个Sample, 将长度字段改为0001, 记录第一个IDR(如果有的话)
        bool bGotFirstIDR = false;
        unsigned char *pFirstIDR = NULL;
        for (unsigned short i=0; i<sampleSize; )
        {
            unsigned int *pSize = (unsigned int*)(pSample+i);
            unsigned int wSize = ntohl(*pSize);
            pSample[i] = 0x00;
            pSample[i+1] = 0x00;
            pSample[i+2] = 0x00;
            pSample[i+3] = 0x01;

            if ( (5 == (pSample[i+4]&0x1f)) && (false == bGotFirstIDR) )
            {
                pFirstIDR = pSample+i;
                bGotFirstIDR = true;
            }

            i += wSize+4;
        }

        // 如果有IDR, 插入SPS和PPS
        if(NULL != pFirstIDR) 
        {
            unsigned int dwSpanSize = pFirstIDR - pSample;
            if (0 != dwSpanSize)
            {
                memcpy(pchTempPtr, pSample, dwSpanSize);
                pchTempPtr += dwSpanSize;
            }

            memcpy(pchTempPtr, m_pchSPS, m_wSPSLen);
            pchTempPtr += m_wSPSLen;

            memcpy(pchTempPtr, m_pchPPS, m_wPPSLen);
            pchTempPtr += m_wPPSLen;

            memcpy(pchTempPtr, pFirstIDR, (sampleSize-dwSpanSize));
            pchTempPtr += (sampleSize-dwSpanSize);

            dwWriteSampleSize = pchTempPtr-m_pchVideoWriteSample;

            if (m_pcCallBack)
            {
                m_pcCallBack->OnVideoCallBack(this, m_pchVideoWriteSample+dwSpanSize, (dwWriteSampleSize-dwSpanSize),true);
                m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSING);
            }
        }
        else
        {
            memcpy(m_pchVideoWriteSample, pSample, sampleSize);
            dwWriteSampleSize = sampleSize;

            if (m_pcCallBack)
            {
                m_pcCallBack->OnVideoCallBack(this, pSample, sampleSize,false);
                m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSING);
            }
        }

        // 写入操作, TODO: 写入失败处理和写入不足数据
        if(NULL != m_pVideoFileHandle)
        {
            fwrite(m_pchVideoWriteSample, 1, dwWriteSampleSize, m_pVideoFileHandle);
        }

        //下一个Sample
        (*pdwNextVideoId) = ++dwVideoId;
    }
    else
    {
        //printf("[VID id=%d] NOT callback startms:%u, curms:%u, diff:%u, msectime:%u \n", sampleId, dwStartMs, dwCurMs, (dwCurMs-dwStartMs), msectime );
        (*pdwNextVideoId) = dwVideoId;
    }

    free( pSample );
    pSample = NULL;

    return true;

}

bool CMp4File::ConstructAudioHeader(unsigned char *pHdr, 
                unsigned char ucProfile, unsigned char ucFreqIdx, unsigned char ucChanCfg, unsigned short wPacketLen) 
{
	if ( (NULL==pHdr) || (0==wPacketLen) )
    {
        return false;
    }

    pHdr[0] = (unsigned char)0xFF;
    pHdr[1] = (unsigned char)0xF1;
    pHdr[2] = (unsigned char)(((ucProfile-1)<<6) + (ucFreqIdx<<2) +(ucChanCfg>>2));
    pHdr[3] = (unsigned char)(((ucChanCfg&3)<<6) + (wPacketLen>>11));
    pHdr[4] = (unsigned char)((wPacketLen&0x7FF) >> 3);
    pHdr[5] = (unsigned char)(((wPacketLen&7)<<5) + 0x1F);
    pHdr[6] = (unsigned char)0xFC;

    return true;
}



bool CMp4File::ProcessAudioSamples(unsigned long dwStartMs, unsigned int dwAudioId, unsigned int *pdwNextAudioId)
{
    if ( (NULL==m_pMp4FileHandle) || (0==m_dwAudioTrackID))
    {
        return false;
    }

    if (NULL == m_pchAudioWriteSample)
    {
        return false;
    }
    else
    {
        memset(m_pchAudioWriteSample, 0, MP4_MAX_SAMPLE_LEN);
    }

    // static方式可以减少读取次数,但会降低可重入性,暂不使用
    unsigned char *pSample = NULL;
    unsigned int sampleSize = 0;    
    unsigned long long  msectime = 0;
    MP4Timestamp sampleTime;
    MP4Duration sampleDuration;
    unsigned int timescale = MP4GetTrackTimeScale( m_pMp4FileHandle, m_dwAudioTrackID );

    unsigned char *pchTempPtr = m_pchAudioWriteSample;
    unsigned int  dwWriteSampleSize = 0;

    //读取Sample数据
    unsigned int sampleId = dwAudioId+1;
    bool bReadRet = MP4ReadSample( m_pMp4FileHandle, m_dwAudioTrackID, sampleId, &pSample, &sampleSize, &sampleTime, &sampleDuration );
    if( (false==bReadRet) || (sampleSize<5) || (sampleSize>MP4_MAX_SAMPLE_LEN) || (NULL==pSample))
    {
        return false;
    }

    //获取当前Sample起始时间
    msectime = sampleTime;
    msectime *= UINT64_C( 1000 );
    msectime /= timescale;

    unsigned long dwCurTick = mpTickGet();
    unsigned long dwCurMs   = mpTickToMs(dwCurTick);
    if ( (dwCurMs - dwStartMs) >= msectime)
    {
        //printf("[AUD id=%d]callback startms:%u, curms:%u, diff:%u, msectime:%u \n", sampleId, dwStartMs, dwCurMs, (dwCurMs-dwStartMs), msectime );
        
        memset(m_achAudioHeader, 0, MP4_AUDIO_ADTSHDR_LEN);
        bool bCreateHdr = ConstructAudioHeader(m_achAudioHeader, 
            m_ucAudioType, m_ucAudioFrequencies, m_ucAudioChanNum, sampleSize+MP4_AUDIO_ADTSHDR_LEN);
        if (false == bCreateHdr)
        {
            printf("[ERROR] Failed to Construct Audio ADTS Header \n");
            return false;
        }
        
        memcpy(m_pchAudioWriteSample, m_achAudioHeader, MP4_AUDIO_ADTSHDR_LEN);
        dwWriteSampleSize += MP4_AUDIO_ADTSHDR_LEN;

        memcpy(m_pchAudioWriteSample+MP4_AUDIO_ADTSHDR_LEN, pSample, sampleSize);
        dwWriteSampleSize += sampleSize;

        if (m_pcCallBack)
        {
            m_pcCallBack->OnAudioCallBack(this, m_pchAudioWriteSample, dwWriteSampleSize);
            m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSING);
        }

        // 写入操作, TODO: 写入失败处理和写入不足数据
        if(NULL != m_pAudioFileHandle)
        {
            fwrite(m_pchAudioWriteSample, 1, dwWriteSampleSize, m_pAudioFileHandle);
        }

        //下一个Sample
        (*pdwNextAudioId) = ++dwAudioId;
    }
    else
    {
        //printf("[AUD id=%d] NOT callback startms:%u, curms:%u, diff:%u, msectime:%u \n", sampleId, dwStartMs, dwCurMs, (dwCurMs-dwStartMs), msectime );
        (*pdwNextAudioId) = dwAudioId;
    }

    free( pSample );
    pSample = NULL;

    return true;


}
void CMp4File::ThreadProcMain(void)
{
    if ( !m_pMp4FileHandle ) 
    {
        if (m_pcCallBack)
        {
            m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_READFILE); //fail
        }
        
        m_bRunning = false;
        return;
    }

    if ( (0 == m_dwVideoTrackID) || (0 == m_dwAudioTrackID) )
    {
        if (m_pcCallBack)
        {
            m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDTRACK); //fail
        }

        m_bRunning = false;
        return;
    }

    if (0 != m_dwVideoTrackID)
    {
        if ( (NULL == m_pchSPS) || (NULL == m_pchPPS) )
        {
            if (m_pcCallBack)
            {
                m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_BUFALLOCATE); //fail
            }

            m_bRunning = false;
            return;
        }

        // dump SPS/PPS from avcC
        bool bRetGetParam = GetH264Param(m_pchSPS, MP4_MAX_SPS_LEN, &m_wSPSLen, 
            m_pchPPS, MP4_MAX_PPS_LEN, &m_wPPSLen);
        if (false == bRetGetParam)
        {
            if (m_pcCallBack)
            {
                m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDAVCC); //fail
            }

            m_bRunning = false;
            return;
        }
    }

    if (0 != m_dwAudioTrackID)
    {
        //m_ucAudioChanNum = MP4GetTrackAudioChannels(m_pMp4FileHandle, m_dwAudioTrackID);
        //m_ucAudioType = MP4GetTrackAudioMpeg4Type(m_pMp4FileHandle, m_dwAudioTrackID);

        unsigned char *pchAudioESConfig = NULL;
        unsigned int  dwAudioESConfigSize = 0;
        bool bGetES = MP4GetTrackESConfiguration(m_pMp4FileHandle, m_dwAudioTrackID, &pchAudioESConfig, &dwAudioESConfigSize);
        if ( (false==bGetES) || (dwAudioESConfigSize<1) || (NULL==pchAudioESConfig) )
        {
            if (m_pcCallBack)
            {
                m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDESDS); //fail
            }
            
            if(dwAudioESConfigSize<1) 
            {
                free(pchAudioESConfig);
            }
             
            m_bRunning = false;
            return;
        }
        else
        {
            m_ucAudioType = ((pchAudioESConfig[0] >> 3) & 0x1f);
            m_ucAudioFrequencies = ((pchAudioESConfig[0] & 0x07) << 1) | ((pchAudioESConfig[1] & 0x80) >> 7);
            m_ucAudioChanNum = ((pchAudioESConfig[1] >> 3) & 0x0f);

            //printf("[AUD] m_ucAudioType:%u m_ucAudioFrequencies:%u m_ucAudioChanNum:%u \n", m_ucAudioType, m_ucAudioFrequencies, m_ucAudioChanNum );
            free(pchAudioESConfig);
        }
    }


    // open files
    if (NULL != m_pOutVideoFile)
    {
        m_pVideoFileHandle = fopen(m_pOutVideoFile, "wb");
    }
    if (NULL != m_pOutAudioFile)
    {
        m_pAudioFileHandle = fopen(m_pOutAudioFile, "wb");
    }


    MP4SampleId dwNumVideoSamples = MP4GetTrackNumberOfSamples( m_pMp4FileHandle, m_dwVideoTrackID );
    MP4SampleId dwNumAudioSamples = MP4GetTrackNumberOfSamples( m_pMp4FileHandle, m_dwAudioTrackID );
    if ( (0==dwNumVideoSamples) && (0==dwNumAudioSamples) )
    {
        if (m_pcCallBack)
        {
            m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDSAMPLE); //fail
        }

        m_bRunning = false;
        return;
    }

    printf("this:0x%p VideoTrackID:%u, dwNumVideoSamples:%u starttick:%u \n", this, m_dwVideoTrackID, dwNumVideoSamples, mpTickGet() );
    printf("this:0x%p AudioTrackID:%u, dwNumAudioSamples:%u starttick:%u \n", this, m_dwAudioTrackID, dwNumAudioSamples, mpTickGet() );

    unsigned int  dwVideoId = 0;
    unsigned int  dwAudioId = 0;
    unsigned long dwStartTick = mpTickGet();
    unsigned long dwStartMs   = mpTickToMs(dwStartTick);
    while (false != m_bRunning)
    {
        if ( (dwVideoId>=dwNumVideoSamples) && (dwAudioId>=dwNumAudioSamples)  )
        {
            break;
        }

        bool bVideoProceed = false;
        if (dwVideoId < dwNumVideoSamples)
        {
            unsigned int dwNextVideoId;
            bool bRet = ProcessVideoSamples(dwStartMs, dwVideoId, &dwNextVideoId);
            if (false == bRet)
            {
                if (m_pcCallBack)
                {
                    m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDSAMPLE); //fail
                }

                m_bRunning = false;
                return;
            }

            if (dwVideoId != dwNextVideoId)
            {
                dwVideoId = dwNextVideoId;
                bVideoProceed = true;
            }
        }


        bool bAudioProceed = false;
        if (dwAudioId < dwNumAudioSamples)
        {
            unsigned int dwNextAudioId;
            bool bRet = ProcessAudioSamples(dwStartMs, dwAudioId, &dwNextAudioId);
            if (false == bRet)
            {
                if (m_pcCallBack)
                {
                    m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSEFAIL, MP4PARSER_ERROR_INVALIDSAMPLE); //fail
                }

                m_bRunning = false;
                return;
            }

            if (dwAudioId != dwNextAudioId)
            {
                dwAudioId = dwNextAudioId;
                bAudioProceed = true;
            }
        }
       
        if ( (false==bVideoProceed) && (false==bAudioProceed) )
        {
            mpSleep(5);
            continue;
        }
    }


    // close files
    if(NULL != m_pVideoFileHandle)
    {
        fclose(m_pVideoFileHandle);
        m_pVideoFileHandle = NULL;
    }
    if(NULL != m_pAudioFileHandle)
    {
        fclose(m_pAudioFileHandle);
        m_pAudioFileHandle = NULL;
    }

    printf("this:0x%p VideoTrackID:%u, dwNumVideoSamples:%u dwVideoId:%u stoptick:%u \n", this, m_dwVideoTrackID, dwNumVideoSamples, dwVideoId, mpTickGet() );
    printf("this:0x%p AudioTrackID:%u, dwNumAudioSamples:%u dwAudioId:%u stoptick:%u \n", this, m_dwAudioTrackID, dwNumAudioSamples, dwAudioId, mpTickGet() );

    if (m_pcCallBack)
    {
        m_pcCallBack->OnParseStatus(this, MP4PARSER_SATUS_PARSED); //success
    }
	m_bRunning = false;

}
