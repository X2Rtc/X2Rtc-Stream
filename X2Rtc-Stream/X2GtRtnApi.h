/*
*  Copyright (c) 2022 The X2RTC project authors. All Rights Reserved.
*
*  Please visit https://www.x2rtc.com for detail.
*
*  Author: Eric(eric@x2rtc.com)
*  Twitter: @X2rtc_cloud
*
* The GNU General Public License is a free, copyleft license for
* software and other kinds of works.
*
* The licenses for most software and other practical works are designed
* to take away your freedom to share and change the works.  By contrast,
* the GNU General Public License is intended to guarantee your freedom to
* share and change all versions of a program--to make sure it remains free
* software for all its users.  We, the Free Software Foundation, use the
* GNU General Public License for most of our software; it applies also to
* any other work released this way by its authors.  You can apply it to
* your programs, too.
* See the GNU LICENSE file for more info.
*/
#ifndef __X2_GT_RTN_API_H__
#define __X2_GT_RTN_API_H__
#include <stdint.h>
#include "X2RtcDef.h"

//SN - �����ڵ� - ֻ�����������ȫ�ֵ�·����·
//RN - ·�ɽڵ� - ����ת������A-Region����һ��B-Region
//GN - �����ڵ� - ֻ��������Ҫ�������ַ�����ͬЭ��Ŀͻ���
typedef enum X2GtRtnType
{
	X2GRType_SN = 1,
	X2GRType_RN = 2,
	X2GRType_GN = 4,
}X2GtRtnType;

typedef enum X2GtRtnCode
{
	X2GRCode_OK = 0,
	X2GRCode_NotInited,
	X2GRCode_NotOnline,
	X2GRCode_NotAuthed,
	X2GRCode_ErrParameters,
	X2GRCode_ErrInternal,
	X2GRCode_HasExsited,
	X2GRCode_NotSNode,
	X2GRCode_PubNotAtLocal,

	X2GRCode_LicenseInvalid = 100,
	X2GRCode_LicenceFailed,
	X2GRCode_LicenceTimout,
	X2GRCode_LicenceOemInvalid,
	X2GRCode_LicenceExhaust,
	X2GRCode_LicenceExpired,
	X2GRCode_LicenseInvalidUtcTime,
	X2GRCode_LicenseUnknowErr,

	X2GRCode_Unknow = 1000,
}X2GtRtnCode;

typedef enum X2GtRtnCodec
{
	//* Video
	X2GRCodec_H264 = 0,
	X2GRCodec_H265,
	X2GRCodec_MJpeg,

	X2GRCodec_VideoMax = 19,

	//* Audio
	X2GRCodec_Opus = 20,
	X2GRCodec_Aac,
	X2GRCodec_G711A,
	X2GRCodec_G711U,

	X2GRCodec_AudioMax = 39,

	//* Data
	X2GRCodec_Data = 100,
}X2GtRtnCodec;

typedef enum X2GtRtnRptEvent
{
	X2GREvent_VertifyLicense = 0,	//��Ȩ��Ϣ
	X2GREvent_OutLog,				//�ڲ���־���
	X2GREvent_RtnStats,				//ý������ͳ��
}X2GtRtnRptEvent;

typedef enum X2GtRtnPubState
{
	X2GRPubState_Idel = 0,			//����
	X2GRPubState_Start,				//��ʼ����
	X2GRPubState_Stop,				//ֹͣ����
	X2GRPubState_AudCodecChanged,	//��Ƶ��������ı�
	X2GRPubState_VidCodecChanged,	//��Ƶ��������ı�
}X2GtRtnPubState;

typedef enum X2GtRtnPubEvent
{
	X2GRPubEvent_OK = 0,
	X2GRPubEvent_Failed,
	X2GRPubEvent_Closed,

	X2GRPubEvent_RtnPath = 10,		//ý������·��·��
}X2GtRtnPubEvent;

struct X2MediaData
{
	char* pData;
	int nLen;
	X2GtRtnCodec eCType;
	bool bKeyframe;
	uint16_t nVidScale;
	uint16_t nAudSeqn;
	uint32_t nPts;
	uint32_t nDts;
};

//typedef void(*cbX2GtRtn)();
// Engine�ڲ����¼��ص�
typedef void(*cbX2GtRtnReportEvent)(X2GtRtnRptEvent eEventType, X2GtRtnCode eCode, const char* strInfo);
// ��������״̬�仯
typedef void(*cbX2GtRtnPublishEvent)(void* userData, X2GtRtnPubEvent eEventType, X2GtRtnPubState eState, const char* strInfo);
//* ���ĵ�������Զ�˷��˹���
typedef void(*cbX2GtRtnRecvStream)(void* userData, X2MediaData* pMediaData);
//* ������������Ƶ/��Ƶ������
typedef void(*cbX2GtRtnSubscribeCodecByRemote)(void* userData, const char* strPubId, X2GtRtnCodec eCodec, bool bEnable);
//* ��������������С�����
typedef void(*cbX2GtRtnSubscribeStreamByRemote)(void* userData, const char* strPubId, int nStreamId, bool bEnable);

X2R_API int X2GtRtn_Set_cbX2GtRtnReportEvent(cbX2GtRtnReportEvent func);
X2R_API int X2GtRtn_Set_cbX2GtRtnPublishEvent(cbX2GtRtnPublishEvent func);
X2R_API int X2GtRtn_Set_cbX2GtRtnRecvStream(cbX2GtRtnRecvStream func);
X2R_API int X2GtRtn_Set_cbX2GtRtnSubscribeCodecByRemote(cbX2GtRtnSubscribeCodecByRemote func);
X2R_API int X2GtRtn_Set_cbX2GtRtnSubscribeStreamByRemote(cbX2GtRtnSubscribeStreamByRemote func);

/*Sdk Engine interface*/
X2R_API int X2GtRtn_Init(const char* strConf);
X2R_API int X2GtRtn_DeInit();

X2R_API int X2GtRtn_UpdateGlobalPath(const char* strGlobalPath);

X2R_API int X2GtRtn_PublishStream(const char* strAppId, const char* strStreamId, const char* strResSId, void* userData);
X2R_API int X2GtRtn_UnPublishStream(const char* strAppId, const char* strStreamId);
X2R_API int X2GtRtn_SendPublishStream(const char* strAppId, const char* strStreamId, X2MediaData* pMediaData);

X2R_API int X2GtRtn_SubscribeStream(const char* strAppId, const char* strStreamId, void* userData);
X2R_API int X2GtRtn_UnSubscribeStream(const char* strAppId, const char* strStreamId);
X2R_API int X2GtRtn_SubscribeAudioCodec(const char* strAppId, const char* strStreamId, X2GtRtnCodec eCType, bool bEnable);
X2R_API int X2GtRtn_SubscribeVideoStream(const char* strAppId, const char* strStreamId, int nId, bool bEnable);

#endif	// __X2_GT_RTN_API_H__
