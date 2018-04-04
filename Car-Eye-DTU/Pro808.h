/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018 
 */


#ifndef _PRO_808_
#define _PRO_808_


#define 		UP_UNIRESPONSE			0x0001		//�ն�ͨ��Ӧ��//
#define 		UP_HEARTBEAT				0x0002		//�ն�����//
#define 		UP_REGISTER				0x0100		//�ն�ע��//
#define 		UP_LOGOUT					0x0101		//�ն�ע��//
#define 		UP_AUTHENTICATION		0x0102		//�ն˼�Ȩ//
#define         UP_POSITIONREPORT             0x0200


#define 		MSGBODY_NOPACKAGE_POS		13
#define 		MSGBODY_PACKAGE_POS			17
#define 		PROTOCOL_SIGN					0x7E		//��ʶλ//
#define 		PROTOCOL_ESCAPE				0x7D		//ת���ʶ//
#define 		PROTOCOL_ESCAPE_SIGN			0x02		//0x7e<-->0x7d�����һ��0x02//
#define 		PROTOCOL_ESCAPE_ESCAPE		0x01		//0x7d<-->0x7d�����һ��0x01//


#define 		DOWN_UNIRESPONSE				0x8001		//ƽ̨ͨ��Ӧ��//
#define 		DOWN_REGISTERRSPONSE		0x8100		//�ն�ע��Ӧ��//
#define   	OBDBUFFER_SIZE				20
#define 		MAX_PROFRAMEBUF_LEN			512

#define BigLittleSwap16(A)  ((((unsigned short)(A) & 0xff00) >> 8) | \
							(((unsigned short)(A) & 0x00ff) << 8))

#define BigLittleSwap32(A)  ((((unsigned long)(A) & 0xff000000) >> 24) | \
							(((unsigned long)(A) & 0x00ff0000) >> 8) | \
							(((unsigned long)(A) & 0x0000ff00) << 8) | \
							(((unsigned long)(A) & 0x000000ff) << 24))



#define 		   LOOP_BUFFER_SIZE			5

#pragma pack(1)


typedef struct
{
	u8 u8Para1;		//����һ//
	u8 u8Para2;		//������//
	u16 u16Para3;	//������//
	u16 u16Para4;	//������//
	u32 u32Para5;	//������//
	u32 u32Para6;	//������//
	u8* buf;
}sProPara;

typedef struct
{
   	u8 flag;
   	unsigned short len;
   	unsigned char buffer[400];

}_SendBuffer;
typedef struct
{
   	unsigned head;
   	unsigned char tail;
   	_SendBuffer SendBuf[LOOP_BUFFER_SIZE];

}_SendDataLoop;


typedef union
{
	struct bit
	{
		u16 msglen:10;		//��Ϣ�峤��//
		u16 encrypt:3;		//���ݼ��ܷ�ʽ// //������λ��Ϊ0����ʾ��Ϣ�岻����//  ����10λΪ1����ʾ��Ϣ�徭��RSA�㷨����
		u16 package:1;		//�ְ����//
		u16 res1:2;			//����//
	}bit;
	u16 val;
}sMsgattribute;		//��Ϣ�����Ը�ʽ�ṹ//


typedef struct
{
	u16 id;						//��ϢID//
	sMsgattribute  attribute;		//��Ϣ������//
	u8 phone[6];					//�ն��ֻ���//
	u16 serialnum;				//��Ϣ��ˮ��//
	u16 totalpackage;				//��Ϣ�ܰ���//  // ����Ϣ�ְ�����ܰ���//
	u16 packetseq;				//�����//  //��1��ʼ//
}sMessagehead;		//��Ϣͷ����//




typedef union
{
	struct bita
	{
		u32 sos:1;						//������Ƴ�����������غ󴥷�//
		u32 overspeed:1;					//���ٱ���//
		u32 fatigue:1;						//ƣ�ͼ�ʻ//
		u32 earlywarning:1;				//Ԥ��//
		u32 gnssfault:1;					//GNSSģ�鷢������//
		u32 gnssantennacut:1;				//GNSS����δ�ӻ򱻼���//
		u32 gnssantennashortcircuit:1;		//GNSS���߶�·//
		u32 powerlow:1;					//�ն�����ԴǷѹ//
		
		u32 powercut:1;					//�ն�����Դ����//
		u32 lcdfault:1;					//�ն�LCD����ʾ������//
		u32 ttsfault:1;						//TTSģ�����//
		u32 camerafault:1;					//����ͷ����//
		u32 obddtc:1;						//OBD������//
		u32 res1:5;						//����//

		u32 daydriveovertime:1;			//�����ۼƼ�ʻ��ʱ//
		u32 stopdrivingovertime:1;			//��ʱͣ��//
		u32 inoutarea:1;					//��������//
		u32 inoutroad:1;					//����·��//
		u32 roaddrivetime:1;				//·����ʻʱ�䲻��/����//
		u32 roaddeviate:1;				//·��ƫ�뱨��//
		u32 vssfault:1;					//����VSS����//
		u32 oilfault:1;						//���������쳣//
		u32 caralarm:1;					//��������(ͨ������������)//
		u32 caraccalarm:1;				//�����Ƿ����//
		u32 carmove:1;					//�����Ƿ�λ��//
		u32 collision:1;					//��ײ�෭����//
		u32 res2:2;						//����//
	}bita;
	u32 val;
}sbitalarm;



typedef union
{
	struct bits
	{
		u32 acc:1;						//ACC  0: ACC��;1:ACC��//
		u32 location:1;					//��λ  0:δ��λ;1:��λ//
		u32 snlatitude:1;					//0:��γ:1:��γ//
		u32 ewlongitude:1;				//0:����;1:����//
		u32 operation:1;					//0:��Ӫ״̬:1:ͣ��״̬//
		u32 gpsencrypt:1;					//0:��γ��δ�����ܲ������;l:��γ���Ѿ����ܲ������//
		u32 trip_stat:2;					//00���ȴ����г�01���г̿�ʼ10��������ʻ11���г̽���,(�и�������0x07)//
		u32 Alarm_en:1;					//�������ܴ򿪹ر�//
		u32 ResetState:1;					//�ϵ�״̬�ϱ�//

		u32 oilcut:1;						//0:������·����:1:������·�Ͽ�//
		u32 circuitcut:1;					//0:������·����:1:������·�Ͽ�//
		u32 doorlock:1;					//0:���Ž�����1�����ż���//
		u32 gpsen:1;						// 1:��GPS���ݣ����ֶ�ռ�� 0����GPS����//
		u32 res2:18;						//����//
	}bits;
	u32 val;
}sbitstate;

typedef struct
{
	sbitalarm alarm;
	sbitstate state;
	u32 latitude;					//γ��(�Զ�Ϊ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��)//
	u32 longitude;				//����(�Զ�Ϊ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��)//
	u16 atitude;					//���θ߶ȣ���λΪ��(m)//
	u16 speed;					//�ٶ� 1/10km/h//
	u16 direction;					//���� 0-359,����Ϊ0��˳ʱ��//
	u8 time[6];					//ʱ�� BCD[6] YY-MM-DD-hh-mm-ss(GMT+8ʱ�䣬����׼֮���漰��ʱ������ô�ʱ��)//
}sPositionbasicinfo;


typedef struct
{

   u8  Sim808Step;
   u8  AnsWerFalg;
   u8 AuthenFlag;
   u16 AutionLen;
   u8 AutionBuf[16];

}SIM808DEAL;


#pragma pack()

extern u8 ProTBuf[512];
extern u8 ProTempBuf[512];
extern sProPara  ProPara;
u16 ProFrame_Pack(u8 *dec,u16 Cmd,sProPara* Para,u8* Tempbuf);
u8 SVR808_FameDeal(void);
u8   Up_Register(void);
u8  UP_Authentication(void);
void ProFrameRec(u8 data);
void ProFramePrase(u8* FrameData,u16 Framelen,u16* ResId);

#endif

