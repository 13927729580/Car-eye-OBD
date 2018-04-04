/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */
#ifndef __gps__
#define __gps__

#ifdef __cplusplus
extern "C"{
#endif
#define GPS_DATA_MAX 50
#define GPS_DATA_longflag_id 156  //����
#define GPS_DATA_longitude_id 157
#define GPS_DATA_latflag_id 158//γ��
#define GPS_DATA_latitude_id 159
#define GPS_DATA_angle_id 163
#define GPS_DATA_celid_id 160
#define GPS_DATA_lacid_id 161
typedef struct{
	u8 num;
	u8 id[16];//���֧��16������
	u32 data[16];
}_OBDGPS;//OBD���ݷŵ�GPS��


typedef struct{
	_OBDGPS obddata;
  u8 enable;//��־GPS�Ƿ���Ч ����GPRMC�Ķ�λ״̬��ʾ�����Ƿ���� 0x55=����
  u32 latitude;//γ�� ��ʽ:ddmm.mmmm *10000
  u8  latflag;//1=��γN  2=��γS
  u32 longitude;//����  ��ʽ:dddmm.mmmm  10000 = ��
  u8  longflag;//1=����E 2=����W
  u32 speed;//�ٶ� /100 = Km/h
  u32 angle;//�Ƕ� /100 = ��
  u32 ymd;//������
  u32 time;//ʱ��
  u32 timeflag;//ʱ���
  u32 distance;//�������
  u32 fuel;//�����ͺ�
  u32 celid;//С����
  u32 lacid;//λ��������
  u32 itemflag;//�������ȡ��� 0=δ��ȡ 1=�Ѷ�ȡ
              /*
              bit0 = ʱ��
              bit1 = ����
              bit2 = ����
              bit3 = γ��
              bit4 = �ٶ�
              bit5 = �Ƕ�
              bit31 = GPS������Ч
              */
  u32 _3gcheck;//3���������
   char Time808[6];  //add by lilei-2018-04-02
}_GPSUNIT;


typedef struct{
  _GPSUNIT unit[GPS_DATA_MAX];
  u8 oprate;//0x55=���ڲ���
  u8 num;
  u8 in;
  u8 out;
}_GPS;

//���ھ�̬Ư���㷨
typedef struct{
    _GPSUNIT gpscur;
    u8  lathh;//γ��->��
    u8  latmm;//γ��->��
    double  latss;//γ��->��
    u8  longhh;//����->��
    u8  longmm;//����->��
    double  longss;//����->��
    u32 vehiclespeed;//����
}_GPSM;

 extern _GPSUNIT Lgpscur;
void gps_Init(void);
u8 gps_read(void);
void app_gps(void *data);
u8 gps_status_get(void);
u32 gps_speed_get(void);
u8 GPS_2MDM_writeEx(void);
u8 gps_tosvr(void);
u8 gps_data_transformEx(void);

u8 gps_gsmpositionset(u32 cellid, u32 lacid);
u8 gps_mobileinfor(u8 *data);
u8 gps_tobigmem(void);
u8 gps_data_get(u32 *lat, u32 *lon, u32 *speed, u32 *angle);
void gps_obdinsert(u8 id, u32 data);
void gps_vehicle_status_set(u8 flag);

u8 gps_wgs84_degrees(double latin, double lonin, double *latout, double *lonout);
void gps_3g_check_set(u8 flag);
u8 gps_assist_online(void);
u8 gps_assist_online_from_zgex(void);
u8 gps_assist_toOBD(void);
u32 gps_time_get(void);
u32 gps_data_read(u8 *data, u32 datalen);
void gps_demo(void);
void UtcToBeiJingTIme(char *TBuf);
#ifdef __cplusplus
}
#endif
#endif


