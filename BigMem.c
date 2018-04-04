/*
*
*������һ������ڴ����򣬸�������������APPʹ�ã�
*��ͬʱֻ����һ��Ӧ����ʹ��
*ʹ�������Ҫ�ͷ�
*
*ʹ�ø÷�ʽ��Ҫ������eat_malloc���ܴ�������ʧ�ܵ����
*
*��ǰ��Ҫ�������¼����ط�
*1��OBD���ݻ�����
*2���������ݻ���
*ͬһʱ��ֻ�ܴ洢һ������
*OBD���ݻ���
*     OBD���ݻ������������ BIG_MEM_MAXΪ1�����棬���Լ�������������ϵͳ����˯��ǰ��Ҫ��1���������ݱ��浽2������
*     FSΪ2�����棬��СΪ1.5M
****************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "definedata.h"
#include "UartTask.h"
#include "AppUser1.h"
#include "AppUser2.h"
#include "OBDapi.h"
#include "db.h"
#include "ATapi.h"
#include "SVRapi.h"
#include "BigMem.h"


static u8 Lbigmemory[BIG_MEM_MAX];
static u8 LbigmemFlag = 0;/*
                            1 = ��ǰΪOBD���ݻ���
                            2 = ��ǰΪ�������ݻ���
                          */
u8 Lbigmemory_send[EAT_RTX_BUF_LEN_MAX];//���ڴ����ݷ��ͻ�����
static _BIGMEM Lbigmem;

void bigmem_init(void)
{
	LbigmemFlag = 0;
	Lbigmem.dataindex = 0;
	Lbigmem.datalen = 0;
	Lbigmem.msgin = 0;
	Lbigmem.msgout = 0;
	Lbigmem.msgnum = 0;
}
/*
*�ڴ�ʹ�����룬��Ҫ����typeΪ��������
*type = 2:��������
*       1:OBD���ݣ���GPS��
*�����ͬһ�����ݿ����ظ�������ڴ棬�����ݻ��0��ַ��ʼ���
********************************************/
u8 *bigmem_get(u8 type){
	if(1 == type){
		if(2 == LbigmemFlag)return NULL;
	}
	if(2 == type){
		if(1 == LbigmemFlag)return NULL;
	}
	else return NULL;
	LbigmemFlag = type;
	return Lbigmemory;
}

void bigmem_free(void){
	LbigmemFlag = 0;
}

/*��������1�������е����ݱ��浽2��������
*/
u8 bigmem_save(void){
	  if(Lbigmem.dataindex < 5)return 1;
	  user_debug("lilei-sleep and save bigbufdata\r\n");
	  db_filesave(Lbigmemory, Lbigmem.dataindex, 0);
		Lbigmem.dataindex = 0;
		Lbigmem.datalen = 0;
	  Lbigmem.msgin = 0;
	  Lbigmem.msgout = 0;
	  Lbigmem.msgnum = 0;
	  return 0;
}
/*OBD+GPS���ݱ��浽1����������
*1����������������������
*   1��Lbigmemory�е�������
*   2��_BIGMEM����֡��
*****************************************/
u8 bigmem_obdgps_in(u8 *datain, u16 datalen)
{
	u16 datainindex;
  
  //user_infor("bigmem_obdgps_in:[%d-%d]", datalen, LbigmemFlag);
	if(NULL == datain || 0 == datalen || datalen >= 512)return 1;//ÿ֡���ݳ��Ȳ�����512���ֽ�
	if(2 == LbigmemFlag)return 2;//ʧ��
	if(0 == LbigmemFlag)
	{
		user_debug("lilei--first fill LBigBUf\r\n");                                 //---lilei---add by lilei-2016-0613
		LbigmemFlag = 1;											//big_mem�����ڱ���OBD+gps����
		Lbigmem.dataindex = 0;
		Lbigmem.datalen = 0;
	  	Lbigmem.msgin = 0;
	  	Lbigmem.msgout = 0;
	  	Lbigmem.msgnum = 0;
	}
	if(Lbigmem.datalen + datalen >= (DATA_LOG_FILE_MAX - 512))
	{//�������� ��Ҫ�����ݱ��浽2����������
		//���ݱ��浽2��������
		user_debug("lilei-BigMem data cnt >1024*100 save file\r\n");         //add by lilei--2016-0613
		db_filesave(Lbigmemory, Lbigmem.dataindex, 0);
		Lbigmem.dataindex = 0;
		Lbigmem.datalen = 0;
	  	Lbigmem.msgin = 0;
	  	Lbigmem.msgout = 0;
	  	Lbigmem.msgnum = 0;
	}
	Lbigmem.msg[Lbigmem.msgin].index = Lbigmem.dataindex;
	Lbigmem.msg[Lbigmem.msgin].len = datalen;
	memcpy((s8 *)&Lbigmemory[Lbigmem.dataindex], datain, datalen);
	Lbigmem.dataindex += datalen;
	if(Lbigmem.dataindex >= DATA_LOG_FILE_MAX)Lbigmem.dataindex = 0;
	Lbigmem.datalen += datalen;
	Lbigmem.msgin ++;
	if(Lbigmem.msgin >= BIG_MEM_MSG_MAX)Lbigmem.msgin = 0;
	Lbigmem.msgnum ++;
	if(Lbigmem.msgnum >= BIG_MEM_MSG_MAX)
	{//������������
		//���ݱ��浽2��������
		user_debug("lilei-BigMem stareamer>1280 save file\r\n");					//add by lilei--2016-0613
		db_filesave(Lbigmemory, Lbigmem.dataindex, 0);
		Lbigmem.dataindex = 0;
		Lbigmem.datalen = 0;
	  	Lbigmem.msgin = 0;
	  	Lbigmem.msgout = 0;
	  	Lbigmem.msgnum = 0;
	}
	user_debug("i:bigmem_obdgps_in:[%d,%d,%d,%d,%d]",Lbigmem.msgnum,Lbigmem.dataindex,Lbigmem.datalen,Lbigmem.msgin,Lbigmem.msgout);
	return 0;
}

/*�Ӷ��������ж�ȡ����
*
***************************************************/
u8 bugmem_obdgps_readfromfile(void)
{
	u32 filesize,fileindex;
	
	if(2 == LbigmemFlag)return 0;//ʧ��
	if(0 == LbigmemFlag)
	{
		LbigmemFlag = 1;//big_mem�����ڱ���OBD+gps����
		Lbigmem.dataindex = 0;
		Lbigmem.datalen = 0;
	  	Lbigmem.msgin = 0;
	  	Lbigmem.msgout = 0;
	  	Lbigmem.msgnum = 0;
	}
	filesize = db_fileread(Lbigmemory);
	if(0 == filesize)bigmem_free();
	else
	{
		if(0xa5 == Lbigmemory[0] && 0xa5 == Lbigmemory[1])
		{
			  //�����ݽ��и�ʽ��
			 Lbigmem.msgin = 0;
			 Lbigmem.msgout = 0;
			 Lbigmem.msg[Lbigmem.msgin].index = 0;
			 Lbigmem.msgnum = 0;
			 for(fileindex = 2; fileindex < filesize; fileindex ++)
			 {
			   	if(0xa5 == Lbigmemory[fileindex] && 0xa5 == Lbigmemory[fileindex +1])
				{
			   		Lbigmem.msg[Lbigmem.msgin].len =  fileindex - Lbigmem.msg[Lbigmem.msgin].index;
			   		Lbigmem.msgin ++;
			   		Lbigmem.msgnum ++;
			   		if(Lbigmem.msgnum >= BIG_MEM_MSG_MAX)return 0;
			   		Lbigmem.msg[Lbigmem.msgin].index = fileindex;
			   		if(Lbigmem.msgin >= BIG_MEM_MSG_MAX)break;
			   	 }
			   }
			 
			   Lbigmem.datalen = filesize;
			   Lbigmem.dataindex = fileindex;
			   user_debug("lilei --read db file msgnum=%d,datalen=%u,dataindex=%u\r\n",Lbigmem.msgnum, Lbigmem.datalen,Lbigmem.dataindex );
			   return Lbigmem.msgnum;
		}
		else
		{
			bigmem_free();
		}
	}
	return 0;
}

/*��1���������ж�ȡ���ݣ�ͨ����APPuser1�����ݴ�1���������ж�ȡ���ڷ���
*
********************************************/
u16 bigmem_obdgps_out(u8 *dataout)
{
	u16 datalen,msgoutlog, msgnumlog,datalenlog;
	
	if(NULL == dataout)return 0;
	if(1 == LbigmemFlag)
	{
		if(0 == Lbigmem.msgnum)
		{
			bigmem_free();
			return 0;
		}
		
		msgoutlog = Lbigmem.msgout;
		msgnumlog = Lbigmem.msgnum;
		datalenlog = Lbigmem.datalen;
		datalen = 0;
		while(1)
		{//��� ��һ�����ݲ�����512���ֽ�
			if(Lbigmem.msg[Lbigmem.msgout].len >= EAT_RTX_BUF_LEN_MAX)
			{
				user_debug("lilei--send LbigBuf len>512\r\n");	
		  	}
		  	else
			{
		  		if(datalen + Lbigmem.msg[Lbigmem.msgout].len < EAT_RTX_BUF_LEN_MAX)
				{
					user_debug("lilei--send LbigBuf-datalen=%d\r\n",Lbigmem.msg[Lbigmem.msgout].len);								//add by lilei-2016-0613
		  			memcpy((s8 *)&Lbigmemory_send[datalen], (s8 *)&Lbigmemory[Lbigmem.msg[Lbigmem.msgout].index], Lbigmem.msg[Lbigmem.msgout].len);
		  			datalen += Lbigmem.msg[Lbigmem.msgout].len;
		  		}
		  		else break;
		  	}
		  	if(Lbigmem.datalen >= Lbigmem.msg[Lbigmem.msgout].len)Lbigmem.datalen -= Lbigmem.msg[Lbigmem.msgout].len;
		  	else
			{//�����쳣
		  		Lbigmem.dataindex = 0;
		    		Lbigmem.datalen = 0;
	      			Lbigmem.msgin = 0;
	      			Lbigmem.msgout = 0;
	      			Lbigmem.msgnum = 0;
		  		return 0;
		  	}
		  	Lbigmem.msgout ++;
			if(Lbigmem.msgout >= BIG_MEM_MSG_MAX)Lbigmem.msgout = 0;
			Lbigmem.msgnum --;
			if(0 == Lbigmem.msgnum)
			{
		      		bigmem_free();
		      		break;
		  	}
		}
		user_debug("i:bigmem_obdgps_out:[%d,%d,%d,%d]",Lbigmem.msgnum,Lbigmem.datalen,Lbigmem.msgout,datalen);
		if(0 == datalen)return 0;
		if(0 == AT_CIPSEND(Lbigmemory_send, datalen))
		{
			return datalen;	
		}
		else
		{//���ݷ���ʧ�� 
			Lbigmem.msgout = msgoutlog;
			Lbigmem.msgnum = msgnumlog;
			Lbigmem.datalen = datalenlog;
			return 0;//����ʧ��
		}	
	}
	else
	{
	   	if(db_filecheck())
		{
	   		if(0 == bugmem_obdgps_readfromfile())
			{
	   			user_debug("i:bigmem_obdgps_out1:[%d,%d,%d]",Lbigmem.msgnum,Lbigmem.msgin,Lbigmem.datalen);
	   			return 0;
	    		}
	    		user_debug("i:bigmem_obdgps_out2:[%d,%d,%d]",Lbigmem.msgnum,Lbigmem.msgin,Lbigmem.datalen);
	   		return 1;
	  	}
	   	return 0;
  	}
}

/*����ֱ�ӷ��͵�������
*
*
*******************************************/
u8 bigmem_obdgps_tosvr(void)
{
	u16 datalen;
	
	if(1 == LbigmemFlag)
	{
		if(0 == Lbigmem.msgnum || Lbigmem.datalen < Lbigmem.msg[Lbigmem.msgout].len)
		{
			bigmem_free();
			return 0;
		}
		user_infor("e:bigmem_obdgps_tosvr:[%d-%d]", Lbigmem.msgnum, Lbigmem.msgout);
		AT_CIPSEND(&Lbigmemory[Lbigmem.msg[Lbigmem.msgout].index], Lbigmem.msg[Lbigmem.msgout].len);
		Lbigmem.msgout ++;
		if(Lbigmem.msgout >= BIG_MEM_MSG_MAX)Lbigmem.msgout = 0;
		Lbigmem.msgnum --;
		Lbigmem.datalen -= Lbigmem.msg[Lbigmem.msgout].len;
		if(0 == Lbigmem.msgnum)
		{
			bigmem_free();
		}
		return datalen;
	}
	else return 0;
}


