/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */


#ifndef __DB__
#define __DB__

#ifdef __cplusplus
extern "C"{
 #endif
#define UPDATE_FILE_MAX 6
typedef struct{
   u8 type;
   u32 size;
   u32 cs;
   u8 name[32];//�ļ���������32���ֽ�
}_updatefile;
typedef struct{
  u8 filenum;
  _updatefile file[UPDATE_FILE_MAX];
}_update;

/*OBD���ݱ��ݣ���ֹOBD�γ����ݶ�ʧ
*2015/6/8 6:31  fangcuisong
*/
typedef struct{
	u32 engine_on_time_135;//������ʱ��
	u32 engine_on_time_136;//��������ʱ��
	u32 fuel_used_138;//���ͺ�
	u32 fuel_used_139;//�����ͺ�
	u32 fuel_used_av_141;//��ƽ���ͺ�
	u32 fuel_used_av_142;//����ƽ���ͺ�
	u32 dist_147;//�����
	u32 dist_148;//�������
	u32 gpstime;
	u32 gpslat;//γ��
	u32 gpslon;//����
}_OBD_DB;
//2015/6/8 6:47
u8 db_obd_init(void);
u8 db_obd_insert(u8 id, u32 data);
u8 db_obd_save(void);
u8 db_obd_reset(void);
/*�����ļ����Ͷ���
*/
#define UPDATE_OBD_APP 12
#define UPDATE_OBD_CAN 2
#define UPDATE_MUSIC 3
#define UPDATE_OBD_LICENSE 4
#define UPDATE_M2M_APP 16
#define UPDATE_M2M_LICENSE 6

u8 db_init(void);
u8 db_update_init(void);
u8 db_update_save(u8 type, u8 *name, u32 size, u32 cs);
u8 *db_update_fileget(u8 type);
/*****************************************************************/
//��¼��������Ϣ
typedef struct{
   u8 svraddr[128];//��������ַ ������IP Ҳ����Ϊ����
   u32 port;//�������˿ں�
   u8 svraddr1[128];//������������ַIP
   u32 port1;//�����������˿ں�
   u8 svrtell_t[24];//���ŷ��������ͺ���
   u8 svrtell_r[24];//���ŷ��������պ���
   u8 apn[16];//APN
   u8 ttell[32];//���ڰβ���ԵĶ��ź���
   u8 telx[5][24];//�û�����
   u16 backcircle;//���ݷ������� SΪ��λ��0��ʾĬ�����豸����
   u8 media;//��ý�岥��ʹ�ܿ���  0=��ֹ  1=ʹ�� Ĭ��ʹ��
 }_SVR;
 
 /*GPS����
 */
 typedef struct{
 	u32 lac;
 	u32 lon;
 	u32 cellid;
 	u32 localid;
}_DBGPS;
 /*�����������ݼ�¼�ļ��б�
 *ÿ���ļ����100K
 *���10���ļ� ���1M�����ݴ洢
 *****************************************************/
 #define DB_FILE_MAX 10
 typedef struct{
    u8 filenum;//��Ч�ļ���
    u32 filesize[DB_FILE_MAX];//ÿ���ļ���С
 }_DBFILE;
 void db_fileinit(void);
 u32 db_fileread(u8 *data);
 u8 db_filesave(u8 *data, u32 datalen, u8 flag);
 u8 db_filecheck(void);
 
 u8 db_svr_default(void);
 u8 db_svr_save(void);
 u8 db_svr_init(void);
 u32 db_svr_cyclget(void);
 void db_svr_cyclset(u32 cycl);
 u32 db_svr_portget(void);
 void db_svr_portset(u32 port);
 u32 db_svr_port1get(void);
 void db_svr_port1set(u32 port);
 u8 *db_svr_sttelget(void);
 void db_svr_sttelset(u8 *stel);
 void db_svr_srtelset(u8 *stel);
 u8 *db_svr_srtelget(void);
 u8 *db_svr_addrget(void);
 u8 *db_svr_addr1get(void);
 void db_svr_addrset(u8 *addr);
 void db_svr_addr1set(u8 *addr);
 void db_svr_telxset(u8 *tel, u8 telnum);
 
 u16 rout_fileread(u8 **dataptr);
 u8 rout_filesave(u8 *dataptr, u32 datalen);
 
u8 db_update_vercheck(u16 ver, u8 type);

//GPS����
u8 db_gps_init(void);
u8 db_gps_get(u32 *lac, u32 *loc);
u8 db_gps_save(void);
void db_gps_set(u32 lac, u32 loc);

u8 db_save(u8 *file, u32 filesize, u8 *filename);

void db_svr_ttellset(u8 *ttell);
u8 *db_svr_ttellget(void);
void db_svr_apnset(u8 *apn);
u8 *db_svr_apnget(void);

u8 db_svr_mmcget(void);
void db_svr_mmcset(u8 flag);
u8 db_swver_save(u8 *infor);
void db_obd_gpsclr(void);
void db_obd_gpsset(u32 gpstime, u32 gpslat, u32 gpslon);
void db_gps_cellsave(void);
u8 db_gpscell_get(u32 *lac, u32 *cel);

u8 db_svr_adcwarmget(void);
void db_svr_adcwarmset(u8 flag);
#ifdef __cplusplus
}
#endif
#endif



