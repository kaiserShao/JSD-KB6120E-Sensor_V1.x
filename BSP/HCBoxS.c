/**************** (C) COPYRIGHT 2014 �ൺ���˴���ӿƼ����޹�˾ ****************
* �� �� ��: HCBox.C
* �� �� ��: ����
* ��  ��  : KB-6120E �������¶ȿ���
* ����޸�: 2014��4��21��
*********************************** �޶���¼ ***********************************
* ��  ��: V1.1
* �޶���: ����
* ˵  ��: ������ʾ�ӿڣ���ʾ������״̬
*******************************************************************************/

//#include	"BIOS.H"

#include "Pin.H"
#include "BSP.H"

struct	uHCBox
{
	uint8_t		SetMode;		//	�趨�Ŀ��Ʒ�ʽ����ֹ�����ȡ����䡢�Զ� ���ַ�ʽ
	FP32		SetTemp;		//	�趨�Ŀ����¶ȣ�
	FP32		RunTemp;		//	ʵ��������¶ȣ�
	uint16_t	OutValue;		//	�����ź����ֵ[0,+2000]��>1000��ʾ���ȣ�<1000��ʾ���䡣
}HCBox;

enum
{	//	����������¶ȵķ�ʽ
	MD_Shut,
	MD_Heat,
	MD_Cool,
	MD_Auto
};


/********************************** ����˵�� ***********************************
*	��������ת��
*******************************************************************************/
#define	fanCountListLen	(4u+(1u+2u))
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

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


uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;		//��ͣ�ļ���
}



uint16_t	volatile  fan_shut_delay;
void	HCBoxFan_Update( void )
{	//	�������¼ת��Ȧ��
	fanCountList[ fanCountList_index] = HCBoxFan_Circle_Read();
	if ( ++fanCountList_index >= fanCountListLen )
	{
		fanCountList_index = 0u;
	}
	//	���ȿ��ص���̬����
	if ( fan_shut_delay > 0u )
		fan_shut_delay --;
	else
		HCBoxFan_OutCmd( FALSE );
}



void	set_HCBoxTemp( FP32 TempSet, uint8_t ModeSet )
{
	HCBox.SetTemp = TempSet;
	HCBox.SetMode = ModeSet;
}



FP32	get_HCBoxTemp( void )
{
	return	usRegInputBuf[5] * 0.0625;
}



uint16_t	get_HCBoxOutput( void )
{
	return	HCBox.OutValue;
}



uint16_t	get_HCBoxFanSpeed( void )
{
	return	FanSpeed_fetch();
}

/********************************** ����˵�� ***********************************
*  ������ƣ�����ѭ����ʱ���ܣ�
*******************************************************************************/

void	HCBox_Output( FP32 OutValue )
{
	static	volatile	uint16_t	HCBoxOutValue = 0;
	//	�������״̬
	HCBox.OutValue = OutValue * 1000 + 1000;
	
	if      ( HCBox.OutValue < 1000 )
	{
		HCBoxOutValue = ( 1000 - HCBox.OutValue  );
		//	�رռ���
		HCBoxHeat_OutCmd( 0 );
		//	��������
		if      ( HCBoxOutValue > 990 )
		{
			HCBoxCool_OutCmd( 1000 );
		}
		else if ( HCBoxOutValue < 10  )
		{
			HCBoxCool_OutCmd( 0 );
		}
		else
		{	
			HCBoxCool_OutCmd( HCBoxOutValue );
		}
	}
	else if ( HCBox.OutValue > 1000 )
	{
		HCBoxOutValue = HCBox.OutValue - 1000;		
		HCBoxCool_OutCmd( 0 );//	�ر�����
		//	��������
		if      ( HCBoxOutValue >  990 )
		{
			
			HCBoxHeat_OutCmd( 1000 );		}
		else if ( HCBoxOutValue < 10 )
		{                                                
			HCBoxHeat_OutCmd( 0 );
		}
		else
		{
			HCBoxHeat_OutCmd( HCBoxOutValue );
		}
	}
	else
	{
		//	�رռ���
		HCBoxHeat_OutCmd( 0 );
		//	�ر�����
		HCBoxCool_OutCmd( 0 );
	}

	HCBoxFan_Update();			//	��������ת��
}


/********************************** ����˵�� ***********************************
*	�����������乲��һ���¶��źţ����߲���ͬʱʹ�á�
*******************************************************************************/
extern	uint16_t	iRetry;
void	HCBoxTemp_Update( void )
{
	if ( iRetry >= 30 )
		HCBox_Output( 0.0f );	//	ע����ȴ�״̬���������һ��
	set_HCBoxTemp( usRegHoldingBuf[5] * 0.0625f, usRegHoldingBuf[6] );
	HCBox.RunTemp = get_HCBoxTemp();
}

/********************************** ����˵�� ***********************************
*  �ȴ�״̬����ֹ���ȡ�����
*******************************************************************************/
volatile	BOOL	EN_Cool = TRUE;
volatile	BOOL	EN_Heat = TRUE;
BOOL ControlFlag = FALSE;
static	void	HCBox_Wait( void )
{
	//	�����Զ�ģʽ���޷�ȷ��ʵ�ʹ���ģʽ����ʱ����ȴ�״̬
	HCBoxTemp_Update();
	HCBox_Output( 0.0f );	//	�ȴ�״̬���
}



/********************************** ����˵�� ***********************************
*  ����״̬�����ȷ�ʽ����
*******************************************************************************/

static	void	HCBox_Heat( void )  
{
	const	FP32	Kp = 0.0390625f*2;			//  5/128
	const	FP32	Ki = ( Kp / 10.0f );	//	160.0f 
	const	FP32	Kd = ( Kp * 30.0f );	//	80.0f

//	const	FP32	Kp = 0.2;
//	const	FP32	Ki = ( Kp / 100.0f );
//	const	FP32	Kd = ( Kp *  10.0f );

	FP32	TempRun, TempSet;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;

	if( EN_Heat )
	{
		HCBoxTemp_Update();
		//	����PID����������ֵ��һ����[0.0 ��+1.0]��Χ
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp - 2;
		Ek_1 = Ek;
		Ek = ( TempSet - TempRun );
		Up = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.3f ){  Ui = -0.3f; }
		if ( Ui > +0.3f ){  Ui = +0.3f; }//0.25
		Ud = ( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f );
		Upid = ( Up + Ui + Ud );
		if ( Upid <  0.0f ){  Upid = 0.0f; }
		if ( Upid > +1.0f ){  Upid = 1.0f; }

		if( ( HCBox.RunTemp - HCBox.SetTemp ) > 0 ) 
		{
			ControlFlag = FALSE;
			Upid = 0.0f;
		}		
		
		HCBox_Output( Upid );	//	����״̬���������ѭ����ʱ���ܣ�
	}
}
/********************************** ����˵�� ***********************************
*  ����״̬�����䷽ʽ����
*******************************************************************************/

static	void	HCBox_Cool( void )
{
	const	FP32	Kp = 0.1171875f*2;			//	15 / 128
	const	FP32	Ki = ( Kp / 10.0f );		//	240.0f
	const	FP32	Kd = ( Kp * 3.0f );	//	80.0f
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;
	FP32	 TempRun, TempSet;
	
  HCBoxTemp_Update();		//	ʵʱ��ȡ�¶�;  if ( ʧ�� ) ת�����״̬

	//	����PID����������ֵ��һ����[-1.0�� 0.0]��Χ
	if( EN_Cool )
	{
		HCBoxTemp_Update();		//	ʵʱ��ȡ�¶�;  if ( ʧ�� ) ת�����״̬
		//	����PID����������ֵ��һ����[-1.0�� 0.0]��Χ
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp + 1;
		Ek_1 = Ek;
		Ek  = ( TempSet - TempRun );
		Up  = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.60f ){  Ui = -0.60f; }
		if ( Ui > +0.60f ){  Ui = +0.60f; }	//	0.5
		Ud =( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f); 
		Upid = Up + Ui + Ud;
		if ( Upid >  0.0f ){  Upid =  0.0f; }
		if ( Upid < -1.0f ){  Upid = -1.0f; }

		if( ( HCBox.RunTemp - HCBox.SetTemp ) < 0 ) 
		{
			ControlFlag = FALSE;
			Upid = 0.0f;
		}

		//	����������ƣ����䷽ʽ�¿������ȣ��ݲ����٣�2014��1��15�գ�
		if ( Upid < 0.0f )
		{
			fan_shut_delay = 60u;
			HCBoxFan_OutCmd( TRUE );
		}
		//	���
// 		if ( FanSpeed_fetch() < 100u )
// 		{	//	���Ȳ�ת����ֹ����Ƭ����
// 				HCBox_Output( 0.0f );	//	ע����ȴ�״̬���������һ��
// 		}
// 		else
		{
			HCBox_Output( Upid );	//	����״̬���������ѭ����ʱ���ܣ�
		}
	}

}


#include "math.h"
/********************************** ����˵�� ***********************************
*	�������¶ȿ���
*******************************************************************************/
static	uint32_t HCBoxCount = 0;

void	HCBoxControl( void )
{   
	FP32	EK = HCBox.RunTemp - HCBox.SetTemp;
	
	if( HCBoxFlag )
	{
		HCBoxFlag = FALSE;
		HCBox_Wait();
		
		if( !ControlFlag )
		{
			EN_Cool = FALSE;
			EN_Heat = FALSE;
			if( fabs( EK ) > 2 )
				HCBoxCount ++;
			else
				HCBoxCount = 0;
		}
		else
		{	
			HCBoxCount = 0;
			if( EK >  2 )
			{
				EN_Cool = TRUE;
				EN_Heat = FALSE;
			}		
			if( EK < -2 )
			{
				EN_Cool = FALSE;
				EN_Heat = TRUE;
			}		
			switch ( HCBox.SetMode )
			{
			case MD_Auto:
				if( EN_Cool )
					HCBox_Cool();
				if( EN_Heat )
					HCBox_Heat();
				break;
			case MD_Cool:	EN_Cool = TRUE; EN_Heat = FALSE;	HCBox_Cool();	break;
			case MD_Heat:	EN_Heat = TRUE; EN_Cool = FALSE;	HCBox_Heat();	break;
			default:
			case MD_Shut:	EN_Heat = EN_Cool = FALSE;				HCBox_Wait();	break;							
			}			
		}
		
		if( HCBoxCount >= 60 * 30 )
			ControlFlag = TRUE;
	}	
}








/********  (C) COPYRIGHT 2014 �ൺ���˴���ӿƼ����޹�˾  **** End Of File ****/





// 			//	����¶�ƫ���2'C��ά��һ��ʱ�䣨30min��, �л�������ʽ
// 			if ( Ek > -1.5f )
// 			{
// 				shutcount_Cool = 0u;
// 			}
// 			else if ( shutcount_Cool < ( 60u * 1 ))//30u
// 			{
// 				++shutcount_Cool;
// 			}
// 			else
// 			{
// 				EN_Cool = FALSE;
// 				EN_Heat = TRUE;
// 			}







































#if 0

// ʹ��TIMx��CH1�Ĳ����ܣ���DMA��¼���������.
#define	CMR_Len	10
static	uint16_t	CMRA[CMR_Len];

// void	CMR1( void )
// {
// 	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel6;
// 	TIM_TypeDef * TIMx = TIM16;
// 	
// 	//	DMA1 channel1 configuration
// 	SET_BIT ( RCC->AHBENR,  RCC_AHBENR_DMA1EN );
// 	//	DMAģ�����, ��������
// 	DMA1_Channelx->CCR  = 0u;
// 	DMA1_Channelx->CCR  = DMA_CCR6_PL_0						//	ͨ�����ȼ���01 �е�
// 						| DMA_CCR6_PSIZE_0					//	�ڴ�����λ��01 16λ
// 						| DMA_CCR6_MSIZE_0					//	��������λ��01 16λ
// 						| DMA_CCR6_MINC						//	����ģʽ���ڴ�����
// 						| DMA_CCR6_CIRC						//	ѭ�����䣺ʹ��ѭ��
// 					//	| DMA_CCR6_DIR						//	���ͷ��򣺴������
// 						;
// 	DMA1_Channelx->CPAR  = (uint32_t) &TIM16->CCR1;			//	����DMA�����ַ
// 	DMA1_Channelx->CMAR  = (uint32_t) CMRA;					//	�ڴ��ַ
// 	DMA1_Channelx->CNDTR = CMR_Len;							//	��������
// 	SET_BIT ( DMA1_Channelx->CCR, DMA_CCR1_EN );			//	ʹ��DMAͨ��

// 	//	����TIMx �������벶��
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_TIM16EN );
// 	TIMx->CR1   = 0u;
// 	TIMx->CR2   = 0u;
// 	TIMx->CCER  = 0u;
// 	TIMx->CCMR1 = 0u;
// 	TIMx->CCMR2 = 0u;
// 	//	TIMx ʱ����ʼ��: ����ʱ��Ƶ��24MHz����Ƶ��1MHz�����롣
// 	//	ʱ���������Բ���������ٶ�������ٶȡ�
// 	TIMx->PSC = 240u - 1;	//	10us @ 24MHz
// 	TIMx->ARR = 0xFFFFu;
// 	TIMx->EGR = TIM_EGR_UG;
// 	
// 	TIMx->CCMR1 = TIM_CCMR1_CC1S_0					//	CC1S  : 01b   IC1 ӳ�䵽IT1�ϡ�
// 				| TIM_CCMR1_IC1F_1|TIM_CCMR1_IC1F_0	//	IC1F  : 0011b ���������˲�����8����ʱ��ʱ�������˲�
// 				| TIM_CCMR1_IC2PSC_1				//	IC1PSC: 01b   ���������Ƶ��ÿ��2���¼�����һ�β���
// 				;
// 	TIMx->CCER  = TIM_CCER_CC1E						//	���� CCR1 ִ�в���
// 				| TIM_CCER_CC1P						//	������CCR1�����ź����ڡ�
// 				;
// 	TIMx->DIER  = TIM_DIER_CC1DE;

// 	TIMx->CR1   = TIM_CR1_CEN;						//	ʹ�ܶ�ʱ��

// 	//	���ùܽţ�PA.6 ��������
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
// 	MODIFY_REG( GPIOA->CRL, 0x0F000000u, 0x04000000u );
// }

uint16_t	fetchSpeed( void )
{	//	ȡ DMA ���� �� �ڴ��ַָ�룬��������ǰ�������Ρ�
	//	���DMA���� �� �ڴ�ָ�붼�����ã�ȡN�εĲ�ֵ�����������ֵ����Сֵ��
	
	/*	�̶����1s��¼����ת��Ȧ������������
	 *	���μ����������˲��Ľ��������ת�١�
	 */
	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel6;
	uint8_t 	ii, index;
	uint16_t	sum = 0u;
//	uint16_t	max = 0u;
//	uint16_t	min = 0xFFFFu;
	uint16_t	x0, x1, period;

	index = ( DMA1_Channelx->CMAR - ( uint32_t ) CMRA ) / sizeof( uint16_t);	//	�ڴ��ַ
	if ( ++index >= CMR_Len ){  index = 0u; }
	if ( ++index >= CMR_Len ){  index = 0u; }
	
	x1 = CMRA[index];
	for ( ii = CMR_Len - 2u; ii != 0; --ii )
	{
		//	�����������õ��ٶ�
		x0 = x1;
		if ( ++index >= CMR_Len ){  index = 0u; }
		x1 = CMRA[index];
		period = (uint16_t)( x1 - x0 );
		//	�Զ�����ݽ����˲�
//		if ( period > max ) {  max = period; }
//		if ( period < min ) {  min = period; }
//		sum += period;
	}
	period = sum / ( CMR_Len - 2u );
//	period = (uint16_t)( sum - max - min ) / ( CMR_Len - (1u+2u));

	if ( period == 0u )
	{
		return	0xFFFFu;
	}
	else
	{	//	ÿ���ӵļ������� / ÿ������ļ���ʱ�� => ÿ���ӵ�ת��
		return	(( 60u * 100000u ) / period );
	}
}

#endif
// iCount;


// _isr_t( void )

// {
// 	static uint8_t iPWM =0;
// 	iCount ++;
// 	if ( (t1 - t0) > 1000 )
// 	//	t0 += 1000;
// 	(t0 = t1;
// 	{
// 		
// 	}
// 	
// 	
// 	++iPWM;
// 	if (iPWM>= 100 )
// 	{
// 		iPWM = 0;
// 	}
// 	
// 	Heat = iPWM>out
// 	
// }
// void heat( void )
// {
// 	static t0;// = iCount;
// 	t1 = iCount;
// 	if ( (t1 - t0) > 1000 )
// 	//	t0 += 1000;
// 	(t0 = t1;
// 		
// 	{
// 		pid = ???
// 		out = pid;		
// 	}
// 	
// 	
// }

