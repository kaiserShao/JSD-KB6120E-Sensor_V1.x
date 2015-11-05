#include "BSP.H"
#include <math.h>
#include <string.h>
#include "Pin.H"
#define	Read_BitN(_b_buf, _n)	(_b_buf[(_n)/8] & ( 1 << (( _n ) % 8 )))
#define	Set_BitN(_b_buf, _n)	(_b_buf[(_n)/8] |= ( 1 << (( _n ) % 8 )))
#define	Clean_BitN(_b_buf, _n)	(_b_buf[(_n)/8] &= ~( 1 << (( _n ) % 8 )))

extern	uint16_t	usRegInputBuf[];
extern	uint16_t	usRegHoldingBuf[];
extern	uint8_t		ucRegDiscBuf[];
extern	uint8_t		ucRegCoilsBuf[];

///////////////////////////////////////////////////
//	�����¶��Զ����Ʒ��ȵ���ͣ��
///////////////////////////////////////////////////
static	void	Fan1_Exec( void )
{
	static	bool	OutState = false;
	
	FP32	Temp = get_NTC1_Temp();	//��Դ
	
	//	����40�濪������40��ء�
	if ( Temp > 40.5f ){  OutState = true;  }
	if ( Temp < 39.5f ){  OutState = false; }
	
	Fan1_OutCmd( OutState );
}
///////////////////////////////////////////////////
//	�������¶ȿ���
///////////////////////////////////////////////////
static	void	Heater_Exec( void )
{
	static	bool	OutState = false;
	static	uint16_t	iRetry = 0u;
	int16_t	Temp16S;
	int16_t	Temp_Set;
	bool	Heater_EN;

	if ( DS18B20_2_Read( &Temp16S ))
	{
		iRetry = 0u;

		usRegInputBuf[8]  = Temp16S;

		Temp_Set = (int16_t)usRegHoldingBuf[8];			//	�趨�¶�

		if (( Temp_Set > ( 55 * 16 )) || ( Temp_Set < ( 15 * 16 )))
		{
			Heater_EN = false;	//	�趨�¶Ȳ������Զ���ֹ������������
		}
		else
		{
			Heater_EN = Read_BitN( ucRegCoilsBuf, 8 );	//	�����¿�
		}

		if ( Heater_EN )
		{	
			//	�����¶��Զ����Ƽ������ļ��ȡ�
			OutState = ( Temp16S < Temp_Set );
		}
		else
		{
			OutState = false;	//	��ֹ������������
		}
	}
	else
	{
		if ( iRetry < 30u )
		{
			++iRetry;
		}
		else
		{
			OutState = false;	//	��ȡ�¶ȴ�����ʧ�ܣ���ֹ�����������������λ����
		}
	}
	
	Heater_OutCmd( OutState );
	usRegInputBuf[9] = OutState ? 1000u : 0u;
}	

///////////////////////////////////////////////////
//	�������¶ȿ���
///////////////////////////////////////////////////
uint16_t volatile fan_shut_delay;
uint16_t	iRetry = 0u;
static	void	HCBox_Exec( void )
{	
	int16_t	Temp16S;
	if ( DS18B20_4_Read( &Temp16S ))
	{
		iRetry = 0u;
		usRegInputBuf[5]  = Temp16S;
	}
	else
	{
		if ( iRetry < 30u )
		{
			++iRetry;
		}
	}
	usRegInputBuf[6] = get_HCBoxOutput();
	usRegInputBuf[7] = get_HCBoxFanSpeed();	
}
///////////////////////////////////////////////////
//	ʱ��Ƭ��0������ʱ�価�����ִ��
///////////////////////////////////////////////////
static	void	Slice0_Exec( void )
{
	bool	Protect_EN;
	bool	Output_EN;
	
	Protect_EN = MonitorTickTimeout();	//	���Ӷ�ʱ����ʱ�������ԵĹر��������
	HCBoxControl();
	Output_EN = Read_BitN( ucRegCoilsBuf, 15 ) |
                Read_BitN( ucRegCoilsBuf, 20 ) | 
                Read_BitN( ucRegCoilsBuf, 25 ) | 
                Read_BitN( ucRegCoilsBuf, 30 ) | 
                Read_BitN( ucRegCoilsBuf, 35 ) ;	// �������?
	PowerAIR_OutCmd( Output_EN );
	
	//ת�Ӵ���AIR
	Output_EN = Read_BitN( ucRegCoilsBuf, 10 );	// �������?
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 10 ) )
		PowerAIR_OutCmd( true );
	else 
	if( Read_BitN( ucRegDiscBuf, 10 ) )
		PowerAIR_OutCmd( false );
	
	//	�۳���
	Output_EN = Read_BitN( ucRegCoilsBuf, 15 );	// �������?
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 15 ) )
	{
		Fan2_OutCmd( true );
		MotorPower_OutCmd( true );
		MotorPWM_SetOutput( usRegHoldingBuf[15] );
	}
	else
	{
		MotorPWM_SetOutput( 0u );
		MotorPower_OutCmd( false );
		Fan2_OutCmd( false );
	}

//	�վ�A
	Output_EN = Read_BitN( ucRegCoilsBuf, 20 );
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 20 ) )
	{
		PWM1_SetOutput( usRegHoldingBuf[20] );
	}
	else
	{
		PWM1_SetOutput( 0u );
	}

	//	�վ�B
	Output_EN = Read_BitN( ucRegCoilsBuf, 25 );
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 25 ) )
	{
		PWM2_SetOutput( usRegHoldingBuf[25] );
	}
	else
	{
		PWM2_SetOutput( 0u );
	}
	
	//	ʱ��C	
	Output_EN = Read_BitN( ucRegCoilsBuf, 30 );
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 30 ) )
	{
		PWM3_SetOutput( usRegHoldingBuf[30] );
	}
	else
	{
		PWM3_SetOutput( 0u );
	}

	//	ʱ��D
	Output_EN = Read_BitN( ucRegCoilsBuf, 35 );
	if ( ! Protect_EN && Output_EN && Read_BitN( ucRegDiscBuf, 35 ) )
	{
		PWM4_SetOutput( usRegHoldingBuf[35] );
	}
	else
	{
		PWM4_SetOutput( 0u );
	}
}

///////////////////////////////////////////////////
//	ʱ��Ƭ��1��AD7705 �л��� CH0 ʱִ��
///////////////////////////////////////////////////
static	void	Slice1_Exec( void )
{
	int16_t	Temp16S;

	if ( DS18B20_1_Read( &Temp16S ))
	{
        usRegInputBuf[2]  = Temp16S;	//	�����¶�
	}
	if ( DS18B20_3_Read( &Temp16S ))
	{	
 		usRegInputBuf[17] = Temp16S;	//	��ǰ�¶ȣ��۳���
		
		usRegInputBuf[22] = Temp16S;	//	��ǰ�¶ȣ��վ�A��
		usRegInputBuf[27] = Temp16S;	//	��ǰ�¶ȣ��վ�B��	
 		
		usRegInputBuf[32] = Temp16S;	//	��ǰ�¶ȣ�ʱ��C��
		usRegInputBuf[37] = Temp16S;	//	��ǰ�¶ȣ�ʱ��D��	
	}
	
	if( Read_BitN( ucRegDiscBuf, 5 ) )
		HCBox_Exec();			//	�������¶ȸ��£��������¶ȿ���
	if( Read_BitN( ucRegDiscBuf, 8 ) )
		Heater_Exec();		//	�������¶ȸ��£��������¶ȿ���
	HCBoxControl();

}

///////////////////////////////////////////////////
//	ʱ��Ƭ��2��AD7705 �л��� CH2 ʱִ��
///////////////////////////////////////////////////
static	void	Slice2_Exec( void )
{
	usRegInputBuf[3] = rint ( 16.0f * get_NTC2_Temp());	//	����¶�
	HCBoxControl();
	Fan1_Exec();			//	��Դ�¶ȸ��£���Դɢ�ȷ��ȿ���
	
}

///////////////////////////////////////////////////
//	ת��/��ȡ AD7705 ͨ�� CH0
///////////////////////////////////////////////////
void	Update_CH0(void )
{
	uint32_t	Sum7705_CH0[CS7705_Max];
	uint8_t		isExist7705[CS7705_Max];
	uint16_t	i, cnt;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		isExist7705[i] = Convert7705 ((enum enumCS7705)i, 0u );
		Sum7705_CH0[i] = 0ul;
	}

	///////////////////////////////////////////////////
	Slice1_Exec();		//	ʱ��Ƭ��1
	///////////////////////////////////////////////////

	for ( cnt = 0u; cnt < 4u; ++cnt )
	{
		///////////////////////////////////////////////////
		Slice0_Exec();	//	ʱ��Ƭ��0������ʱ�価�����ִ��
		///////////////////////////////////////////////////

		for ( i = 0u; i < CS7705_Max; ++i )
		{
			if ( isExist7705[i] )
			{
				Sum7705_CH0[i] += Readout7705 ((enum enumCS7705)i, 0u );
			}
		}
	}
	
	//	ͨ�� CH0 �Ľ��������β�����ƽ��ֵ
	for ( i = 0u; i < CS7705_Max; ++i )
	{
		if ( isExist7705[i] )
		{
			uint16_t mean = Sum7705_CH0[i] / cnt;
			usRegInputBuf[15 + ( i * 5 )] = mean;	//	�װ��ѹ
		}
	}
}

///////////////////////////////////////////////////
//	ת��/��ȡ AD7705 ͨ�� CH1
///////////////////////////////////////////////////
void	Update_CH1( void )
{
	static	uint32_t	Sum7705_CH1[CS7705_Max][4];
	uint8_t		isExist7705[CS7705_Max];
	uint16_t	i, cnt;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		isExist7705[i] = Convert7705 ((enum enumCS7705)i, 1u );
	}
	
	///////////////////////////////////////////////////
	//	ʱ��Ƭ��2
	///////////////////////////////////////////////////
	Slice2_Exec();

	for ( cnt = 0u; cnt < 2u; ++cnt )
	{
		///////////////////////////////////////////////////
		Slice0_Exec(); //	ʱ��Ƭ��0������ʱ�価�����ִ��
		///////////////////////////////////////////////////
		
		for ( i = 0u; i < CS7705_Max; ++i )
		{
			if ( isExist7705[i] )
			{
				uint16_t	mean;

				Sum7705_CH1[i][0] = Sum7705_CH1[i][1];
				Sum7705_CH1[i][1] = Sum7705_CH1[i][2];
				Sum7705_CH1[i][2] = Sum7705_CH1[i][3];
				Sum7705_CH1[i][3] = Readout7705 ((enum enumCS7705)i, 1u );
				
				mean = ((uint32_t)Sum7705_CH1[i][0] + Sum7705_CH1[i][1] + Sum7705_CH1[i][2] + Sum7705_CH1[i][3] ) / 4u;

				usRegInputBuf[16 + ( i * 5 )] = mean;	//	��ǰѹ��
			}
		}
	}
}
/*
	KB6120E_�����Զ�����
*/
static void KB6120E_ConfigSelect( void )
{
	uint8_t		isExist7705[CS7705_Max];
	uint8_t	i;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		isExist7705[i] = Convert7705 ((enum enumCS7705)i, 0u );
	}
	for ( i = 0u; i < CS7705_Max; ++i )
	{
		if ( isExist7705[i] )
		{
			 Set_BitN( ucRegDiscBuf, 15 + i * 5  );
		}
	}
	if	( Read_BitN(ucRegDiscBuf, 15)  && 
        (!Read_BitN(ucRegDiscBuf, 20)) && 
        (!Read_BitN(ucRegDiscBuf, 25)) && 
        (!Read_BitN(ucRegDiscBuf, 30)) && 
        (!Read_BitN(ucRegDiscBuf, 35)) )
		Set_BitN( ucRegDiscBuf, 10 );
}
		

int32_t	main( void )
{
	int16_t		Temp16S;
	uint16_t	i;	

	BIOS_Init( );			//	����������ʼ��
	//	��ʼ��ͨ�Ż�����
	for ( i = 0; i < 40u; ++i )
	{
		usRegHoldingBuf[i] = 0u;
		usRegInputBuf[i]   = 0u;
	}
	for ( i = 0; i < (( 40u + 7 ) / 8 ); ++i )
	{
		ucRegCoilsBuf[i] = 0u;
		ucRegDiscBuf[i] = 0u;
	}

	DS18B20_1_Read( &Temp16S );		//	��18B20, ���� 0x0550 �����¶�
	
	if( DS18B20_2_Read( &Temp16S  ))  
		Set_BitN( ucRegDiscBuf, 8 );	//	�������¶�
	
	DS18B20_3_Read( &Temp16S );		//	��ǰ�¶�
	
	if( DS18B20_4_Read( &Temp16S ) )	
		Set_BitN( ucRegDiscBuf, 5 );	//	�������¶�
	
	Initialize7705( );						//	��ѹ��ѹ��ʼ��

	//	�����Զ�����
	KB6120E_ConfigSelect();			
	
	//	��ʼ��MODBUSЭ��ջ
	MODBUS_Init( 1 );
	
	//	���Ź�����
	//	InitWDT();
	for(;;)
	{
		//	�����������ʾϵͳͨ��������
		++usRegInputBuf[0];
		
		Update_CH0( );

		Update_CH1( );
		
		//	���Ź�����
		//	ClearWDT();
	}
}
/********  (C) COPYRIGHT 2015 �ൺ���˴���ӿƼ����޹�˾  **** End Of File ****/
