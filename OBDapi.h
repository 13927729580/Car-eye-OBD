/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */



#ifndef _OBD_API_
#define _OBD_API_

#ifdef __cplusplus
extern "C"{
#endif
#define CMD_ROUT 0x05
#define CMD_UPDATE_FORCE 0x83 //��ָ������OBD��M2M֮��ͨѶ ���ᵽ������ OBDҪ��M2Mָ��ǿ������ĳ���ļ� 2014-10-14 10:24
#define CMD_UPDATE 0x84
#define CMD_GSENSOR 0x15
#define CMD_M2M_CONTROL 0x60
#define CMD_WAKEUP 0x61
#define CMD_VEHICLESET 0x80
#define CMD_OBDDATA_AUTO 0x6c
#define CMD_OBDDATA_MENU 0x8c
#define CMD_OBD_STATUS 0x8e
#define CMD_VEHICLE_STATUS 0x8d
#define CMD_OBD_RESTART 0x8f
#define CMD_GPS_ASSIST 0x14  //OBD������������
#define CMD_AUDIO_PLAY 0x86  //��������
#define CMD_FILE_LOAD 0x85  //ͨ��PC�����ļ���M2M
                            /*PC:0x85  0x07  type  size  filename  //type���ļ�����char  size:�ļ���Сint  filename���ļ���char
                            * M2M:0xc5 0x07/0x00 framesize //00=����  01=������  framesize:ÿ֡���ݴ�С���512 Ĭ��100�� ���ָ��ͨ��ֻ�ڲ��������ػ���Ҫ�޸�ÿ֡���ݴ�Сʱʹ�� ����ֱ����һ��ָ��
                            * M2M:0xc5  0x03  index  //index=���������������ÿ֡Ĭ�����100���ֽ� short
                            * PC��0x85 0x03 index data
                            * M2M:0xc5 0x04       //��ȡ����
                            * PC: 0x85 0x04 cs   //PC�����������ݵĺ�У�� int 
                            */
                            
#define OBD_RX_BUF_MAX 6144  //6K���ݿռ�
#define OBD_RX_MSG_MAX 50
typedef struct{
  u16 index;
  u8 len;
  u8 cmd;
  u8 frameindex;
  u8 cmd_sub;
  u32 time;//ʱ���
}_MSG;
typedef struct{
   u8 data[OBD_RX_BUF_MAX];
   u16 dataindex;
   u16 datalen;
   u8 msgin;
   u8 msgout;
   u8 msgnum;
   _MSG msg[OBD_RX_MSG_MAX];
 }_OBDDB;

//�г̼�¼ 
typedef struct{
    u32 starttime;
    u32 endtime;
    u32 distance;
    u32 fuel;
}_ROUTE;
typedef struct{//������ �������ж�
	u32 time;
	double speed0;//��ǰ�ٶ�
	double speed1;//��⵽���ٶ�
	double speed2;//��ǰ�ٶ�
	
	u32 time1;//�Ƕȼ��
	double ang0;//�Ƕȼ��
	double ang1;
}_speedcheck;

 //���ڱ�����Ҫ��ʱ�����ָ��,����OBD�����ǹ㲥�����ݾ�Ϊ��ʱ����
typedef struct{
	u8 data[64];//���ֻ��Ϊ64���ֽ�
  u8 len;
  u8 cmd;
  u8 cmd_sub;
  u32 time;//ʱ���
}__MSG;
typedef struct{
	u8 msginex;
	u8 msgoutex;
	u8 msgnumex;
	__MSG msgex[OBD_RX_MSG_MAX];
 }__OBDDB;
 
u32 Lenginespeedmax_get(void);
u32 Lvehiclespeedmax_get(void);
void back2OBD_7f(u8 cmd, u8 status);
void obd_init(void);
u8 obd_read(void);
u8 obd_data_read(u8 *cmd1, u8 *cmd2, u8 **data, u16 *datalen);
u8 obd_data_read_ex(u8 *cmd1, u8 *cmd2, u8 **data, u16 *datalen);
u8 obd_datadeal(void);
u8 obd_write(u8 *data, u16 datalen);
u8 obd_writeEx(u8 *data, u16 datalen);//͸��
u8 obd_update_keep(void);
void obd_Rx_bufclr(void);
u8 obd_cmdclear(u8 cmd, u8 cmdsub);
void hw_th_unable_set(u8 flag);

u8 obd_vehiclespeedget(u32 *speed);
u8 obd_heard(void);
u16 obd_write_tomessage(u8 *data, u16 datalen);

u8 SVR_rout_tosvr(u8 *data, u32 datainlen);
u8 obd_cmd84(u8 cmd_sub, u8 *data, u16 datalen);
void obd_vol_offenable(void);
u8 obd_cmd8d(u8 cmd_sub);
void back2SVR8405(u8 status,u8 type,u16 ver,u16 id);
#ifdef __cplusplus
}
#endif
#endif

