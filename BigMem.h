/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */


#ifndef _BIG_MEM_
#define _BIG_MEM_

#ifdef __cplusplus
extern "C"{
#endif
#define BIG_MEM_MSG_MAX 1280  //���Ա��潫��35���ӵ�OBD+GSP����
#ifndef DATA_LOG_FILE_MAX
   #define DATA_LOG_FILE_MAX 1024*100 //�������ݻ����ļ���С
#endif
#ifndef BIG_MEM_MAX
    #define BIG_MEM_MAX 1024 * 150//150K
#endif
typedef struct{
  u32 index;
  u16 len;
}_BIGMSG;
typedef struct{
   u32 dataindex;//������������ Ҳ�ǻ��������ݴ�С
   u32 datalen;//�������е�������
   u16 msgin;//д��֡���
   u16 msgout;//����֡���
   u16 msgnum;//֡����
   _BIGMSG msg[BIG_MEM_MSG_MAX];//ÿ֡��Ϣ
 }_BIGMEM;
 
void bigmem_init(void);

u8 *bigmem_get(u8 type);
void bigmem_free(void);

u8 bigmem_obdgps_in(u8 *datain, u16 datalen);
u16 bigmem_obdgps_out(u8 *dataout);
u8 bigmem_obdgps_tosvr(void);
u8 bigmem_save(void);
#ifdef __cplusplus
}
#endif
#endif

