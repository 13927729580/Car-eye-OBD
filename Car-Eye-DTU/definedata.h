/* car-eye��������ƽ̨ 
 * car-eye����������ƽ̨   www.car-eye.cn
 * car-eye��Դ��ַ:  https://github.com/Car-eye-admin
 * Copyright car-eye ��������ƽ̨  2018
 */



#if !defined (__DEFINE_DATA__)
#define __DEFINE_DATA__



#define 	EAT_UART_RX_BUF_LEN_MAX 		512
#define 	EAT_UART_TX_BUF_LEN_MAX 		512
#define   EAT_RTX_BUF_LEN_MAX 512
#define   EAT_UART_MDM_RX_BUF_LEN_MAX   2000

extern u8 PcTool_Rx_buf[EAT_UART_RX_BUF_LEN_MAX + 1];
extern u8 PcTool_Tx_buf[EAT_UART_TX_BUF_LEN_MAX + 1];

extern u8 GPS_Rx_buf[EAT_UART_RX_BUF_LEN_MAX + 1] ;
extern u8 GPS_Tx_buf[EAT_UART_TX_BUF_LEN_MAX + 1] ;

extern u8 MDM_Rx_buf[EAT_UART_MDM_RX_BUF_LEN_MAX + 1] ;
void system_status_set(u8 flag);
void system_subtask_status_set(u8 flag);
u8 system_status_get(void);
//
#endif

