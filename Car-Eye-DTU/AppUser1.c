/********************************************************************
 * Include Files
 ********************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <rt_misc.h>

#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_periphery.h"
#include "eat_uart.h"
#include "definedata.h"
#include "UartTask.h"
#include "AppUser1.h"
#include "AppUser2.h"
#include "OBDapi.h"
#include "ATapi.h"
#include "SVRapi.h"
#include "BigMem.h"
#include "db.h"
#include "Pro808.h"

extern u32 Ltaskstartflag;
/*����һ���ڴ��������ڶ�̬����
***************************************/
extern u8 APP_UPDATE_FLAGEX;
static u8 MDM_Link_Service_Flag = 0;/*
                                      M2M����������ӱ�־��M2M���ӵ�������������Ϊ0x55������Ϊ0x00
                                    */
/*��ʱ������AppUser2��⵽�豸��������Ͽ�����øýӿ�֪ͨAppUser1�豸�Ѿ���������Ͽ�
*/
static u8 mdm_unlinkset_flag = 0;
static u8 Lobd_start_mode = 0;

void MDM_unlinkset(u8 state)
{
	if(m2m_status() < 8)return;
	if(0x55 == mdm_unlinkset_flag)return;
	mdm_unlinkset_flag = 0x55;
	MDM_Link_Service_Flag = state;
	mdm_unlinkset_flag = 0;
}

static unsigned char Lpowr_lost_flag = 0;
unsigned char Lpowr_lost_flag_get(void){
	return Lpowr_lost_flag;
}

/*���ص�ǰʱ�� s
*���Ϊ524�� �������0��ʼ����
****************************************/
u32 user_time(void)
{
	u32 time;
	
	time = eat_get_current_time();//31.25us 32/1000000
	return time * 32/1000000;
}
/*
void user_meminit(void){
	eat_bool mem_ini_flag;
	mem_ini_flag = eat_mem_init(Lbigmemory, BIG_MEM_MAX);
    if(EAT_TRUE == mem_ini_flag)
    {
        LbigmemFlag = 0x55; 
    }else
    {
        LbigmemFlag = 0x00;
    }   
}

u8 *usr_mallocEx(void){
	return Lbigmemory;
}
void *user_malloc(u32 size){
	if(0 == LbigmemFlag)user_meminit();
	if(0 == LbigmemFlag)return NULL;
	if(size >= BIG_MEM_MAX)return NULL;
	return eat_mem_alloc(size);
}

void *user_mfree(void *addr){
	if(NULL == addr)return NULL;
	if(0 == LbigmemFlag)return NULL;
	return eat_mem_free(addr);
}
*/
static _bdata Lbdata1;//GPS�㲥����
static _bdata Lbdata2;//OBD�㲥����
extern u8 Lbigmemory_send[EAT_RTX_BUF_LEN_MAX];//���ڴ����ݷ��ͻ�����
void Lbdata1_insert(u8 id, u32 datain)
{
	u8 u8index;
	
	if(0x88 == Lbdata1.unitesdone)return;//������Ҫ����
	for(u8index = 0; u8index < Lbdata1.num; u8index ++)
	{
		if(id == Lbdata1.units[u8index].id)
		{
			 Lbdata1.units[u8index].data = datain;
			 return;
		}
	}
	if(u8index >= Lbdata1.num && Lbdata1.num < BROAD_UNIT_MAX)
	{
		Lbdata1.units[Lbdata1.num].id = id;
		Lbdata1.units[Lbdata1.num].data = datain;
		Lbdata1.num ++; 
	}
}
void Lbdata1_insert_done(void){
	Lbdata1.unitesdone = 0x88;
	
}

void Lbdata2_insert(u8 id, u32 datain){
	u8 u8index;
	
	if(0x88 == Lbdata2.unitesdone)return;//������Ҫ����
	for(u8index = 0; u8index < Lbdata2.num; u8index ++){
		if(id == Lbdata2.units[u8index].id){
			 Lbdata2.units[u8index].data = datain;
			 return;
		}
	}
	if(u8index >= Lbdata2.num && Lbdata2.num < BROAD_UNIT_MAX){
		Lbdata2.units[Lbdata2.num].id = id;
		Lbdata2.units[Lbdata2.num].data = datain;
		Lbdata2.num ++; 
	}
}
void Lbdata2_insert_done(void){
	Lbdata2.unitesdone = 0x88;
}

/*�������ڷ��� 0=δ�����κ�����
*              1=�ɹ�����
*/
u8 Lbdata_toserver(void)
{
	u8 backtosvr[256], unitindex, u8result,dataflag;
	u32 u32index,datalen,datalent,timeflag;
	static unsigned char Lbdata2num = 0;//OBD����ÿ���ӷ���1��
	
	if(0x88 == Lbdata1.unitesdone || 0x88 == Lbdata2.unitesdone);
	else return 0;
	datalen = 0;
	dataflag = 0;
	if(0x88 == Lbdata1.unitesdone)
	{
		  Lbdata1.num = 0;
	    	 Lbdata1.unitesdone = 0x00;
		 user_debug("Fill in Gps 808\r\n");
		 ProFrame_Pack(ProTBuf,UP_POSITIONREPORT,&ProPara,ProTempBuf);
		
	 }
	  
  
 
	user_debug("i:Lbdata_toserver:[%d-%02x]", datalen,dataflag);
	return 1;
}
u8 MDM_Rx_buf[EAT_UART_MDM_RX_BUF_LEN_MAX + 1] = {0};//
u8 MDM_Rx_buf_Temp[EAT_UART_RX_BUF_LEN_MAX + 1] = {0};
u8 MDM_Tx_buf[EAT_UART_TX_BUF_LEN_MAX + 1] = {0};//�û���������MDMӦ�ô���  
u8 MDM_App_buf[EAT_UART_TX_BUF_LEN_MAX + 1] = {0};//�û���������MDMӦ�ô���  
static u16 MDM_Tx_buf_Num = 0;
static u16 MDM_Rx_buf_In = 0;
static _MDM mdm = {0};
u8 LMDM_SEND_ENABLE = 0;
static u8 SVR_EVENT_30 = 0;//ÿ30���������Ϣ
static u8 SVR_EVENT_01 = 0;//ÿ1���������Ϣ
static u8 SVR_EVENT_MOBILE_INFOR_UPDATE = 0;//���»�վ����
static u8 MDM_Tx_buf_flag = 0;//MDM_Tx_buf ʹ�ñ�־
static u8 mdm_msgread_flag = 0;//MODEM����
static u8 m2m_obd_heard = 0; //10s��������

static u8 m2m_obd_heardflag = 0;
u8 m2m_obd_heard_set(u8 data){
	if(0x55 == m2m_obd_heardflag)eat_sleep(1);
	m2m_obd_heardflag = 0x55;
	if(1 == data){
		if(m2m_obd_heard < 10)m2m_obd_heard ++;
	}
	else m2m_obd_heard = 0;
	m2m_obd_heardflag = 0x00;
	return 0;
}

static u8 svr_event_mobileinfor_updateflag = 0;
u8 svr_event_mobileinfor_update(u8 state){
	if(0x55 == svr_event_mobileinfor_updateflag)eat_sleep(1);
	svr_event_mobileinfor_updateflag = 0x55;
	SVR_EVENT_MOBILE_INFOR_UPDATE = state;
	svr_event_mobileinfor_updateflag = 0x00;
	return 0;
}

static u8 svr_event30_enable_flag = 0;
void svr_event30_enable(u8 state)
{
	if(0x55 == svr_event30_enable_flag)eat_sleep(1);
	svr_event30_enable_flag = 0x55;
	SVR_EVENT_30 = state;
	svr_event30_enable_flag = 0;
}

static u8 svr_event01_enable_flag = 0;
void svr_event01_enable(u8 state){
	if(0x55 == svr_event01_enable_flag)eat_sleep(1);
	svr_event01_enable_flag = 0x55;
	SVR_EVENT_01 = state;
	svr_event01_enable_flag = 0;
}
/*************************************************************************
*DES   : MDM��Ϣ�ṹ���ʼ��
*        modem�������ݵ�MDM_Rx_buf�У��ýṹ�����ڱ���ÿһ֡����Ϣ
*INPUT : NULL
*RETURN: NULL
*********************************************************************************/
void mdm_Init(void){
	u8 u8t1;
	
	MDM_Rx_buf_In = 0;
	
	mdm.MDMmsgNum = 0;
	mdm.MDMmsgIn = 0;
	mdm.MDMmsgOut = 0;
	
	MDM_Tx_buf_Num = 0;
	
	LMDM_SEND_ENABLE = 0;
	MDM_Tx_buf_flag = 0;
	mdm_msgread_flag = 0;
	SVR_EVENT_MOBILE_INFOR_UPDATE = 0;
	
	m2m_obd_heard = 0;
	
	Lbdata1.unitesdone = 0;
	Lbdata2.unitesdone = 0;

	Lobd_start_mode=0;       										

/*����ջ�����
*Ϊ�˷�ֹmodem���͵����ݱ��쳣������ýӿڱ�����APP����
*�Ͳ�ӿڲ��������
*�ýӿڲ�֧�����룬��˲��������жϽӿ��е��ã�Ҳ���ʺ����ڰ�ʱ��Ƭ�л���ϵͳ
****************************************/
void MDM_RxBufClr(void){
	MDM_Tx_buf_Num = 0;
	MDM_Rx_buf_In = 0;
	mdm.MDMmsgNum = 0;
	mdm.MDMmsgIn = 0;
	mdm.MDMmsgOut = 0;
}


/*�������ȴ���ϵͳ��ָ��
*ͨѶ�жϵ�
*�ýӿ�ֻ�ṩMDM_DataToApp����
*RETURN: ��0 = ϵͳָ�� �Ѿ�����
         0 = ��ϵͳָ��
***************************************/
u8 MDM_SystemCommand(u8 *data, u16 datalen){
	u16 index;
	
	if(NULL == data || 0 == datalen)return 0;
	for(index = 0; index < datalen; index ++)
	{
		if(0xa5 == *(data + index) && 0xa5 == *(data + index +1))
		{
			//user_debug("SystemC:%02x-%02x", *(data + index + 4),*(data + index +5));
			app_svrlink(0);
			break;
		}
	}
	index = 0;
	if('C' == *(data + index) && 'L' == *(data + index + 1) && 'O' == *(data + index + 2) && 'S' == *(data + index + 3) && 'E' == *(data + index + 4))
	{
		//�����Ѿ��Ͽ� ��Ҫ��������
		m2m_statusSet(4);
		user_debug("i:Restart:close");
		return 1;
	}
	return 0;
}



/*
*DES   : ���ݴ�MDM_Rx_buf����MDM_App_buf��
*INPUT : NULL
*RETURN: ���ݸ���
*/
u16 MDM_DataToApp(u8 **dataptr)
{
	u16 datalen,dataindextemp;
	u16 index,smsindex;
	
	if(0 == mdm.MDMmsgNum)return 0;
	dataindextemp = 0;
	memset(MDM_App_buf, 0, EAT_UART_TX_BUF_LEN_MAX);
	//dataindextemp = mdm.msg[mdm.MDMmsgOut].MDMmsg_Index;
	for(datalen = 0; datalen < mdm.msg[mdm.MDMmsgOut].MDMmsg_Datalen && datalen < EAT_UART_TX_BUF_LEN_MAX; datalen ++)
	{
		MDM_App_buf[datalen] = MDM_Rx_buf[mdm.msg[mdm.MDMmsgOut].MDMmsg_Index + dataindextemp];
		dataindextemp ++;
		if(mdm.msg[mdm.MDMmsgOut].MDMmsg_Index + dataindextemp >= EAT_UART_MDM_RX_BUF_LEN_MAX)
		{
			mdm.msg[mdm.MDMmsgOut].MDMmsg_Index = 0;
			dataindextemp = 0;
		}
	}
	mdm.MDMmsgOut ++;
	if(mdm.MDMmsgOut >= MDM_MSG_MAX)mdm.MDMmsgOut = 0;
	if(0x55 == mdm_msgread_flag)eat_sleep(1);
	mdm_msgread_flag = 0x55;
	mdm.MDMmsgNum --;
	mdm_msgread_flag = 0x00;
	index = 0;
	for(index = 0; index < datalen; index ++)
	{
		if(0x0d == MDM_App_buf[index] || 0x0a == MDM_App_buf[index]);
		else break;
	}
  	if(MDM_SystemCommand(&MDM_App_buf[index], datalen - index) != 0)
	{
  		return 0;
  	}
  	if(datalen > 4)
	{//��ͬ��Ӧ��M2M��һ���Է��� �����Ҫ���з�֡����  ��ȡ��Ҫ������
      		for(smsindex = 0; smsindex < datalen -4; smsindex ++)
		{
      			if('+' == MDM_App_buf[smsindex] && 'C' == MDM_App_buf[smsindex+1 ] && 'M' == MDM_App_buf[smsindex+ 2] && 'T' == MDM_App_buf[smsindex + 3])
			{
      				if((0 == smsindex) || (0x0d == MDM_App_buf[smsindex -1] || 0x0a == MDM_App_buf[smsindex -1]))
				{
      		    			index = smsindex;//��ȡ������Ϣ
      		    			break;
      	  			}
      			}
      		}
  	}
  	if('+' == MDM_App_buf[index] && 'C' == MDM_App_buf[index+1 ] && 'M' == MDM_App_buf[index+ 2] && 'T' == MDM_App_buf[index + 3])
	{//�ж�����
		//���Ŵ���
		    user_debug("i:SMS0:%s",(s8 *)&MDM_App_buf[index]);
		    SVR_SMSdeal(&MDM_App_buf[index]);
		    return 0;
	}
  
	*dataptr = &MDM_App_buf[index];
	//user_debug("MDM-LEN:[%d-%d]", datalen, index);
	datalen = datalen - index;
  //debug_hex("MDM:", *dataptr, datalen);
	//user_debug("MDM[%d]:%s <<",datalen,MDM_App_buf);
	return datalen;
}



void Lobd_start_mode_set(unsigned char state){
	Lobd_start_mode = state;
}

/********************************************************************
 * APP��modemͨѶ
 * modem��app��������ʱapp_main���Ȼ�õ���Ϣ���ڷ���"MR"��Ϣ��app_modem
 * app_modem��������ݽ���
 *                    ����
 *                    ��Ӧ
 * app_mainҲ�ᷢ������������Ϣ��app_modemҪ��app���ĳЩ��������ʽ:Sxx 
 *                                                                  |_ SΪAPP������ָ�� xxΪָ�����
 * app_mainҲ�����OBD����ͨ��app_modem���͵�����������ʽΪ:Oxx
 ********************************************************************/
 extern void TastStartSet_01(u8 flag);
void app_modem(void *data)
{
  	u8 *dataptr;
	EatEvent_st event;
	u32 eventnum;
	u8 u8result,u8t1;
	u8 testflag = 0;
	u16 datalen;
	u8 errornum,gpsonlineenbale;
	u8 ADCWARM,ADCNUM;
	u32 testtime;
	
  

  	ADCWARM = 0x55;//Ĭ�ϱ�����ֹ  2016/3/10 15:07
  	ADCNUM = 0;
  	user_debug("i:app_modem start");
  	Lobd_start_mode = 0;
  	AT_Init(db_svr_addr1get(), db_svr_port1get());
  	svr_event30_enable(0);
  	svr_event01_enable(0);
  	testflag = 0;
  	TastStartSet_01(1);
  	errornum = 0;
  	testtime = 0;
  	gpsonlineenbale = 0;
  	Lpowr_lost_flag = 0;
	while(EAT_TRUE)
	{
		if(m2m_obd_heard >= 5)
		{
			obd_heard();
			m2m_obd_heard_set(0);
		}
		eventnum = eat_get_event_num_for_user(EAT_USER_1);
		if(eventnum || 1 == system_status_get())
		{
		   eat_get_event_for_user(EAT_USER_1, &event);
		   if(event.event == EAT_EVENT_USER_MSG || 1 == system_status_get())
		   {
          		if(1 == system_status_get() || (0x01 == event.data.user_msg.len && 'S' == *(event.data.user_msg.data)))
			{//����˯��
      	      			u8result = 0;
      	      			errornum = 0;
      	   
      	      			for(datalen = 0; datalen < 1800; datalen ++)
				{//���ͻ���������  �����������1800�� ʱ�����Ϊ1Сʱ
	  	       	    if((0x00 == (u8result & 0x01)) && (0 == SVR_Bigmem_send()))
				    {
	  	       	     		bigmem_save();
	  	       	    		u8result |= 0x01;
	  	       	    }
	  	       	    if((0x00 == (u8result & 0x02)) /*&& (anydata_databackout() != 0)*/)
				    {//���ݷ��ͽ���
	  	       	    		errornum ++;//����5�����ݷ���ʧ������Ϊʧ��  ��֤�������е�������ȫ�����͵�ƽ̨
	  	       	    		if(errornum >= 3)u8result |= 0x02;
	  	       	    }
	  	       	    else errornum = 0;
	  	       	    if(0x03 == (u8result & 0x03))break;
	  	            	    eat_sleep(2000);
	  	       	}
		  	       //anydata_EVT_ENGINE_OFF();                                                  //add by lilei-2016-0823
	      	     		AT_CIPSHUT();
	      	     		AT_CMGDA();//˯��ǰɾ�����ж���
			       m2m_statusSet(3);//3-->0
			       gpsonlineenbale = 0;
			       
			       
			       system_subtask_status_set(0x01);
			       user_debug("i:Mode-sleep");
			       eat_sleep_enable(EAT_TRUE);
			       db_gps_cellsave();
			       db_gps_save();
			       
			       while(1)
				{
				        eat_get_event_for_user(EAT_USER_1, &event);
				        if(0x01 == event.data.user_msg.len && 'U' == *(event.data.user_msg.data))break;
			       }
			       gpsonlineenbale = 0;
			       eat_sleep_enable(EAT_FALSE);
			       user_debug("i:Mode-weak up");
			   }
			}
		}
		if(0 == m2m_status())
		{
			MDM_RxBufClr();
			m2m_startcheck();
		}
		if(1 == m2m_status()){
			m2m_information();
		}
		if(2 == m2m_status())
		{
			sim_information();
		}
		if(3 == m2m_status())
		{
			m2m_gprsbond();
		}
		if(4 == m2m_status())
		{
			  u8result = 0;
			  m2m_gprslink();
		}
		if(5 == m2m_status())
		{
			if(0 == Up_Register())
			{
				//user_debug("Register Success\r\n");
				m2m_statusSet(6);
				errornum = 0;
			}
			else
			{
			   	errornum ++;
			   	if(errornum >= 3)
			   	{
			   		m2m_statusSet(4);
					user_debug("-set status :4\r\n");
			   		errornum = 0;
			   	}
		  	}
		}
		if(6 == m2m_status())
		{
			//if(gpsonlineenbale != 0x55)
			//{//2015/6/3 16:06 fangcuisong
			 //   	u8result = gps_assist_online_from_zgex();
			 //   	if(0 == u8result)
			  // 	{
			//	    gpsonlineenbale = 0x55;
			//gps_assist_toOBD();
			   //	}
				  
			//}
			if(0 == UP_Authentication())
			{
				errornum = 0;
				user_debug("Authentication success\r\n");
				m2m_statusSet(7);
			}
			else
			{
			  	errornum ++;
			  	if(errornum >= 3)
				{
			   		m2m_statusSet(4);
			   		errornum = 0;
			   	}
			}
		}
		if(7 == m2m_status())
		{
			m2m_statusSet(8);
			MDM_unlinkset(0x55);		
		}		
		obd_datadeal();//OBD���ݴ���
		gps_tosvr();//GPS���ݷ���
		if(m2m_status() > 6)
		{
			  //add by lilei-2016-0823
			obd_update_keep();
			if(m2m_status() > 7)
			{
				if(0 == MDM_Link_Service_Flag)
				{//��������M2M�Ѿ��Ͽ� ��Ҫ����
					m2m_statusSet(3);
				}
			
			}
			//bigmem_obdgps_tosvr();//���ػ����������� ʱ���Ⱥ�˳��û��ϵ ���������Դ���
			if(0x55 == SVR_EVENT_30)
			{//ÿ30����Ҫ���������
				//SVR_heard();
				svr_event30_enable(0);
			}
		}
		if(mdm.MDMmsgNum != 0)
		{//���ݴ���
			for(u8t1 = 0; u8t1 < MDM_MSG_MAX; u8t1 ++)
			{
			    SVR808_FameDeal();
			    //datalen = MDM_DataToApp(&dataptr);
			    //if(datalen)
			    //{
				 //   u8result = SVR_FameDeal(dataptr, datalen);
				    //if(0x7f == u8result);//��OBD���� ��������
				    //else break;
			    //}
			    //else break;
		  	}
		}
		else eat_sleep(5);//��С˯��ʱ��Ϊ5MS
		
		if(0x55 == SVR_EVENT_01)
		{//ÿ1����Ҫ���������
			ADCNUM ++;					 
	  		if(ADCNUM >= 5 && 0 == APP_UPDATE_FLAGEX)
			{
				m2m_ccidread();
				user_debug("send heart 808\r\n");
				ProFrame_Pack(ProTBuf,UP_HEARTBEAT,&ProPara,ProTempBuf);
	  			if(m2m_status() > 7)
				{				
	  		    		if(0 == Lobd_start_mode)
					{
						
	  		    			back2OBD_2Bytes(0x8e,0x02); 
				    	}
				  
			  	}
	  			if(1 == AT_CADC())
				{//��ѹ�쳣 ��Ҫ����
	  				if(0 == ADCWARM)
					{
						
	  			    		Lpowr_lost_flag = 0x55;
	  			    		ADCWARM = 1;
	  		  		}
					else
					{	

					}
	  			}
	  			else
				{
	  				ADCWARM = 0;
	  			}
	  			ADCNUM = 0;
	  		}
	  		if(1 == ADCWARM && m2m_status() > 6)
			{//���͵�ѹ�쳣������Ϣ
	  			db_obd_save();
	  			obd_cmd8d(1);
	  			ADCWARM = 0x55;//������Ϣֻ����1�� �ٴη�����Ҫ�豸�����������˯���л���
	  		
	  		}
	  	
			if(0 == APP_UPDATE_FLAGEX)
			{
			    if(0 == Lbdata_toserver())
		           {//���ݷ��͵�������
			    	//δ��������
			    		if(m2m_status() > 7)
					{//���ͻ���������
			    	    		SVR_Bigmem_send();
			      		}
			    	} 
		  	}
			svr_event01_enable(0);
	   }
	  AT_SMSinit();//�ýӿ��еĹ��ܱ���ִ�гɹ� ����ѭ��ִ�� ֱ���ɹ�
	  if(m2m_status() >= 8 && 0 == testflag)
	  {//����
			//SVR_test(1);
			//SVR_test(2);
			testflag ++;
	  }
   }
	user_debug("i:app_modem end");
}




/**************************************************************************
*APP��MODEMͨѶ
**********************************************************************************/
/*
 * ������������ȡMODEM���ص����� ���ݱ��浽MDM_Rx_buf��
 * ���������NULL
 * ���������NULL
 * ���ز�����0=������
 *           N=���յ������ݸ���
 * �ⲿ���ã����̼߳�⵽EAT_EVENT_MDM_READY_RDʱ���øýӿ�
 * ע������ýӿڱ����̵߳��ã�MDM_Rx_buf_Temp���ݽ���
*/
static unsigned char MDM_read_debug = 0;
void MDM_read_debugset(void){
	MDM_read_debug = 0x55;
}
u16 MDM_read(void)
{
	u16 len,u16index,u16t1;
	u8 recnum;
	
	if(mdm.MDMmsgNum >= MDM_MSG_MAX)
	{
		user_debug("i:MDM_read:buf-full");
		return 0;
	}
	recnum = 0;
	while(1)
	{
	    memset(MDM_Rx_buf_Temp, 0, EAT_UART_RX_BUF_LEN_MAX);
	    len = eat_modem_read(MDM_Rx_buf_Temp, EAT_UART_RX_BUF_LEN_MAX);
	    if(len != 0)
	    {
	    	//user_debug("M:%s", MDM_Rx_buf_Temp);
	    	//if(0x55 == MDM_read_debug)user_debug("MODEM[%d-%02x,%02x,%02x,%02x,%02x,%02x]",len,MDM_Rx_buf_Temp[0],MDM_Rx_buf_Temp[1],MDM_Rx_buf_Temp[2],MDM_Rx_buf_Temp[3],MDM_Rx_buf_Temp[4],MDM_Rx_buf_Temp[5]);
	    	for(u16t1 = 0; u16t1 < len; u16t1 ++)
		{
	    		if((u16t1 + 6 < len)&& 0x0d == MDM_Rx_buf_Temp[u16t1] && 0x0a == MDM_Rx_buf_Temp[u16t1+1] &&  0x01 == MDM_Rx_buf_Temp[u16t1+2] && 0x00 == MDM_Rx_buf_Temp[u16t1+3] && 0x00 == MDM_Rx_buf_Temp[u16t1+4] && 0x00 == MDM_Rx_buf_Temp[u16t1+5])
			{
	    		   	user_debug("i:===========================");
	    		   	//anydata_CONN_RES();                         //add by lilei-2016-0823
	    		} 
	    	}
	    	//debug_hex("MODEM:", MDM_Rx_buf_Temp, len);
	    	mdm.msg[mdm.MDMmsgIn].MDMmsg_Datalen = len;
	    	mdm.msg[mdm.MDMmsgIn].MDMmsg_Index = MDM_Rx_buf_In;
	    	for(u16index = 0; u16index < len; u16index ++)
		{
	    		MDM_Rx_buf[MDM_Rx_buf_In] = MDM_Rx_buf_Temp[u16index];
	    		MDM_Rx_buf_In ++;
	    		if(MDM_Rx_buf_In >= EAT_UART_MDM_RX_BUF_LEN_MAX)MDM_Rx_buf_In = 0;
	    	}
	    	mdm.MDMmsgIn ++;
	    	if(mdm.MDMmsgIn >= MDM_MSG_MAX)mdm.MDMmsgIn = 0;
	    	
	    	
	    	if(mdm.MDMmsgNum < MDM_MSG_MAX)
		{
	    		if(0x55 == mdm_msgread_flag)eat_sleep(1);
	    		mdm_msgread_flag = 0x55;
	    		mdm.MDMmsgNum ++;
	    		mdm_msgread_flag = 0x00;
	    	}
	    	
	    }
	    else break;
	    eat_sleep(5);
	    recnum ++;
	    if(recnum > 16)break;//����������յ�8K������ֱ�ӷ���
      }
	return len;
}


/*
 * �����������������ݵ�MODEM
 * ���������data = ��Ҫ���͵�����
             datalen= ���͵����ݸ��� ������512
 * ���������NULL
 * ���ز�����0=����ʧ��
 *           N=���͵����ݸ���
 * �ⲿ���ã���Ҫʱ����
 * ע������ýӿڻ��Զ��ڷ������ݽ�β+0x0d 0x0a ���Ӧ�ó�����Ҫ���Ӹ��ַ�
*/
u16 MDM_write(u8 *data, u16 datalen){
	u16 overtime;
	
	if(NULL == data || datalen >= (EAT_UART_TX_BUF_LEN_MAX - 8))return 0;
	//user_infor("MDM_write>>%s", data);
	//if(0x55 == MDM_read_debug)user_debug("MDM_write[%d-%02x,%02x,%02x,%02x]<<",datalen,*(data+3),*(data+4),*(data+5),*(data+6));
	if(MDM_Tx_buf_Num != 0 || 0x55 == MDM_Tx_buf_flag){
		while(1){
			eat_sleep(5);
			if(0 == MDM_Tx_buf_Num && MDM_Tx_buf_flag != 0x55)break;
			overtime ++;
			if(overtime > 200)return 0;
		}
	}
	
	MDM_Tx_buf_flag = 0x55;
	MDM_Tx_buf_Num = datalen;
	memset(MDM_Tx_buf, 0, EAT_UART_TX_BUF_LEN_MAX);
	memcpy(MDM_Tx_buf, data, datalen);
	MDM_Tx_buf[MDM_Tx_buf_Num ++] = 0x0d;
	MDM_Tx_buf[MDM_Tx_buf_Num ++] = 0x0a;
	MDM_Tx_buf_flag = 0x00;
	
	return datalen;
}
/*����0x0d,0x0a
*********************************************/
u16 MDM_write1(u8 *data, u16 datalen){
	u16 overtime;
	
	if(NULL == data || datalen > EAT_UART_TX_BUF_LEN_MAX)return 0;
	if(MDM_Tx_buf_Num != 0 || 0x55 == MDM_Tx_buf_flag){
		while(1){
			eat_sleep(5);
			if(0 == MDM_Tx_buf_Num && MDM_Tx_buf_flag != 0x55)break;
			overtime ++;
			if(overtime > 200)return 0;
		}
	}
	//debug_hex("MDM_write1>>", data, datalen);
	if(0x55 == MDM_read_debug)user_debug("i:MDM_write1[%d-%02x,%02x,%02x,%02x]<<",datalen,*(data+3),*(data+4),*(data+5),*(data+6));
	MDM_Tx_buf_flag = 0x55;
	memset(MDM_Tx_buf, 0, EAT_UART_TX_BUF_LEN_MAX);
	MDM_Tx_buf_Num = datalen;
	memcpy(MDM_Tx_buf, data, datalen);
	MDM_Tx_buf_flag = 0x00;
	return datalen;
}
/*
*/
u8 MDM_writeEx(void){
	u16 len;
	
	if(MDM_Tx_buf_Num >= EAT_UART_TX_BUF_LEN_MAX)
	{
		MDM_Tx_buf_Num = 0;
		return 0;
	}
	

	if(MDM_Tx_buf_Num && 0x00 == LMDM_SEND_ENABLE)
	{
		debug_hex("MDM_writeEx>>",MDM_Tx_buf,MDM_Tx_buf_Num);
		
			
		len = eat_modem_write(MDM_Tx_buf, MDM_Tx_buf_Num);
		if(len < MDM_Tx_buf_Num)
		{//���ͻ�����
			user_debug("i:MDM_Tx_Buff full");
			LMDM_SEND_ENABLE = 0x55;
		}
		else
		{
		 	if(0x55 == MDM_Tx_buf_flag)eat_sleep(1);
		 	MDM_Tx_buf_flag = 0x55;
			MDM_Tx_buf_Num = 0;
		 	MDM_Tx_buf_flag = 0x00;
		}
		
	}
	return MDM_Tx_buf_Num;
}

u8 MDM_writeEnable(void){
	user_infor("e:MDM_writeEnable>>");
	LMDM_SEND_ENABLE = 0;
	return 0;
}

