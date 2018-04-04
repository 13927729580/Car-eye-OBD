/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */

#ifndef _APP_USER2_
#define _APP_USER2_

#ifdef __cplusplus
extern "C"{
#endif

/*Ӧ��ʱ�Ӷ���
*ʱ�����������ط���
*  1��ϵͳ�ں�  ÿ�����ݴ���ʱ����
*  2��GPS       ÿ�γɹ���ȡ��GPSʱ��ʱ����
*  3��Ӧ�ü���  ÿ�����
*/
typedef struct{
 u32 time_rtc;
 u32 time_rtc_update;
 u32 time_gps;
 u32 time_gps_update;
 u32 time_app;
 u32 time_app_update;
}_stime;

void Lstime_gps_update(u32 time);
void Lstime_sys_update(u32 time);
u32 Lstime_get(void);

void app_linking(void *data);
void app_keep_step_set(u8 state);
void app_svrlink(u8 state);
void linking_init(void);

void app_sleep_flag_set(u8 flag);
#ifdef __cplusplus
}
#endif
#endif


