/*
*
*GPS数据处理
*************************************************************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <rt_misc.h>

#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"
#include "definedata.h"
#include "UartTask.h"
#include "AppUser1.h"
#include "OBDapi.h"
#include "ATapi.h"
#include "gps.h"
//#include "anydata.h"

#define GPS_UNIT_MAX 1024
#define MOBILE_INFOR_MAX 512
static u8 GPS_UNIT_BUF[GPS_UNIT_MAX + 1];
static u8 GPS_UNIT_BUF_temp[GPS_UNIT_MAX + 1]; //GPS_UNIT_BUF
static u32 GPS_UNIT_temp_LEN = 0;
static u8 MOBILE_INFOR_BUF[MOBILE_INFOR_MAX + 1];
static u32 GPS_UNIT_LEN = 0;
 _GPSUNIT Lgpscur;//记录当前GPS参数
static _GPSUNIT Lgpscursend;//记录当前需要发送的数据
static _GPS Lgps;
static u8 Lgps_enable_flag = 0;//GPS数据有效标志
static _GPSM Lgpsm;//GPS坐标转角度hh:mm:ss
extern u8 LMDM_SEND_ENABLE;
extern u8 LGPS_SYSTEM_TIME_SYNCHRON;
static u8 LGPS_FRAME_SEND_TIME = 0;//GPS数据上传等待次数 以行驶相对方向角来判断
                                   /*
                                   0(静止) = 60(1分钟1次)
                                   偏差<1  = 10(10秒1次)
                                   偏差1-2 = 5(5秒1次)
                                   偏差2-4 = 2(2秒1次）
                                   偏差>4 = 1(1秒1次)
                                   */
static u8 LGPS_DATA_ERROR_NUM = 0;//GPS数据异常次数
static u8 LGPS_TIME_SYN = 0;//GPS数据有效后采用GPS时间对系统时间进行一次校准

static u8 GPS_UNIT_LENflag = 0;
static u8 GPS_SEND_NUMflag = 0;
static u8 LGPS_FRAME_SEND_TIME_EX = 0;//GPS数据强制上传 0x55=上传2014/12/11 10:01
static u32 GPS_LAST_FRAME_TIME = 0;//最后一帧GPS数据的时间，如果该时间与当前时间比差了4个小时则需要重新获取星历来实现快速定位
static double GPS_LAST_LAC = 0;//最后一帧GPS坐标
static double GPS_LAST_LON = 0;
static u32 GPS_Speed = 0;
static u8 Lgps_status = 0;//GPS状态 0 = GPS未启动 1=GPS未定位 2=GPS已经定位
/*本次行驶油耗L
* 本次行驶里程Km
* 更新时间 超过5秒认为无效
*/
static _OBDGPS Lobdgps;/*
static u32 Ldistance_obd;
static u32 Lfuel_obd;*/
static u32 Lupdate_time;
/*GSM位置信息
* cellid
*lacid
*更新时间  超过2分钟认为无效
*/
static u32 Lcellid;
static u32 Llacid;
static u32 Lgsm_position_updatetime;

static u8 gps_ubx_online[3072];//最多不超过3K数据
static u32 gps_ubx_datalen = 0;
static u32 gps_ubx_update_time = 0;//ubx数据获取的时间
static u32 gps_3g_check = 0;//三急检测数据
static u8 gps_vehicle_status = 0;/*车辆状态，通过当前油耗类型设置车辆状态
                                  0=未知
                                  1=怠速
                                  2=行车  车辆状态的改变GPS数据必须上传*/

u32 gps_time_get(void){
	return GPS_LAST_FRAME_TIME;
}
void gps_vehicle_status_set(u8 flag){
	if(gps_vehicle_status != flag)LGPS_FRAME_SEND_TIME_EX = 0x55;
	gps_vehicle_status = flag;
}

void gps_3g_check_set(u8 flag){
	if(1 == flag){
		gps_3g_check = 0x01;
		LGPS_FRAME_SEND_TIME_EX = 0x55;
	}
	else if(2 == flag){
		gps_3g_check = 0x02;
		LGPS_FRAME_SEND_TIME_EX = 0x55;
	}
	else if(4 == flag){
		gps_3g_check = 0x04;
		LGPS_FRAME_SEND_TIME_EX = 0x55;
	}
}
/*返回GPS状态
*/
u8 gps_status_get(void)
{
	return Lgps_status;
}
/*获取GPS速度
*/
u32 gps_speed_get(void){
	return GPS_Speed;
}

/*WGS-84坐标转度
*ddmm.mmmmm ---> dd.dddd
*/
u8 gps_wgs84_degrees(double latin, double lonin, double *latout, double *lonout){
	u32 u32t1;
	double d1;
	
	u32t1 = latin / 100;
	d1 = ((latin / 100.0 - u32t1) * 100) / 60.0;
	*latout = u32t1 + d1;
	
	u32t1 = lonin / 100;
	d1 = ((lonin / 100.0 - u32t1) * 100) / 60.0;
	*lonout = u32t1 + d1;
	
	return 0;
}


u32 gps_data_read(u8 *data, u32 datalen){
	if(NULL == data || datalen >= GPS_UNIT_MAX)return 0;
	memset((s8 *)GPS_UNIT_BUF, 0, GPS_UNIT_MAX);
	memcpy((s8 *)GPS_UNIT_BUF, (s8 *)data, datalen);
	GPS_UNIT_LEN = datalen;
	return datalen;
}

void gps_obdinsert(u8 id, u32 data){
	u8 u8index;
	
	for(u8index = 0; u8index < Lobdgps.num; u8index ++)
	{
		if(id == Lobdgps.id[u8index])
		{
			Lobdgps.data[u8index] = data;
			return;
		}
	}
	Lobdgps.id[Lobdgps.num] = id;
	Lobdgps.data[Lobdgps.num] = data;
	if(Lobdgps.num < 16)Lobdgps.num ++;
}

u8 gps_gsmpositionset(u32 cellid, u32 lacid){
	Lcellid = cellid;
	Llacid = lacid;
	Lgsm_position_updatetime = user_time();                                                               //lilei-?-2016-10-18-not use-in functin
	if(0 == Lgps_enable_flag){//GPS无效 需要产生一组GPS数据 当GPS数据有效时只保存基站数据 待需要时供软件提取
		gps_data_transformEx();
	}
	return 0;
}
u8 gps_mobileinfor_deal(void){
	u32 index, datalen,index1;
	u32 cellid, lacid;
	u8 *res;
	
	if(0 == MOBILE_INFOR_BUF[0])return 1;
	res = MOBILE_INFOR_BUF;
	datalen = strlen(MOBILE_INFOR_BUF);
	for(index = 0; index < datalen; index ++){
			if('"' == *(res + index)){//提取基站信息
				index1 = 0;
				for(; index < datalen; index ++){
					if(',' == *(res + index)){
						index1 ++;
						if(6 == index1){//提取cell id
							cellid = 0;
							index ++;
							for(; index < datalen; index ++){
								if(',' == *(res + index)){
									index1 ++;
									break;
								}
								else if(0x0a == *(res + index) || 0x0d == *(res + index)){
									MOBILE_INFOR_BUF[0] = 0;
									return 1;
								}
								if(*(res + index) >= 'a' && *(res + index) <= 'f')cellid = (cellid << 4) + *(res + index) - 'a' + 0x0a;
								else if(*(res + index) >= 'A' && *(res + index) <= 'F')cellid = (cellid << 4) + *(res + index) - 'A' + 0x0a;
								else if(*(res + index) >= '0' && *(res + index) <= '9')cellid = (cellid << 4) + *(res + index) - '0';
								else{
								 MOBILE_INFOR_BUF[0] = 0;
								 return 2;
								}
							}
						}
						if(7 == index1){//提取lac id
							lacid = 0;
							index ++;
							for(; index < datalen; index ++){
								if(',' == *(res + index)){
									gps_gsmpositionset(cellid, lacid);
									MOBILE_INFOR_BUF[0] = 0;
									return 0;
								}
								else if(0x0a == *(res + index) || 0x0d == *(res + index)){
									MOBILE_INFOR_BUF[0] = 0;
									return 1;
								}
								if(*(res + index) >= 'a' && *(res + index) <= 'f')lacid = (lacid << 4) + *(res + index) - 'a' + 0x0a;
								else if(*(res + index) >= 'A' && *(res + index) <= 'F')lacid = (lacid << 4) + *(res + index) - 'A' + 0x0a;
								else if(*(res + index) >= '0' && *(res + index) <= '9')lacid = (lacid << 4) + *(res + index) - '0';
								else{
								 MOBILE_INFOR_BUF[0] = 0;
								 return 2;
								}
							}
						}
					}
					if(0x0a == *(res + index) || 0x0d == *(res + index)){
						MOBILE_INFOR_BUF[0] = 0;
						return 1;
					}
				}
				if(index >= datalen){
					MOBILE_INFOR_BUF[0] = 0;
					return 1;
				}
			}
			else if(0x0a == *(res + index) || 0x0d == *(res + index)){
				MOBILE_INFOR_BUF[0] = 0;
				return 1;
			}
		}
		user_infor(res);
		MOBILE_INFOR_BUF[0] = 0;
		return 1;
}
u8 gps_mobileinfor(u8 *data){
	if(MOBILE_INFOR_BUF[0] != 0)return 0;
	if(NULL == data || strlen(data) >= MOBILE_INFOR_MAX)return 0;
	strcpy((s8 *)MOBILE_INFOR_BUF, (s8 *)data);
	return strlen(MOBILE_INFOR_BUF);
}
u8 gps_gsmpositionget(u32 *cellid, u32 *lacid){
	u32 curtime;
	
	curtime = user_time();
	if(Lupdate_time > curtime)return 1;
	if(Lupdate_time + 120 < curtime)return 1;
	*cellid = Lcellid;									//lilie-?-2016-10-18- Lcellid-Llacid is not useful-in need
	*lacid = Llacid;
	return 0;
}

/*
*只通过基站数据进行定位
****************************************************/
u8 gps_data_transformEx(void){
	u8 u8result;
	u32 speed,u32t1,u32t2;
	
	return 0;//2015/7/15 13:19 fangcuisong 测试中发现anydata平台轨迹数据会出现异常情况，该异常判定为时间戳导致 暂时关闭该功能
	u8result = obd_vehiclespeedget(&speed);
	Lgps.unit[Lgps.in].latitude = 0;
	Lgps.unit[Lgps.in].latflag = 0;
	Lgps.unit[Lgps.in].longitude = 0;
	Lgps.unit[Lgps.in].longflag = 0;
	Lgps.unit[Lgps.in].speed = speed;
	Lgps.unit[Lgps.in].angle = 0;
	Lgps.unit[Lgps.in].ymd = 0;
	Lgps.unit[Lgps.in].time = 0;
	Lgps.unit[Lgps.in].timeflag = G_system_time_getEx();//获取时间戳
	u32t1 = 0;
	u32t2 = 0;
	//u8result = gps_obddataget(&u32t1, &u32t2);
	Lgps.unit[Lgps.in].distance = u32t1;
	Lgps.unit[Lgps.in].fuel = u32t2;
	u32t1 = 0;
	u32t2 = 0;
	gps_gsmpositionget(&u32t1, &u32t2);
	Lgps.unit[Lgps.in].celid = u32t1;
	Lgps.unit[Lgps.in].lacid = u32t2;
	
	Lgps.unit[Lgps.in].itemflag = 0x3f;
	Lgps.in ++;
	if(Lgps.in >= GPS_DATA_MAX)Lgps.in = 0;
	if(Lgps.num <= GPS_DATA_MAX){
		if(0x55 == GPS_SEND_NUMflag)eat_sleep(1);
		GPS_SEND_NUMflag = 0x55;
		Lgps.num ++;
		GPS_SEND_NUMflag = 0x00;
	}
	else{
			user_debug("i:GPS1-BUF full");
	}
	user_infor("e:GPS1-TO-SVR");
}

u8 gps_data_get(u32 *lat, u32 *lon, u32 *speed, u32 *angle){
	u32 u32t1;
	double d1;
	//需要将ddmm.mmmm ---> dd.dddd --> dddddd
	u32t1 = GPS_LAST_LAC / 100;
	d1 = ((GPS_LAST_LAC / 100 - u32t1) * 100) / 60.0;
	*lat = (u32t1 + d1) * 1000000;
	u32t1 = GPS_LAST_LON / 100;
	d1 = ((GPS_LAST_LON / 100 - u32t1) * 100) / 60.0;
	*lon = (u32t1 + d1) * 1000000;
	if(Lgps_status != 2){
	    *speed = Lgpscur.speed;
	    *angle = Lgpscur.angle;
  }
  else{
  	*speed = 0;
  	*angle = 0;
  }
	return 1;
}

/*GPS坐标数据转到角数据
*/
u8 gps_data_transform(void)
{
	double dd;
	u8 lahh,lamm,lohh,lomm,u8flag;
	double lass,loss;
	u8 u8result;
	u32 speed,u32t1,angle,u32t2;
	double lat1,lat2,long1,long2,latex,longex;
	u32 systemtime;
	
	if(Lgpscur.itemflag < 0x3f)return 1;
	db_gps_set(Lgpscur.latitude, Lgpscur.longitude);
	Lgps_status = 2;
	systemtime = ydmhms2u32(Lgpscur.ymd, Lgpscur.time);//G_system_time_getEx();//获取时间戳
	if(0 == systemtime)return 2;
	u32t1 = G_system_time_getEx();
	if((u32t1 + 60) < systemtime || u32t1 > (systemtime + 120))
	{//2015/7/9 16:56 FangCuisong 如果时间有1分钟的误差 需要使用GPS时间来进行校准
		G_system_time_SetExEx(systemtime);
	}
	db_obd_gpsset(systemtime,Lgpscur.latitude, Lgpscur.longitude);//保存第一个点的GPS数据 2015/7/2 7:22
	GPS_LAST_FRAME_TIME = systemtime;
	Lgpscur.timeflag = systemtime;
	//GPS经纬度(ddmm.mmmm)转度、分、秒
	/*经纬度度转法
	度=取整[latitude/1000000]
	分=取整数[[取小数[latitude/1000000]]*60]
	秒=取整数[取小数[[取小数[latitude/1000000]]*60]*60]
	该算法错误
	*/
	/*经纬度 转换
	度 = 取整[latitude/1000000]
	分 = 取整[取小数[latitude/1000000]]*100]
	秒 = 取整[取小数[[取小数[latitude/1000000]]*100]]*60]
	*/
	lahh = Lgpscur.latitude / 1000000;
	lohh = Lgpscur.longitude / 1000000;
	
	dd = Lgpscur.latitude / 1000000.0;
	dd = dd - lahh;
	dd = dd * 100;
	lamm = dd;
	dd = Lgpscur.longitude / 1000000.0;
	dd = dd - lohh;
	dd = dd * 100;
	lomm = dd;
	
	dd = Lgpscur.latitude / 1000000.0;
	dd = dd - lahh;
	dd = dd * 100;
	dd = dd - lamm;
	dd = dd * 60;
	lass = dd;
	dd = Lgpscur.longitude / 1000000.0;
	dd = dd - lohh;
	dd = dd * 100;
	dd = dd - lomm;
	dd = dd * 60;
	loss = dd;
	u8flag = 0;
	
	GPS_LAST_LAC = Lgpscur.latitude / 10000.0;
	GPS_LAST_LON = Lgpscur.longitude / 10000.0;
	GPS_Speed = Lgpscur.speed;
	
	//user_debug("GPS_LAST_LAC=[%.4f-%.4f]", GPS_LAST_LAC,GPS_LAST_LON);
	if(0x55 == Lgpsm.gpscur.enable){//当经纬度相差1秒时认为发生了移动
		if(lass >= Lgpsm.latss + 1.0)u8flag ++;
		else if(lass + 1.0 <= Lgpsm.latss)u8flag ++;
		if(loss >= Lgpsm.longss + 1.5)u8flag ++;
		else if(loss + 1.5 <= Lgpsm.longss)u8flag ++;
	}
	else u8flag = 1;
	speed = 0;
	u8result = obd_vehiclespeedget(&speed);
	if(0 == u8result){//OBD速度有效
		if(speed < 1){
			Lgpscur.speed = speed * 100;//当车辆静止时GPS速度设置为0 防止静态漂移
		}
		else{
		}
	}
	
	//过滤运行过程中异常数据
	/*2015/9/10 18:20 测试中发现车辆从行使到停止会产生一个比较大的漂移,需要过滤该漂移
	*/
	if(/*Lgpscur.angle && Lgpscur.speed && */Lgpsm.gpscur.latitude != 0 && 0x55 == Lgpsm.gpscur.enable && LGPS_DATA_ERROR_NUM < 3)
	{//运行中的数据校准
		  dd = Lgpscur.speed / 100.0;//速度在转换过程中乘了100
		  dd = dd / 3.6;//Km-->m
		  //度、分、秒--->秒
		  long1 = Lgpsm.longss + Lgpsm.longmm * 60 + Lgpsm.longhh * 60 * 60;
		  long2 = loss + lomm * 60 + lohh * 60 * 60;
		  lat1 = Lgpsm.latss + Lgpsm.latmm * 60 + Lgpsm.lathh * 60 * 60;
		  lat2 = lass + lamm * 60 + lahh * 60 * 60;
		  
		  if(long1 < long2)longex = long1 - long2;
		  else longex = long2 - long1;
		  if(lat1 < lat2)latex = lat1 - lat2;
		  else latex = lat2 - lat1;
		  
		  if(0 == Lgpscur.angle || 0 == Lgpscur.speed)
		  {//防止静态漂移过大 2015/9/10 18:02 fangcuisong 
		  		if(longex * 31 > 10.1 || latex * 31 > 10.1)
				{//静态漂移最大允许10m  2015/10/1 7:16 fangcuisong
	        			user_infor("e:GPS-data-error[%d-%d]",longex,latex);
	        			LGPS_DATA_ERROR_NUM ++;//防止某种情况下正常数据被丢弃 因此如果连续三次认为数据异常则强制认为数据是正常的
	        			if(Lgpscur.speed > 500)LGPS_FRAME_SEND_TIME = 0;//车速非0时 出现数据丢失后下一帧数据强制上传
	        			return 2;//当检测到X/Y坐标偏移大于车辆的行使距离时数据丢弃
	      	  		}
		  }
		  else
		  {
	        		if(longex * 31 > dd || latex * 31 > dd)
				{
	        			user_infor("e:GPS-data-error[%d-%d]",longex,latex);
	        			LGPS_DATA_ERROR_NUM ++;//防止某种情况下正常数据被丢弃 因此如果连续三次认为数据异常则强制认为数据是正常的
	        			if(Lgpscur.speed > 500)LGPS_FRAME_SEND_TIME = 0;//车速非0时 出现数据丢失后下一帧数据强制上传
	        			return 2;//当检测到X/Y坐标偏移大于车辆的行使距离时数据丢弃
	        		}
	    	  }
  }
  LGPS_DATA_ERROR_NUM = 0;
	
	//计算当前数据帧是否需要返回到服务器
	//第一帧无条件返回
	//非首帧需要根据行驶相对方向决定返回次数 静止时每60S返回一次
	//                                       角度偏移<1度时10S返回一次
	//                                       角度偏移<2度时5S返回一次
	//                                       角度偏移<4度时2S返回一次
	//                                       角度偏移>4度时1S返回一次
	if(0 == Lgpsm.gpscur.enable || ((GPS_LAST_FRAME_TIME > Lgpscursend.timeflag) && (GPS_LAST_FRAME_TIME - Lgpscursend.timeflag > 60)))
	{//第一帧数据无条件上传到服务器 或GPS时间超过60S
		LGPS_FRAME_SEND_TIME = 0;
	}
	else if(Lgpscur.speed < 2 || (0 == Lgpscur.angle && 0 == Lgpsm.gpscur.angle))
	{//如果不判断Lgpsm.gpscur.angle 则从运动到停止最终停止点其速度、角度将不会为0
	 	if(LGPS_FRAME_SEND_TIME)LGPS_FRAME_SEND_TIME --;
	 	else LGPS_FRAME_SEND_TIME = 40;//40秒一次
	 }
	 else
	 {
	   	if(Lgpscur.angle > Lgpsm.gpscur.angle)u32t1 = Lgpscur.angle - Lgpsm.gpscur.angle;
	   	else u32t1 = Lgpsm.gpscur.angle - Lgpscur.angle;
	   	if(0 == LGPS_FRAME_SEND_TIME)
		{
	   		if(u32t1 < 100)LGPS_FRAME_SEND_TIME = 10;
	   		else if(u32t1 < 200)LGPS_FRAME_SEND_TIME = 5;
	   		else if(u32t1 < 400)LGPS_FRAME_SEND_TIME = 2;
	     		else LGPS_FRAME_SEND_TIME = 0;
	   	}
	   	else
		{
	     		LGPS_FRAME_SEND_TIME --;
	     		if(u32t1 > 400)LGPS_FRAME_SEND_TIME = 0;
	     		else if(u32t1 > 200 && LGPS_FRAME_SEND_TIME > 2)LGPS_FRAME_SEND_TIME = 2;
	   	}
	 }
	 //决定是否缓冲区数据更新为当前数据，应该：
	 //静态时数据不更新
	 //运行过程中需要根据其他参数一起判断，如：速度、角度
	 if(Lgpsm.gpscur.angle > Lgpscur.angle)angle = Lgpsm.gpscur.angle - Lgpscur.angle;
	 else angle = Lgpscur.angle - Lgpsm.gpscur.angle;
	 if(0 == LGPS_FRAME_SEND_TIME || 0x55 == LGPS_FRAME_SEND_TIME_EX || u8flag || Lgpsm.gpscur.enable != 0x55 || speed > 0 || Lgpscur.speed >= 800 || /*测试中发现静止时可能产生角度 而且角度比较大angle > 100 ||*//*静止到运行或运行到静止时可快速同步*/
	    lahh != Lgpsm.lathh || lohh != Lgpsm.longhh || lamm != Lgpsm.latmm || lomm != Lgpsm.longmm)
	   {//更新坐标
	    	//user_debug(">>[%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d]",u8flag,Lgpsm.gpscur.enable,speed,Lgpscur.speed,lahh,Lgpsm.lathh,lohh,Lgpsm.longhh,lamm,Lgpsm.latmm,lomm,Lgpsm.longmm);
	    	//该处理主要是为了防止静态漂移 通过测试GPS停止状态下相对角度一定为0
	    	Lgpsm.gpscur.latitude = Lgpscur.latitude;
	      Lgpsm.gpscur.latflag = Lgpscur.latflag;
	      Lgpsm.gpscur.longitude = Lgpscur.longitude;
	      Lgpsm.gpscur.longflag = Lgpscur.longflag;
	      Lgpsm.gpscur.speed = Lgpscur.speed;
	      Lgpsm.gpscur.angle = Lgpscur.angle;
	      Lgpsm.gpscur.ymd = Lgpscur.ymd;
	      Lgpsm.gpscur.time = Lgpscur.time;
	      Lgpsm.gpscur.timeflag = Lgpscur.timeflag;
	      if(LGPS_TIME_SYN != 0x55)
	      {
	      		LGPS_TIME_SYN = 0x55;
	      }
	      //user_debug("timeflag===%d",Lgpsm.gpscur.timeflag);
	      Lgpsm.gpscur.enable = 0x55;
	      Lgpsm.lathh = lahh;
	      Lgpsm.longhh = lohh;
	      Lgpsm.latmm = lamm;
	      Lgpsm.longmm = lomm;
	      Lgpsm.latss = lass;
	      Lgpsm.longss = loss;
	      Lgpsm.vehiclespeed = speed;
	}
	if(0x55 == LGPS_FRAME_SEND_TIME_EX || 0 == LGPS_FRAME_SEND_TIME)
	{
		  if(Lgpscursend.itemflag > 0)
		  {//发送缓冲区的数据没有被急时发送出去
		  	if(0x55 == Lgpscursend.enable)eat_sleep(5);
		  	Lgpscursend.enable = 0x55;
		  	Lgps.unit[Lgps.in].itemflag = Lgpscursend.itemflag;
		  	Lgpscursend.itemflag = 0;
		  	Lgps.unit[Lgps.in].latitude = Lgpscursend.latitude;
	      		Lgps.unit[Lgps.in].latflag = Lgpscursend.latflag;
	      		Lgps.unit[Lgps.in].longitude = Lgpscursend.longitude;
	      		Lgps.unit[Lgps.in].longflag = Lgpscursend.longflag;
	      		Lgps.unit[Lgps.in].speed = Lgpscursend.speed;
	      		Lgps.unit[Lgps.in].angle = Lgpscursend.angle;
	      		Lgps.unit[Lgps.in].ymd = Lgpscursend.ymd;
	      		Lgps.unit[Lgps.in].time = Lgpscursend.time;
	      		Lgps.unit[Lgps.in].timeflag = Lgpscursend.timeflag;
	      		Lgps.unit[Lgps.in].distance = Lgpscursend.distance;
	      		Lgps.unit[Lgps.in].fuel = Lgpscursend.fuel;
	      		Lgps.unit[Lgps.in].celid = Lgpscursend.celid;
	      		Lgps.unit[Lgps.in].lacid = Lgpscursend.lacid;
	      		Lgps.in ++;
	      		if(Lgps.in >= GPS_DATA_MAX)Lgps.in = 0;
	      		if(Lgps.num <= GPS_DATA_MAX)
			{
	      			if(0x55 == GPS_SEND_NUMflag)eat_sleep(1);
		      		GPS_SEND_NUMflag = 0x55;
		      		Lgps.num ++;
		      		GPS_SEND_NUMflag = 0x00;
	      		}
	      		else
			{//数据保存到大缓冲区中
	      			user_infor("e:GPS-BUF full");
	      			//gps_tobigmem();
	      		}
	    }
	    Lgpscursend.latitude = Lgpsm.gpscur.latitude;
	    Lgpscursend.latflag = Lgpsm.gpscur.latflag;
	    Lgpscursend.longitude = Lgpsm.gpscur.longitude;
	    Lgpscursend.longflag = Lgpsm.gpscur.longflag;
	    Lgpscursend.speed = Lgpsm.gpscur.speed;
	    Lgpscursend.angle = Lgpsm.gpscur.angle;
	    Lgpscursend.ymd = Lgpsm.gpscur.ymd;
	    Lgpscursend.time = Lgpscur.time;
	    Lgpscursend.timeflag = Lgpsm.gpscur.timeflag;
	    
	    //user_debug("in---------------");
	    //u32time2ydmhms(systemtime);
	    u32t1 = 0;
	    u32t2 = 0;
	   // u8result = gps_obddataget(&u32t1, &u32t2);
	    Lgpscursend.distance = u32t1;
	    Lgpscursend.fuel = u32t2;
	    u32t1 = 0;
	    u32t2 = 0;
	    gps_gsmpositionget(&u32t1, &u32t2);                                
	    Lgpscursend.celid = u32t1;
	    Lgpscursend.lacid = u32t2;                                                     
	    
	    Lgpscursend.itemflag = 0x3f;
	    Lgpscursend.enable = 0;
	    user_infor("e:GPS-TO-SVR");
	    
	    Lgpscur.itemflag = 0;//当前数据已经被使用
  }
  LGPS_FRAME_SEND_TIME_EX = 0;
	//user_infor("==[%d-%d][%d-%d-%d-%f][%d-%d-%d-%f]",Lgpsm.gpscur.speed,Lgpsm.gpscur.angle,Lgpscur.latitude,Lgpsm.lathh,Lgpsm.latmm,Lgpsm.latss,Lgpscur.longitude,Lgpsm.longhh,Lgpsm.longmm,Lgpsm.longss);
	user_infor("e:==[gps.speed=%d-gps.angle=%d]",Lgpsm.gpscur.speed,Lgpsm.gpscur.angle);
	return 0;
}

/*当从有GPS信号进入到无GPS信号时 最后一帧数据必须强制返回
*/
u8 gps_data_transform_compel(void){
	if(Lgpscur.itemflag < 0x3f)return 1;
	LGPS_FRAME_SEND_TIME_EX = 0x55;
	gps_data_transform();
	Lgpscur.itemflag = 0;
	return 0;
}

void gps_Init(void){
	u32 lac,loc;
	Lgps_status = 0;
	LGPS_FRAME_SEND_TIME_EX = 0;
	LGPS_FRAME_SEND_TIME = 0;
	LGPS_TIME_SYN = 0;
	Lgps_enable_flag = 0;//默认GPS数据无效
	Lupdate_time = 0;
	Lgps.num = 0;
	Lgps.in = 0;
	Lgps.out = 0;
	Lgpsm.vehiclespeed = 0;
	Lgpsm.gpscur.enable = 0;
	Lgpsm.gpscur.latitude = 0;
	Lgpsm.gpscur.longitude = 0;
	Lgpscur.enable = 0;
	Lgpscur.itemflag = 0;
	GPS_UNIT_LEN = 0;
	memset(GPS_UNIT_BUF, 0 , GPS_UNIT_MAX + 1);
	memset(MOBILE_INFOR_BUF, 0 , MOBILE_INFOR_MAX + 1);
	LGPS_SYSTEM_TIME_SYNCHRON = 0;
	GPS_UNIT_LENflag = 0;
	GPS_SEND_NUMflag = 0;
	
	gps_ubx_datalen = 0;
	gps_ubx_update_time = 0;
	
	Lgpscursend.enable = 0;
	Lgpscursend.itemflag = 0;
	
	GPS_LAST_FRAME_TIME = 0;//上电时初始化为0 即每次上电必须重新获取星历数据
	/*GPS位置初始坐标定义为0 服务器需要保存最近一次的坐标
	*/
	//GPS_LAST_LAC = 0;//2229.5567;//2229.5567, 11354.9492
	//GPS_LAST_LON = 0;//11354.9492;
  
  	lac = 0;
  	loc = 0;
  	db_gps_get(&lac, &loc);
  	GPS_LAST_LAC = lac / 10000.0;
  	GPS_LAST_LON = loc / 10000.0;
}

u8 gps_read(void){
	u8 datatemp[64];
	u32 datalen;
	
	if(GPS_UNIT_LEN != 0){
		while(1){//数据处理不过来直接丢弃
		    datalen = eat_uart_read(EAT_UART_2, datatemp, 64);
		    if(datalen < 64)break;
	  }
		return 1;
	}
	
	datalen  = 0;
	while(1){
		datalen = eat_uart_read(EAT_UART_2, GPS_UNIT_BUF, GPS_UNIT_MAX);
		if(datalen < GPS_UNIT_MAX)break;//如果GPS_UNIT_LEN == GPS_UNIT_MAX 说明缓冲区满 吧数据读出后直接扔了
	}
	
	if(datalen != 0){
		if(0x55 == GPS_UNIT_LENflag)eat_sleep(5);
	  GPS_UNIT_LENflag = 0x55;
	  GPS_UNIT_LEN = datalen;
	  GPS_UNIT_LENflag = 0;
		return 1;
	}
	return 0;
}


/*GPS信号来自OBD,格式为:
*a5 a5 len1 len2 0x14 0x03 latflag latitude[4byes] longflag longitude[4bytes] speed[4bytes] angle[4bytes]
*/
u8 gps_dealex(void){
	u32 datalen, dataindex;
	u8 cs,cs1;
	
	user_debug("i:gps_dealex:[%d]", GPS_UNIT_temp_LEN);
	if(0 == GPS_UNIT_temp_LEN)return 1;
	Lgpscur.itemflag = 0;
	Lgps_status = 1;
	if(GPS_UNIT_BUF_temp[0] != 0xa5 || GPS_UNIT_BUF_temp[1] != 0xa5 || GPS_UNIT_BUF_temp[4] != 0x14 || GPS_UNIT_BUF_temp[5] != 0x03){
		user_debug("i:gps_dealex formate error");
		return 1;
	}
	datalen = GPS_UNIT_BUF_temp[2];
	datalen = (datalen << 8) + GPS_UNIT_BUF_temp[3];
	
	GPS_UNIT_temp_LEN = datalen + 5;//修正长度
	
	dataindex = 6;
	Lgpscur.latflag = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.latitude = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.latitude = (Lgpscur.latitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.latitude = (Lgpscur.latitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.latitude = (Lgpscur.latitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.longflag = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.longitude = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.longitude = (Lgpscur.longitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.longitude = (Lgpscur.longitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.longitude = (Lgpscur.longitude << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.speed = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.speed = (Lgpscur.speed << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.speed = (Lgpscur.speed << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.speed = (Lgpscur.speed << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.angle = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.angle = (Lgpscur.angle << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.angle = (Lgpscur.angle << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.angle = (Lgpscur.angle << 8) + GPS_UNIT_BUF_temp[dataindex ++];

	
	Lgpscur.ymd = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[2]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.ymd = (Lgpscur.ymd << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[1]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.ymd = (Lgpscur.ymd << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[0]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.ymd = (Lgpscur.ymd << 8) + GPS_UNIT_BUF_temp[dataindex ++];

	
	Lgpscur.time = GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[3]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.time = (Lgpscur.time << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[4]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.time = (Lgpscur.time << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	Lgpscur.Time808[5]=GPS_UNIT_BUF_temp[dataindex];
	Lgpscur.time = (Lgpscur.time << 8) + GPS_UNIT_BUF_temp[dataindex ++];
	
	Lgpscur.itemflag = 0x3f;
      Lgps_enable_flag = 0x55;
	UtcToBeiJingTIme(Lgpscur.Time808);
	gps_data_transform();
	return 0;
}

/*
*GPS数据返回到服务器
*
********************************************/
u8 gps_tosvr(void)
{
	u8 backtosvr[64];
	u8 index,id;
	u32 time;
	u32 sendtime,data;
	
	if(Lgps.num < 1 && Lgpscursend.itemflag < 1)return 0;
	time = user_time();                                                                                          //lilei-?-not use-2016-10-18
	index = 0;
	memset((s8 *)backtosvr, 0, 64);
	//G_system_time_Set(Lgps.unit[Lgps.out].ymd, Lgps.unit[Lgps.out].time);
	if(Lgpscursend.itemflag)
	{//发送缓冲区中的数据有效
		/*if(0 == Lgpscursend.latitude || 0 == Lgpscursend.longitude){
			Lgpscursend.itemflag = 0;
			return 0;
		}*/
		user_debug("\r\ni:--lat=[%d] lon=[%d],Lgpscursend.latflag=%d\r\n",Lgpscursend.latitude,Lgpscursend.longitude,Lgpscursend.latflag);   //add by lilei-2016-0512
		if(0x55 == Lgpscursend.enable)eat_sleep(5);
		Lgpscursend.enable = 0x55;
		data = Lgpscursend.latflag;
		Lbdata1_insert(GPS_DATA_latflag_id, data);
		Lbdata1_insert(GPS_DATA_latitude_id, Lgpscursend.latitude);
		data = Lgpscursend.longflag;
		Lbdata1_insert(GPS_DATA_longflag_id, data);
		Lbdata1_insert(GPS_DATA_longitude_id, Lgpscursend.longitude);
		Lbdata1_insert(162, Lgpscursend.timeflag);
		Lbdata1_insert(7, Lgpscursend.speed);
		Lbdata1_insert(GPS_DATA_angle_id, Lgpscursend.angle);
		Lbdata1_insert(GPS_DATA_celid_id, Lgpscursend.celid);
		Lbdata1_insert(GPS_DATA_lacid_id, Lgpscursend.lacid);
		Lbdata1_insert(164, gps_3g_check);
		gps_3g_check = 0;//发送一次后该参数自动清0
		for(index = 0; index < Lobdgps.num; index ++)
		{
			if(Lobdgps.id[index]==0)
			{
				continue;
			}                                                  			
			Lbdata1_insert(Lobdgps.id[index], Lobdgps.data[index]);
		}
		//anydata_lat_set(Lgpscursend.latitude);
		//anydata_lon_set(Lgpscursend.longitude);
		//anydata_heading_set(Lgpscursend.angle);
		//anydata_gpstime_set(Lgpscursend.timeflag);     
	  	Lbdata1_insert_done();
	  	Lgpscursend.itemflag = 0x00;
	  	Lgpscursend.enable = 0x00;
	}
	else if(Lgps.num && 0x00 == Lgps.oprate)
	{//读缓冲区中的数据
		/*if(0 == Lgps.unit[Lgps.out].latitude || 0 == Lgps.unit[Lgps.out].longitude){
			Lgps.out ++;
	    if(Lgps.out >= GPS_DATA_MAX)Lgps.out = 0;
	    if(0x55 == GPS_SEND_NUMflag)eat_sleep(1);
	    GPS_SEND_NUMflag = 0x55;
	    Lgps.num --;
	    GPS_SEND_NUMflag = 0x00;
	  	return 0;
		}*/
		//user_debug(">>>time=[%d] index=[%d]",Lgps.unit[Lgps.out].timeflag, Lgps.out);
		user_debug("\r\nlilei- Lgps.unit[Lgps.out].latflag=%d\r\n", Lgps.unit[Lgps.out].latflag);
		Lgps.oprate = 0x55;
		data = Lgps.unit[Lgps.out].latflag;
		Lbdata1_insert(GPS_DATA_latflag_id, data);
		Lbdata1_insert(GPS_DATA_latitude_id, Lgps.unit[Lgps.out].latitude);
		data = Lgps.unit[Lgps.out].longflag;
		Lbdata1_insert(GPS_DATA_longflag_id, data);
		Lbdata1_insert(GPS_DATA_longitude_id, Lgps.unit[Lgps.out].longitude);
		Lbdata1_insert(162, Lgps.unit[Lgps.out].timeflag);//时间戳必须有效
		Lbdata1_insert(7, Lgps.unit[Lgps.out].speed);
		//anydata_speed_set(Lgps.unit[Lgps.out].speed);      
		Lbdata1_insert(GPS_DATA_angle_id, Lgps.unit[Lgps.out].angle);
		Lbdata1_insert(GPS_DATA_celid_id, Lgps.unit[Lgps.out].celid);
		Lbdata1_insert(GPS_DATA_lacid_id, Lgps.unit[Lgps.out].lacid);
		Lbdata1_insert(164, gps_3g_check);
		gps_3g_check = 0;//发送一次后该参数自动清0
	
		for(index = 0; index < Lobdgps.num; index ++)
		{
			//Lbdata1_insert(Lobdgps.id[index], Lobdgps.data[index]);
			if(Lgps.unit[Lgps.out].obddata.id[index]==0)
			{

				continue;
			}							                  
			Lbdata1_insert(Lgps.unit[Lgps.out].obddata.id[index], Lgps.unit[Lgps.out].obddata.data[index]);
			//if(0x8c == Lgps.unit[Lgps.out].obddata.id[index])anydata_fuel_set(Lgps.unit[Lgps.out].obddata.data[index] *10);//本次油耗      add by lilei-2016-0823 
			//else if(0x95 == Lgps.unit[Lgps.out].obddata.id[index])anydata_dist_set(Lgps.unit[Lgps.out].obddata.data[index] * 10);//本次里程 add by lilei-2016-0823
			//else if(6 == Lgps.unit[Lgps.out].obddata.id[index])anydata_engine_set(Lgps.unit[Lgps.out].obddata.data[index] / 100);//发动机转速add by lilei-2016-0823
		}
	  	Lgps.out ++;
		if(Lgps.out >= GPS_DATA_MAX)Lgps.out = 0;
		if(0x55 == GPS_SEND_NUMflag)eat_sleep(1);
		GPS_SEND_NUMflag = 0x55;
		Lgps.num --;
		GPS_SEND_NUMflag = 0x00;
		Lgps.oprate = 0x00;
		Lbdata1_insert_done();
	}
	return 0;
}

u8 gps_tobigmem(void){
	u8 index0;
	u8 backtosvr[64];
	u8 index;
	u32 sendtime;
	
	if(Lgps.num < 1)return 0;
	Lgps.oprate = 0x55;
	while(1){
		index = 0;
		backtosvr[index ++] = 0x14;
	  backtosvr[index ++] = 0x03;
	  backtosvr[index ++] = Lgps.unit[Lgps.out].latflag;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].latitude >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].latitude >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].latitude >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].latitude >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = Lgps.unit[Lgps.out].longflag;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].longitude >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].longitude >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].longitude >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].longitude >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].speed >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].speed >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].speed >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].speed >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].angle >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].angle >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].angle >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].angle >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].celid >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].celid >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].celid >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].celid >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].lacid >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].lacid >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].lacid >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].lacid >> 0) & 0x00ff;
	  index ++;
	  index ++;
	  index ++;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].distance >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].distance >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].distance >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].distance >> 0) & 0x00ff;
	  
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].fuel >> 24) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].fuel >> 16) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].fuel >> 8) & 0x00ff;
	  backtosvr[index ++] = (Lgps.unit[Lgps.out].fuel >> 0) & 0x00ff;
	  sendtime = Lgps.unit[Lgps.out].timeflag;
	  
	  Lgps.out ++;
	  if(Lgps.out >= GPS_DATA_MAX)Lgps.out = 0;
	  Lgps.num --;
	  SVR_FrameSend_Mem(backtosvr, index, sendtime);
	  if(0 == Lgps.num)break;
	}
	Lgps.oprate = 0x00;
	return 0;
}


extern void TastStartSet_04(u8 flag);
void app_gps(void *data)
{
	EatEvent_st event;
  
  eat_sleep(500);
  user_infor("e:app_gps ..");
  gps_Init();
  TastStartSet_04(1);
  eat_sleep_enable(EAT_TRUE);
	while(EAT_TRUE){
		eat_get_event_for_user(EAT_USER_4, &event);
		if(event.event == EAT_EVENT_USER_MSG){
      if(0x01 == event.data.user_msg.len && 'S' == *(event.data.user_msg.data)){
      	//保存当前星历
      	//gps_assist_get(); //后面实现
      	user_debug("i:GPS sleep");
      	system_subtask_status_set(0x08);
      }
      else if(0x01 == event.data.user_msg.len && 'U' == *(event.data.user_msg.data)){
      	user_debug("i:GPS weakup");//gps_weakup();
      }
			if(GPS_UNIT_LEN != 0){
				if(1){//调试
					//if(m2m_status() > 6)user_debug(GPS_UNIT_BUF);
				}
				if(GPS_UNIT_LEN != 0){
					memcpy((s8 *)GPS_UNIT_BUF_temp, (s8 *)GPS_UNIT_BUF, GPS_UNIT_LEN);
		    	GPS_UNIT_temp_LEN = GPS_UNIT_LEN;
		    	memset(GPS_UNIT_BUF, 0, GPS_UNIT_MAX);
				  GPS_UNIT_LEN = 0;
					gps_dealex();
					
				}
				if(MOBILE_INFOR_BUF[0] != 0){
					gps_mobileinfor_deal();
				  memset(MOBILE_INFOR_BUF, 0 , MOBILE_INFOR_MAX);
				}
			}
		}
	}
}

/********************************************
*GPS星历
*GSM初始化后检测GPS数据，如果GPS数据无效则获取星历
*星历来自三个方面
*1、来自之前睡眠前保存的星历
*2、来自u-blox服务器的星历�
*static u8 gps_ubx_online[3072];//最多不超过3K数据
*static u32 gps_ubx_datalen = 0;
*static u32 gps_ubx_update_time = 0;//ubx数据获取的时间
****************************************************************************/
/*直接从U-blox服务器获取星历数据
*获取星历进行快速定位
*G_system_time
*bit31-bit26:yy
*bit25-bit22:mm
*bit21-bit17:dd
*bit16-bit0:time
*/
/*
*星历数据来自睡眠前保存的星历
*/
u8 gps_assist_online_frombuf(void){
	
	if(0 == gps_ubx_datalen)return 1;
	eat_uart_write(EAT_UART_2,(const unsigned char *)gps_ubx_online, gps_ubx_datalen);
	gps_ubx_datalen = 0;//星历用过后无效
	return 0;
}
/*通过U-blox服务器获取星历
**************************************************/
u8 gps_assist_online_from_ublox(void){
	u32 curtime,t1,t2,overtime;
	u8 data,framenum;
	u8 cmd[128];
	u8 *recdata;
	double dlac,dlon;
	u16 datalen,u16t1,u16t2,ubxlen,ubxindex;
	
	//同步星历
	if(GPS_LAST_LAC < 0.5 && GPS_LAST_LON < 0.5)return 1;
	t1 = GPS_LAST_LAC / 100;
	dlac = t1 + ((GPS_LAST_LAC / 100.0 - t1)*100.0) / 60.0;
	t1 = GPS_LAST_LON / 100;
	dlon = t1 + ((GPS_LAST_LON / 100.0 - t1)*100.0) / 60.0;
	
	user_debug("i:gps_assist_online_from_ublox[%.4f;%.4f;%.4f;%.4f] >>", GPS_LAST_LAC, GPS_LAST_LON,dlac,dlon);
	
	sprintf((s8 *)cmd, "cmd=aid;user=fcsong000833@163.com;pwd=Vmyjf;lat=%.4f;lon=%.4f;pacc=10000\n", dlac, dlon);//2229.5567, 11354.9492);
	//user_debug(cmd);
	if(0 == AT_CIPSEND_1(cmd, strlen((s8 *)cmd))){
		overtime = 0;
		framenum = 0;
		ubxlen = 0;
		ubxindex = 0;
		
		while(1){
			if(0x55 == Lgps_enable_flag){//GPS已经有效不需要更新星历
				return 0;
			}
			datalen = MDM_DataToApp(&recdata);
			if(datalen != 0){				
				framenum ++;
				user_debug("i:read[%d-%d-%d]", framenum,datalen,ubxindex);
				if(0x0d == *(recdata +0) || 0x0a == *(recdata +0)){
					recdata ++;
					datalen --;
				}
				if(0x0d == *(recdata +0) || 0x0a == *(recdata +0)){
					recdata ++;
					datalen --;
				}
				if('+' == *(recdata +0) && 'R' == *(recdata +1) && 'E' == *(recdata +2)){
					if(*(recdata +9) != '1')continue;//数据不是来自ublox服务器
					for(u16t1 = 0; u16t1 < datalen; u16t1 ++){
						if(':' == *recdata){
							recdata += 3;//跳过0d 0a
							if(datalen > (u16t1 + 3))datalen = (datalen - u16t1 - 3);
							break;
						}
						recdata ++;
					}
				}
				if(0 == ubxlen){
				    for(u16t1 = 0; u16t1 < datalen; u16t1 ++){
				    	if('u' == *(recdata +0) && '-' == *(recdata +1) && 'b' == *(recdata +2)){
				    		datalen = datalen - u16t1;
				    		break;
				    	}
				    	recdata ++;
				    }
			  }
				if('u' == *(recdata +0) && '-' == *(recdata +1) && 'b' == *(recdata +2) && 'l' == *(recdata +3) && 'o'== *(recdata +4) && 'x' == *(recdata +5)){
					for(u16t1 = 6; u16t1 <  datalen; u16t1 ++){
						if('L' == *(recdata +u16t1 +0) && 'n' == *(recdata +u16t1 +2) && 't' == *(recdata +u16t1 +4) && 'h' == *(recdata +u16t1 +5)){
							u16t1 += 6;
							for(; u16t1 < datalen; u16t1 ++){
								if(*(recdata +u16t1) >= '0' && *(recdata +u16t1) <= '9'){
									ubxlen = ubxlen * 10 + *(recdata +u16t1) - '0';
								}
								else if(0x0d == *(recdata +u16t1) || 0x0a == *(recdata +u16t1))break;
							}
							if(ubxlen > 3072){//数据太多
								user_infor("e:gps_assist_online ubx too much");
	              return 0;
							}
							if(ubxlen < 100){
								user_debug("i:gps_assist_online no data");
	              return 1;
							}
							for(; u16t1 < datalen; u16t1 ++){
								if('n' == *(recdata +u16t1 +0) && '/' == *(recdata +u16t1 +1) && 'u' == *(recdata +u16t1 +2) && 'b' == *(recdata +u16t1 +3)&& 'x' == *(recdata +u16t1 +4)){
									u16t1 += 9;///ubx后面连续两个\r\n
									break;
								}
							}
							for(; u16t1 < datalen; u16t1 ++){
								gps_ubx_online[ubxindex ++] = *(recdata +u16t1);
							}
						}
					}
				}
				else{
				   if(ubxlen != 0 && ubxindex < ubxlen){
				   	  for(u16t1 = 0; u16t1 < datalen; u16t1 ++){
								gps_ubx_online[ubxindex ++] = *(recdata +u16t1);
							}
				   }
			  }
				overtime = 0;
			}
			if(ubxlen != 0 && ubxindex >= ubxlen){
				user_debug("i:gps_assist_online data read done[%d-%d]", ubxindex,ubxlen);
				if(ubxindex > ubxlen){//数据中包含一条"+REC..."信息 需要提取出来
					for(u16t1 = 0; u16t1 < ubxindex; u16t1 ++){
						if(0x0d == gps_ubx_online[u16t1] && 0x0a == gps_ubx_online[u16t1+1] && '+' == gps_ubx_online[u16t1+2] && 'R' == gps_ubx_online[u16t1+3]){
							for(u16t2 = u16t1; u16t2 < ubxindex; u16t2 ++){
								if(':' == gps_ubx_online[u16t2]){
									u16t2 += 3;//含回车换行
									break;
								}
							}
							memcpy((char *)&gps_ubx_online[u16t1],(char *)&gps_ubx_online[u16t2],ubxindex - u16t2 + 1);
							ubxindex = ubxindex - (u16t2 - u16t1);
							break;
						} 
					}
				}
				if(ubxindex != ubxlen){
					user_debug("i:gps_assist_online datalen error[%d-%d]", ubxindex,ubxlen);
					return 1;
				}
				gps_ubx_datalen = ubxindex;
				//eat_uart_write(EAT_UART_2,(const unsigned char *)gps_ubx_online, ubxindex);
				break;
			}
			overtime ++;
			eat_sleep(5);
			if(ubxlen != 0 && overtime > 400){//已经接收到数据 但可能网络问题导致数据丢失
				user_debug("i:gps_assist_online rec overtime1");
				return 1;
			}
			if(overtime > 1200){
				user_debug("i:gps_assist_online rec overtime");
				return 2;
			}
		}
		user_infor("e:gps_assist_online data read success[%d-%d]",ubxlen,ubxindex);
	  return 0;
	}
	else{
	  //数据发送失败
	   user_debug("i:gps_assist_online send cmd error");
	   return 0x7f;
	}
}




u8 gps_assist_online_from_zg(void){
	u32 lac,log;
	
	lac = GPS_LAST_LAC * 10000;
	log = GPS_LAST_LON * 10000;
	if(0 == lac || 0 == log)return 1;
	gps_ubx_datalen = SVR_gps_online(gps_ubx_online, lac, log);
	if(0 == gps_ubx_datalen)return 1;
	user_debug("i:GPS_ZG_Online OK[%d]", gps_ubx_datalen);
	eat_uart_write(EAT_UART_2,(const unsigned char *)gps_ubx_online, gps_ubx_datalen);
	gps_ubx_datalen = 0;//星历用过后无效
	return 0;
}

/*设备已经连接到ZG服务器 但没有获取到GPS数据
*直接从ZG服务器获取GPS数据
*/
u8 gps_assist_online_from_zgex(void){
	u32 lac,log;
	
	lac = GPS_LAST_LAC * 10000;
	log = GPS_LAST_LON * 10000;
	if(0 == lac || 0 == log)return 1;
	if(0x55 == Lgps_enable_flag){//GPS已经有效不需要更新星历
			return 0;
	}
	gps_ubx_datalen = SVR_gps_onlineEx(gps_ubx_online, lac, log);
	if(0 == gps_ubx_datalen)return 1;
	user_debug("i:GPS_ZG_Online_EX OK[%d]", gps_ubx_datalen);
	return 0;
}

/*
*M2M主动将星历数据发送到OBD
*$14 20  OBD请求星历数据
*$54 20  M2M发送星历数据到GPS
*/
u8 gps_assist_toOBD(void){
	u16 sendindex, sendlen;
	u8 cmd1,cmd2,frameindex,errornum;
	u16 datalen;
	u8 *data;
	u32 overtime;
	u8 senddata[210];
	
	if(0 == gps_ubx_datalen || gps_ubx_datalen > 3072)
	{
		if(0 == gps_assist_online_from_zgex());//if(0 == gps_assist_online_from_ublox());
		else return 1;
	}
	senddata[0] = 0x14;
  	senddata[1] = 0x19;
  	senddata[2] = (gps_ubx_datalen >> 8 )& 0x00ff;
  	senddata[3] = (gps_ubx_datalen >> 0 )& 0x00ff;
  	obd_write(senddata, 4);
  	overtime = 0;
  	frameindex = 0;
  	user_debug("i:gps_assist_toOBDEx[%d]", gps_ubx_datalen);
  	errornum = 0;
  	while(1)
	{
  		cmd1 = 0;
  		if(0x00 == obd_data_read(&cmd1, &cmd2, &data, &datalen))
		{
  			if(0x14 == cmd1 && 0x20 == cmd2)
			{
  				errornum = 0;
  				overtime = 0;
  			}
  			else if(0x14 == cmd1 && 0x60 == cmd2)
			{
  				errornum = 0;
  				frameindex = *(data + 6);
_again:
  				overtime = 0;
  				if(0xff == frameindex)
				{
  					user_debug("i:gps_assist_toOBDEx OK");
  	    				return 0;
  				}
  			
  				if(frameindex * 50 > gps_ubx_datalen)
				{
  					user_debug("i:gps_assist_toOBDEx frameindex[%d]",frameindex);
  					continue;
  				}
  				else if(frameindex * 50 + 50 < gps_ubx_datalen)sendlen = 50;
  				else sendlen = gps_ubx_datalen - frameindex * 50;
  				senddata[0] = 0x54;
  	    			senddata[1] = 0x20;
  	    			senddata[2] = frameindex;
  	    			senddata[3] = sendlen;
  	    			memcpy((s8 *)&senddata[4], (s8 *)&gps_ubx_online[frameindex * 50], sendlen);
  	    			obd_write(senddata, sendlen + 4);
  			}
  			else if(0x7f == cmd1 && (0x14 == cmd2 || 0x54 == cmd2))
			{
  				errornum ++;
  				if(errornum > 6)
				{
  					user_debug("i:gps_assist_toOBDEx obd-readerror");
  					return 1;
  				}
  				if(0x20 == *(data + 6))
				{
  			   		user_debug("i:gps_assist_toOBDEx not need");
  			   		return 1;//不需要
  		  		}
  		  		if(0x54 == cmd2)goto _again;
  			}
  		}
  		else
		{
  	   		eat_sleep(5);//参数建议>=5 否则可能起不到延时效果
  	   		overtime ++;
  	   		if(overtime > 1000)
			{
  	   			user_debug("i:gps_assist_toOBDEx overtime[%d,%d]",frameindex,overtime);
  	   			return 1;//2S超时
  	   		}
    		}
  	}
	return 0;
}

/*Return:0=失败
         1=不需要星历
         2=内存星历
         3=ublox星历
         4=zg星历
*/
u8 gps_assist_online(void){
	u32 curtime,t1,t2;
	u8 data;
	
	user_debug("i:gps_assist_online >>[%.4f-%.4f]",GPS_LAST_LAC,GPS_LAST_LON);
	if(GPS_LAST_LAC < 0.5 || GPS_LAST_LON < 0.5)return 1;//坐标无效
	if(0x55 == Lgps_enable_flag)return 1;//GPS有效不需要下载星历
	if(gps_ubx_update_time != 0 && gps_ubx_datalen != 0){
		curtime = G_system_time_getEx();
		if(curtime){//如果当前时间比GPS_LAST_FRAME_TIME多2个小时则需要同步星历 由于该产品为车载产品 几乎不存在车辆部启动情况下被移动的问题因此不考虑位移导致星历更新
			t1 = curtime & 0x001ffff;
			t2 = GPS_LAST_FRAME_TIME & 0x001ffff;
			data = (curtime >> 17) & 0x1f;
			t1 += data * 24 * 3600;
			data = (GPS_LAST_FRAME_TIME >> 17) & 0x1f;
			t2 += data * 24 * 3600;
	    
	    if(t1 < t2 && t2 - t1 < 7200){
	    	gps_assist_online_frombuf();
	    	return 2;
	    }
	    else if(t1 >= t2 && t1 - t2 < 7200){
	    	gps_assist_online_frombuf();
	    	return 2;
	    }
		}
	}
	
	gps_ubx_datalen = 0;
	if(0 == m2m_ser_addrcheck("121.15.156.148")){//星历来自至高服务器
		if(0 == gps_assist_online_from_zg())return 4;
	}
	else{//星历来自u-blox服务器
		gps_assist_online_from_ublox();
		return 3;
	}
	return 0;
}


void UtcToBeiJingTIme(char *TBuf)
{

     u8 iInterval=0;

     u8 iDays=0;
     u16 iYears=0;
    
       TBuf[3]+=8;
	iInterval=(TBuf[3]/24);
	TBuf[3]=TBuf[3]%24;
       if(iInterval>0)
       {


		TBuf[2]+=1;
		switch(TBuf[1])
		{
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
			case 12:
			{
				iDays = 31;
			}
			break;
			case 2:
			{
				iYears=2000+TBuf[0];
				if((((iYears%4)==0)&&((iYears%100)!=0))||((iYears%400)==0))
				{
					iDays=29;
				}
				else
				{

					iDays=28;
				}
			      
			}
			break;
			case 4:
			case 6:
			case 9:
			case 11:
			{
				iDays = 30;
			}
			break;

		}
		iInterval=TBuf[2]-iDays;
		if(iInterval>0)
		{
			TBuf[2]=iInterval;
			TBuf[1]+=1;
			iInterval=TBuf[1]/12;
			TBuf[1]=TBuf[1]%12;
			if(iInterval>0)
			{
				TBuf[0]+=1;

			}

		}
	 }
     
}


