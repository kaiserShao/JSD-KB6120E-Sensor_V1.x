/**************** (C) COPYRIGHT 2013 �ൺ���˴���ӿƼ����޹�˾ ****************
* �� �� ��: FM24.C
* �� �� ��: Dean
* ��  ��  : ��д24ϵ�е� FRAM ����
*         : 
* ����޸�: 2015��7��13��
*********************************** �޶���¼ ***********************************
* ��  ��: 
* �޶���: 
*******************************************************************************/

#include "BSP.H"

#define	_SLAVE_FM24C64	0xA0

/*******************************************************************************
* ��������: FM24C64 Save/Load
* ����˵��: FM24C64 ��ȡ
* �������: ��ַ/ָ��/���ݳ���
* �������: None
* �� �� ֵ: ���سɹ���־
*******************************************************************************/
bool	FM24C64_Save( uint16_t address, const uint8_t * buffer, uint16_t NBytes )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Write ))	{ bus_i2c_stop(); return FALSE; }
    if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
    if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// continue send write data.
    do
	{
		if ( ! bus_i2c_shout( *buffer++ )){	bus_i2c_stop(); return FALSE; 	}
	}
	while ( --NBytes );

    bus_i2c_stop();

	/*	����洢������Ҫ polling */
	return	TRUE;
}

bool	FM24C64_Load( uint16_t address, uint8_t * buffer, uint16_t NBytes )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Write ))	{ bus_i2c_stop(); return FALSE; }
	if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
	if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// Send read command and receive data.
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Read ))	{ bus_i2c_stop(); return FALSE; }
	while ( --NBytes )
	{
		*buffer++ = bus_i2c_shin( I2C_ACK );	// Receive and send ACK
	}
	*buffer = bus_i2c_shin( I2C_NoACK );		// Receive and send NoACK
	bus_i2c_stop();

	return TRUE;
}

/********  (C) COPYRIGHT 2015 �ൺ���˴���ӿƼ����޹�˾  **** End Of File ****/