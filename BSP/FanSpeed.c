#include "Pin.H"
#include "BSP.H"
/********************************** ����˵�� ***********************************
*	��������ת��
*******************************************************************************/
#define	fanCountListLen	( 4u+(1u+2u) )
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;		//��ͣ�ļ���
}

uint16_t	FanSpeed_fetch( void )
{
	/*	�̶����1s��¼����ת��Ȧ������������
	 *	���μ����������˲��Ľ��������ת�١�
	 */
	uint8_t 	ii, index = fanCountList_index;
	uint16_t	sum = 0u;
	uint16_t	max = 0u;
	uint16_t	min = 0xFFFFu;
	uint16_t	x0, x1, speed;
	x1 = fanCountList[index];
	for ( ii = fanCountListLen - 1u; ii != 0; --ii )
	{
		//	�����������õ��ٶ�
		x0 = x1;
		if ( ++index >= fanCountListLen ){  index = 0u; }
		x1 = fanCountList[index];
		speed = ( x1 - x0 );
		//	�Զ�����ݽ����˲�
		if ( speed > max ) {  max = speed; }
		if ( speed < min ) {  min = speed; }
		sum += speed;
	}

	speed = (uint16_t)( sum - max - min ) / ( fanCountListLen - (1u+2u));
	
	return	speed  * 30u;
}


extern  uint16_t	volatile  fan_shut_delay;
uint16_t	HCBoxFan_Update( void )
{	
	static uint16_t fanspeed;
	//	�������¼ת��Ȧ��
 	fanCountList[ fanCountList_index] = HCBoxFan_Circle_Read();
	if ( ++fanCountList_index >= fanCountListLen )
	{
		fanCountList_index = 0u;
	}
	fanspeed = FanSpeed_fetch();
	//	���ȿ��ص���̬����
	if ( --fan_shut_delay == 0u )
	{
		HCBoxFan_OutCmd( FALSE );
	}	
	return fanspeed;
}
