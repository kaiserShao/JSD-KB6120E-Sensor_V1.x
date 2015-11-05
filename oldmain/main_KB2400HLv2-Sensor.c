#include "BSP.H"
#include <math.h>
#include <string.h>

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
static	void	Fan2_Exec( void )
{
	static	bool	OutState = false;
	
	FP32	Temp = get_NTC2_Temp();
	
	//	����40�濪������40��ء�
	if ( Temp > 40.5f ){  OutState = true;  }
	if ( Temp < 39.5f ){  OutState = false; }
	
	Fan2_OutCmd( OutState );
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
//	ʱ��Ƭ��0������ʱ�価�����ִ��
///////////////////////////////////////////////////
static	void	Slice0_Exec( void )
{
	bool	Protect_EN;
	bool	Output_EN;
	
	Protect_EN = MonitorTickTimeout();	//	���Ӷ�ʱ����ʱ�������ԵĹر��������
	//	�۳���
// 	Output_EN = Read_BitN( ucRegCoilsBuf, 15 );	// �������?
// 	if ( ! Protect_EN && Output_EN )
// 	{
// 		Fan1_OutCmd( true );
// 		MotorPower_OutCmd( true );
// 		MotorPWM_SetOutput( usRegHoldingBuf[15] );
// 	}
// 	else
// 	{
// 		MotorPWM_SetOutput( 0u );
// 		MotorPower_OutCmd( false );
// 		Fan1_OutCmd( false );
// 	}

	//	ʱ��C
	Output_EN = Read_BitN( ucRegCoilsBuf, 30 );	// �������?
	if ( ! Protect_EN && Output_EN )
	{
		PWM1_SetOutput( usRegHoldingBuf[30] );
	}
	else
	{
		PWM1_SetOutput( 0u );
	}

	//	ʱ��D
	Output_EN = Read_BitN( ucRegCoilsBuf, 35 );	// �������?
	if ( ! Protect_EN && Output_EN )
	{
		PWM2_SetOutput( usRegHoldingBuf[35] );
	}
	else
	{
		PWM2_SetOutput( 0u );
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
// 		usRegInputBuf[17] = Temp16S;	//	��ǰ�¶ȣ��۳���
		usRegInputBuf[32] = Temp16S;	//	��ǰ�¶ȣ�ʱ��C��
		usRegInputBuf[37] = Temp16S;	//	��ǰ�¶ȣ�ʱ��D��	
	}

	Heater_Exec();		//	�������¶ȸ��£��������¶ȿ���

	//	����
	usRegInputBuf[22] = rint ( 16.0f * get_NTC1_Temp());	//	NTC1 �¶�
	usRegInputBuf[27] = rint ( 16.0f * get_NTC2_Temp());	//	NTC2 �¶�
}

///////////////////////////////////////////////////
//	ʱ��Ƭ��1��AD7705 �л��� CH2 ʱִ��
///////////////////////////////////////////////////
static	void	Slice2_Exec( void )
{
	usRegInputBuf[3] = rint ( 16.0f * get_NTC1_Temp());	//	����¶�

	Fan2_Exec();			//	��Դ�¶ȸ��£���Դɢ�ȷ��ȿ���
	
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

	for ( cnt = 0u; cnt < 8u; ++cnt )
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
			usRegInputBuf[25 + ( i * 5 )] = mean;	//	�װ��ѹ
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
		Slice0_Exec();	//	ʱ��Ƭ��0������ʱ�価�����ִ��
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

				usRegInputBuf[26+ ( i * 5 )] = mean;	//	��ǰѹ��
			}
		}
	}
}

int32_t	main( void )
{
	int16_t		Temp16S;
	uint16_t	i;
	
	//	����������ʼ��
	BIOS_Init( );
	DS18B20_1_Read( &Temp16S );		//	��18B20, ���� 0x0550
	DS18B20_2_Read( &Temp16S );
	DS18B20_3_Read( &Temp16S );
	Initialize7705( );

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
	
	// Set_BitN( ucRegDiscBuf, 1 );	//	����ѹ�� ����
	Set_BitN( ucRegDiscBuf, 2 );	//	�����¶� ����
	Set_BitN( ucRegDiscBuf, 3 );	//	����¶� ����
	Set_BitN( ucRegDiscBuf, 5 );	//	������ ����
// 	Set_BitN( ucRegDiscBuf, 8 );	//	������ ����
// 	Set_BitN( ucRegDiscBuf, 10 );	//	������ ����
// 	Set_BitN( ucRegDiscBuf, 15 );	//	�۳��� �����ź� ����
// 	Set_BitN( ucRegDiscBuf, 16 );	//	       ��ǰѹ�� ��Ч
// 	Set_BitN( ucRegDiscBuf, 17 );	//	       ��ǰ�¶� ��Ч
	Set_BitN( ucRegDiscBuf, 30 );	//	ʱ��C  �����ź� ����
	Set_BitN( ucRegDiscBuf, 31 );	//	       ��ǰѹ�� ��Ч
	Set_BitN( ucRegDiscBuf, 32 );	//	       ��ǰ�¶� ��Ч
	Set_BitN( ucRegDiscBuf, 35 );	//	ʱ��D  �����ź� ����
	Set_BitN( ucRegDiscBuf, 36 );	//	       ��ǰѹ�� ��Ч
	Set_BitN( ucRegDiscBuf, 37 );	//	       ��ǰ�¶� ��Ч
	
	//	��ʼ��MODBUSЭ��ջ
	MODBUS_Init( 1 );
	
	//	���Ź�����
	//	InitWDT();
	for(;;)
	{
		//	�����������ʾϵͳ����������
		++usRegInputBuf[0];

		Update_CH0( );

		Update_CH1( );

		//	���Ź�����
		//	ClearWDT();
	}
}

/********  (C) COPYRIGHT 2015 �ൺ���˴���ӿƼ����޹�˾  **** End Of File ****/
