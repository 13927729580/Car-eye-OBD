/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */

#ifndef _AT_API_
#define _AT_API_

#ifdef __cplusplus
extern "C"{
 #endif


/*��¼SIM����Ϣ
*/
typedef struct{
	u8 tellen;//�绰�����ֽڸ���
	u8 tel[12];//�绰���� ����11���ַ�ֻȡ11��
	u8 ccidlen;//ccid�ֽڸ���
	u8 ccid[21];//ccid  ����20���ַ�ֻȡ20��
	//ccid������Ϣ
	u8 tsp;//��Ӫ�̴���
	       /*0 = δ֪��
	         1 = �ƶ�
	         2 = ��ͨ
	         3 = ����
	       */
	u8 business;//ҵ������  ccid.7
	u8 simfun;//SIM������ һ��Ϊ0��Ԥ����SIM��Ϊ1
	u8 province;//��ʡ����
	u8 year;//���
	u8 supplier;//��Ӧ�̴���
}_SIMinf;

/*��¼M2M��Ϣ
*/
typedef struct{
	u8 imeilen;
	u8 imei[24];
	u8 cimilen;
	u8 cimi[24];
	//cimi������Ϣ
	u16 mmc;//�ƶ����Һ���
	u8 mnc;//�ƶ�����
}_M2Minf;



/*��¼����״̬
*/
typedef struct{
	u8 modemstart;//modem�Ƿ��Ѿ����� 0 = δ���� 1=������
	u8 gprsbond;//GPRS���� 0 = δ���� 1=����
	u8 gprslink;//GPRS���� 0 = δ���� 1=������
	u8 csq;
	u8 m2mstatus;
	u8 localiplen;
	u8 localip[25];
	u8 seraddr[128];//��������ַ��֧��IP�Լ����� ���Ȳ�����128���ַ�
	u32 port;
}_M2Mstatus;

void AT_Init(u8 *seraddr, u32 port);
/*M2M���*/ 
u8 AT_AT(void);
u8 AT_CPIN(void);

//��λ
u8 AT_CFUN(u8 status);

//����
u8 AT_EQV(void);

//��ȡM2M��Ϣ
u8 AT_GSN(u8 *IMEI);
u8 AT_CIMI(u8 *CIMI);
u8 AT_CCID(u8 *CCID);
u8 AT_CNUM(u8 *num);

//GPRS
u8 AT_CREG(void);
u8 AT_CGATT(void);
u8 AT_CGREG(void);
u8 AT_CIICR(void);
u8 AT_CSQ(void);
u8 AT_CSTT(u8 flag);
u8 AT_CIFSR(u8 *ip);
u8 AT_CIPSTART(u8 *ip, u32 port);
u8 AT_CIPSHUT(void);
u8 AT_CIPCLOSE(void);
u8 AT_CIPSEND(u8 *data, u16 datalen);
u8 AT_CENG(void);
u8 AT_CMGDA(void);
u8 AT_SMSENDex(u8 *tell, u8 *data);
u8 AT_SMSinit(void);
//SMS����
u8 AT_CNMI(void);

//Ӧ�ýӿ�
void m2m_startcheck(void);
void sim_information(void);
void m2m_information(void);
void m2m_gprsbond(void);
void m2m_gprslink(void);
u8 * m2m_imeiget(void);
//״̬��Ϣ
u8 m2m_status_modemstart(void);
u8 m2m_status_gprsbond(void);
u8 m2m_status_gprslink(void);
u8 m2m_status(void);
void m2m_statusSet(u8 status);

u8 m2m_svr_addrtemplerset(u8 *addr, u32 port);
u8 m2m_svr_addrreback(void);
u8 m2m_ser_addrcheck(u8 *addr);
void AT_TTS(char *mus);
void AT_CREC(char *file, u8 mu);

unsigned char UDP_Creat(void);
void UDP_Cipclose(void);
unsigned char UDP_Send(unsigned char *send, int sendlen);

void UDP_Cipclose(void);
u8 AT_CIPSEND_test(u8 *data, u16 datalen);
u8 AT_CIPSTART_test(u8 *ip, u32 port);
u8 AT_CIPCLOSE_test(void);

u32 AT_VER(void);
u8 *m2m_verget(void);
u32 m2m_volget(void);
u8 *m2m_ccidget(void);
void m2m_ccidread(void);
u8 AT_CADC(void);
u32 AT_CBC(void);
void AT_ATH(void);
void AT_ATA(void);

void AT_CENG_CELL(u32 *lac, u32 *cell);
#ifdef __cplusplus
}
#endif
#endif
  

