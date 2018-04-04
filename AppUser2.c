/*
*������
**********************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <rt_misc.h>

#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "definedata.h"
#include "UartTask.h"
#include "ATapi.h"
#include "AppUser1.h"
#include "AppUser2.h"
#include "SVRapi.h"

#define MOBILE_INFOR_UPDATE_TIME 60 //��վ���ݸ���ʱ���� 
static u32 Ltime2_ticket = 0;
static u8 Lserver_keap_step = 0;
static u32 Lserver_Link_time;//��¼���һ���豸���յ����������������� �������80sδ���յ���������������Ϊ�豸��������Ѿ��Ͽ�
static _stime Lstime = {0};
static u8 app_svrlinkflag = 0;
static u32 app_sleep_time = 0;
static u8 app_sleep_flag = 0;
void app_sleep_flag_set(u8 flag){
	if(0x55 == app_sleep_flag)eat_sleep(1);
	app_sleep_flag = 0x55;
	if(flag)app_sleep_time ++;
	else app_sleep_time = 0;
	app_sleep_flag = 0;
}
void app_svrlink(u8 state){
	if(0x55 == app_svrlinkflag)eat_sleep(1);
	app_svrlinkflag = 0x55;
	if(0 == state)Lserver_Link_time = state;
	else Lserver_Link_time ++;
	app_svrlinkflag = 0x00;
}

static u8 app_keep_step_setflag = 0;
void app_keep_step_set(u8 state){
	if(0x55 == app_keep_step_setflag)eat_sleep(1);
	app_keep_step_setflag = 0x55;
	if(0 == state)Lserver_keap_step = state;
	else Lserver_keap_step ++;
	app_keep_step_setflag = 0;
}


void Lstime_init(void)
{
 
 	 Lstime.time_rtc = 0;
 	 Lstime.time_rtc_update = 0;
	 Lstime.time_gps = 0;
	 Lstime.time_gps_update = 0;
	 Lstime.time_app = 0;
	 Lstime.time_app_update = 0;
}


void Lstime_app_init(u32 time)
{
	 if(0 == Lstime.time_gps_update)
	 {
    		Lstime.time_app = time;
    		Lstime.time_app_update = user_time() / 1000;  
  	}
}

void Lstime_app_update(void)
{
 	if(Lstime.time_gps_update != 0)
	{
    		Lstime.time_app ++;
    		Lstime.time_app_update = user_time() / 1000;
  	}
}

void Lstime_sys_update(u32 time)
{
 	Lstime_app_init(time);
 	Lstime.time_rtc = time;
 	Lstime.time_rtc_update = user_time() / 1000;
}

void Lstime_gps_update(u32 time)
{
 	Lstime_app_init(time);
 	Lstime.time_gps = time;
 	Lstime.time_gps_update = user_time() / 1000;
}

u32 Lstime_get(void)
{
 	u32 time,time1,time2,time3;
 	u8 index;
 
 	if(0 == Lstime.time_rtc_update && 0 == Lstime.time_app_update && 0 == Lstime.time_gps_update)return 0;
 	time = user_time() / 1000;
 	if(time >= Lstime.time_gps_update)time1 = time - Lstime.time_gps_update;
 	else time1 = 61;
 	if(time >= Lstime.time_rtc_update)time2 = time - Lstime.time_rtc_update;
	 else time2 = 61;
	 if(time >= Lstime.time_app_update)time3 = time - Lstime.time_app_update;
	 else time3 = 61;
	 if(time1 < 4)return Lstime.time_gps;
	 if(time2 < 4)return Lstime.time_rtc;
	 if(time3 < 4)return Lstime.time_app;
	 if(time1 > 60 && time2 > 60 && time3 > 60)
	 {
	 	//user_debug("TTTTTTTTTTTTTTTTTTTTTTTTTTTT");
	 	return 0;//1������ʱ��û�и�������Ϊ�쳣
 	}
 	index = 0;
 	if(time1 < time2)
	{
  		if(time1 < time3)index = 1;
  		else index = 3;
 	}
 	else
	{
  		if(time2 < time3)index = 2;
  		else index = 3;
 	}
 	if(1 == index)return  Lstime.time_gps;
 	else if(2 == index)return  Lstime.time_rtc;
 	else if(3 == index)return  Lstime.time_app;
 
 	return 0;
}


void linking_init(void){
	app_keep_step_set(0);
	app_svrlink(0);
	Ltime2_ticket = 0;
	app_sleep_flag = 0;
	Lstime_init();
}

extern void TastStartSet_03(u8 flag);
void app_linking(void *data){
	EatEvent_st event;
	u8 mobileinforupdate;//��վ���ݸ������� ÿ30S��Ҫ����һ�λ�վ����
	

	app_keep_step_set(0);
  	TastStartSet_03(1);
	eat_sleep(10000);//˯��10s
	mobileinforupdate = MOBILE_INFOR_UPDATE_TIME;//��һ��Ĭ����Ҫ����
	eat_sleep_enable(EAT_TRUE);
	while(EAT_TRUE){
		eat_get_event_for_user(EAT_USER_3, &event);
		if(event.event == EAT_EVENT_USER_MSG)
		{
			  Ltime2_ticket ++;
			  Lstime_app_update();
			  app_sleep_flag_set(1);
			  if(app_sleep_time > 90)
			 {//90S��û�н��յ���Ч��GPS��OBD��������Ϊ�쳣
			  	bigmem_save();//�豸�����ϱ�OBD�����ϵ� ��Ҫ����������1�������е�����
	        		db_gps_save();
			  	app_sleep_time = 0;
			  	system_status_set(1);
//			  	eat_sleep_enable(EAT_TRUE);
			  	continue;
			  }
			  svr_event01_enable(0x55);//ÿ1S������Ϣ
			  if(0 == Ltime2_ticket % 2)
			  {//ÿ2S��������Ϣ
			  	m2m_obd_heard_set(1);
			  }
		    	  if(m2m_status() > 6)
			  {
		    		app_keep_step_set(1);
		    		app_svrlink(1);
		    	  }
		    	  if(mobileinforupdate < MOBILE_INFOR_UPDATE_TIME)mobileinforupdate ++;
		    	  if(m2m_status() > 7 && mobileinforupdate >= MOBILE_INFOR_UPDATE_TIME)
			  {//���»�վ����
		    	  	if(0 == svr_event_mobileinfor_update(0x55))mobileinforupdate = 0;
		    	  }
		    if(Lserver_keap_step >= 30){
		    	svr_event30_enable(0x55);//��ֹ�������̷��ͱ���
		    }
		    if(Lserver_Link_time > 65)
		    {
		    		user_debug("i:M2M breakout with service");
		    		MDM_unlinkset(0);
		    		Lserver_Link_time = 0;
		    }
		}
	}
}

