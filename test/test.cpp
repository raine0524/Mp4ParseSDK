
#include "mp4parser.h"
#include <stdio.h>


#ifdef WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif


#ifdef WIN32
  #define TEST_FILE_1 "d:\\1.mp4"
  #define TEST_FILE_2 "d:\\2.mp4"
  #define TEST_OUT_VIDEO "d:\\out"
  #define TEST_OUT_AUDIO "d:\\out"
#else
  #define TEST_FILE_1 "/tmp/1.mp4"
  #define TEST_FILE_2 "/tmp/2.mp4"
  #define TEST_OUT_VIDEO "/tmp/out.h264"
  #define TEST_OUT_AUDIO "/tmp/out.aac"
#endif


bool g_bParseFinished_1 = false;
bool g_bParseFinished_2 = false;


class CDemo : public CMp4FileCallBack
{
public:
    CDemo(int value);
    virtual ~CDemo();

    unsigned short SetFile(const char *pInFile, const char *pOutVideoFile=NULL, const char *pOutAudioFile=NULL);
	Mp4VideoInfo* GetVideoInfo() {
		if (pFile)
			return pFile->GetVideoInfo();
		else
			return NULL;
	}
    unsigned short StartParse();
    unsigned short StopParse();

public:
    virtual void OnParseStatus(const CMp4File *pcParser, const unsigned short wStatus, const unsigned short wExtStatus=0);
    virtual void OnVideoCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen, const bool bKeyFrame);
    virtual void OnAudioCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen);
private:
    CMp4File *pFile;
    FILE *pOutVideoFile;
    FILE *pOutAudioFile;
    int flag;
};

CDemo::CDemo(int value)
{
    pFile = NULL;
    pFile = new CMp4File(this);

    pOutVideoFile = NULL;

    char achBuf[20];
    sprintf(achBuf, "%s%u.h264", TEST_OUT_VIDEO, value);
    pOutVideoFile = fopen(achBuf, "wb");

    sprintf(achBuf, "%s%u.aac", TEST_OUT_AUDIO, value);
    pOutAudioFile = fopen(achBuf, "wb");

    flag = value;
}

CDemo::~CDemo()
{
    if (pFile)
    {
        delete pFile;
        pFile = NULL;
    }
}

unsigned short CDemo::SetFile(const char *pInFile, const char *pOutVideoFile, const char *pOutAudioFile)
{
    if (NULL == pFile)
    {
        return 1;
    }
    return pFile->SetFile(pInFile, pOutVideoFile, pOutAudioFile);
}
unsigned short CDemo::StartParse()
{
    if (NULL == pFile)
    {
        return 1;
    }
    return pFile->StartParse();
}
unsigned short CDemo::StopParse()
{
    if (NULL == pFile)
    {
        return 1;
    }
    return pFile->StopParse();
}

void CDemo::OnParseStatus(const CMp4File *pcParser, const unsigned short wStatus, const unsigned short wExtStatus)
{
    if (pcParser == pFile)
    {
		//printf("OnParseStatus wStatus:%d this:%p \n", wStatus, this);
        if( (wStatus == MP4PARSER_SATUS_PARSED) || (wStatus == MP4PARSER_SATUS_PARSEFAIL) )
        {
			printf("OnParseStatus wStatus:%d flag:%u this:%p \n", wStatus, flag, this);
            if (flag == 1)
            {
                g_bParseFinished_1 = true;
            }
            else
            {
                g_bParseFinished_2 = true;
            }

			if (pOutVideoFile)
			{
				fclose(pOutVideoFile);
				pOutVideoFile = NULL;
			}

			if (pOutAudioFile)
			{
				fclose(pOutAudioFile);
				pOutAudioFile = NULL;
			}
        }
    }
}
void CDemo::OnVideoCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen, const bool bKeyFrame)
{
    if (pcParser == pFile)
    {
        if (pOutVideoFile && pPacketPtr)
        {
			//NOTICE: 写文件会增加回调延迟, 仅为验证使用
            fwrite(pPacketPtr, 1, wPackLen, pOutVideoFile);
        }
    }
}
void CDemo::OnAudioCallBack(const CMp4File *pcParser, const unsigned char *pPacketPtr, const unsigned short wPackLen)
{
    if (pcParser == pFile)
    {
        if (pOutAudioFile && pPacketPtr)
        {
			//NOTICE: 写文件会增加回调延迟, 仅为验证使用
            fwrite(pPacketPtr, 1, wPackLen, pOutAudioFile);
        }
    }
}


CDemo *g_pDemo_1 = NULL;
CDemo *g_pDemo_2 = NULL;

void quit()
{
	if (g_pDemo_1)
	{
		g_pDemo_1->StopParse();
		delete g_pDemo_1;
		g_pDemo_1 = NULL;
	}

	if (g_pDemo_2)
	{
		g_pDemo_2->StopParse();
		delete g_pDemo_2;
		g_pDemo_2 = NULL;
	}

	HpMp4ParserQuit();
	system("pause");
}

int main()
{
    HpMp4ParserInit();

    unsigned short wRet = MP4PARSER_NO_ERROR;

    g_pDemo_1 = new CDemo(1);
    if (NULL == g_pDemo_1)
    {
        quit();
		return -1;
    }

    wRet = g_pDemo_1->SetFile(TEST_FILE_1, NULL, NULL);
    if (MP4PARSER_NO_ERROR != wRet)
    {
		quit();
		return -1;
    }

	Mp4VideoInfo* pVideoInfo = g_pDemo_1->GetVideoInfo();
	if (pVideoInfo)
		printf("Get video info: duration=%d, bitrate=%d, width=%d, height=%d, FrameRate=%d\n",	\
					pVideoInfo->duration, pVideoInfo->bitrate, pVideoInfo->width, pVideoInfo->height, pVideoInfo->frame_rate);

    wRet = g_pDemo_1->StartParse();
    if (MP4PARSER_NO_ERROR != wRet)
    {
		quit();
		return -1;
    }

/*
    g_pDemo_2 = new CDemo(2);
    if (NULL == g_pDemo_2)
    {
		quit();
		return -1;
    }

    wRet = g_pDemo_2->SetFile(TEST_FILE_2, NULL, NULL);
    if (MP4PARSER_NO_ERROR != wRet)
    {
		quit();
		return -1;
    }

    wRet = g_pDemo_2->StartParse();
    if (MP4PARSER_NO_ERROR != wRet)
    {
		quit();
		return -1;
    }
*/


    do
    {
#ifdef WIN32
        Sleep(1<<10);
#else
		sleep(1);
#endif
	//}while((false == g_bParseFinished_2));  
    //}while( (false == g_bParseFinished_1) && (false == g_bParseFinished_2));  //test StopParse()
    }while( (false == g_bParseFinished_1) /*|| (false == g_bParseFinished_2)*/);  //test Multiple File Parse


	quit();
    return 0;
}

