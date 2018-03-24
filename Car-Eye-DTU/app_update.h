/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018 
 */

#ifndef _APP_UPDATE_
#define _APP_UPDATE_

#ifdef __cplusplus
extern "C"{
#endif
typedef struct{
	u8 flag;
	u8 *data;
	u8 name[35];
	u32 dataindex;//�����ֽڸ���
	u32 frameindex;//֡���
	u32 datalen;//�����ܳ���
	u32 cs;//������У��
}_APPUPDATE;
 

void update_init(void);
u8 update_start(u32 datalen, u8 type, u8 *name);
void update_end(void);
u8 update_datain(u8 *data, u16 datalen);
u8 update_datainEx(u8 *data, u16 datalen, u32 findex);
u8 update_cs(u32 cs);
u8 update_do(void);

u8 update_obdlicense(void);
u8 update_obdapp(void);
u8 update_obdmusic(void);
#ifdef __cplusplus
}
#endif
#endif


