#include "BSP.H"

extern	void	MODBUS_Init( void );
extern	uint16_t	usRegInputBuf[];
extern	uint16_t	usRegHoldingBuf[];
extern	uint8_t		ucRegDiscBuf[];
extern	uint8_t		ucRegCoilsBuf[];

//	BOOL	Flash_InPage_Save( uint16_t const * Buffer, uint16_t Count );
//	BOOL	Flash_InPage_Load( uint16_t       * Buffer, uint16_t Count );

// 	extern	void	PWM1_OutputSet( uint16_t OutValue );
// 	extern	void	DAC1_OutputSet( uint16_t OutValue );
// 	extern	void	HVPower_OutCmd( BOOL NewState );
#define	filter_length	128
uint16_t	filter_array[filter_length];
uint16_t	filter_index = 0u;
uint16_t	filter( uint16_t NewValue )
{
	uint32_t	sum;
	uint16_t	i;

	filter_array[filter_index] = NewValue;
	if ( ++ filter_index >= filter_length )
	{
		filter_index = 0u;
	}
	
	sum = 0u;
	for ( i = 0u; i < filter_length; ++i )
	{
		sum += filter_array[i];
	}
	
	return	( sum / filter_length );
}


int32_t	main( void )
{
	//	����������ʼ��
	BIOS_Init();

	usRegHoldingBuf[0] = 0u;
	usRegHoldingBuf[1] = 0u;
	usRegHoldingBuf[2] = 0u;
	usRegHoldingBuf[3] = 0u;
	usRegHoldingBuf[4] = 0u;
	usRegHoldingBuf[5] = 0u;
	usRegHoldingBuf[6] = 0u;
	usRegHoldingBuf[7] = 0u;
	usRegHoldingBuf[8] = 0u;
	usRegHoldingBuf[9] = 0u;
	//	��ȡ�ڲ�EEPROM
	Eload( 0x0000u, & usRegHoldingBuf[10], 60u );	
//	Flash_InPage_Load( & usRegHoldingBuf[10], 30u );
	Initialize_AD7705();
	usRegInputBuf[0] = 0u;
	//	��ʼ��MODBUSЭ��ջ
	MODBUS_Init();
	
	//	���Ź�����
	for(;;)
	{
		//	���Ź�����

		++usRegInputBuf[0];		//	�����������ʾ�ӻ���������

		//	�����Լ�LED
		if ( usRegHoldingBuf[0] )
		{
			PWM1_OutputSet( usRegHoldingBuf[0] );	//	���PWM
		}
		else
		{
			PWM1_OutputSet( 0u );					//	�ر�PWM
		}

		//	������ѹ������ѹ
		usRegInputBuf[1] = Readout_ADC1_6();

		//	���Ƹ�ѹ
		if ( usRegHoldingBuf[1] )
		{	
			DAC1_OutputSet( usRegHoldingBuf[1] );
			HVPower_OutCmd( TRUE );					//	�򿪸�ѹ
		}
		else
		{
			DAC1_OutputSet( 0u );
			HVPower_OutCmd( FALSE );				//	�رո�ѹ
		}
		
		//	ȡ��AD7705�Ķ���
		if ( usRegHoldingBuf[2] )
		{
			usRegInputBuf[2] = filter( Readout_AD7705( usRegHoldingBuf[2] ));
		}

		if ( 0xFF00 == usRegHoldingBuf[9] )	//	���󱣴�E������ ��
		{	//	�����������ݵ��ڲ�EEPROM
			Esave( 0x0000u, & usRegHoldingBuf[10], 60u );	
		//	Flash_InPage_Save( & usRegHoldingBuf[10], 30u );
			//	������ɺ󣬳��������־
			usRegHoldingBuf[9] = 0u;
		}
	}
}