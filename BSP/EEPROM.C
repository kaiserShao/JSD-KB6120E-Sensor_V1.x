/**
* �� �� ��: EE24.C
* �� �� ��: Dean
* ��  ��  : ��д24ϵ�е�EEPROM����
*         : 
* ����޸�: 2012��4��14��
*********************************** �޶���¼ ***********************************
* ��  ��: 
* �޶���: 
*******************************************************************************/

#include "BSP.H"
#include "Pin.H"
#include <absacc.h>

BOOL	STM32_Flash_Save( uint16_t VirtualAddress, void const * Buffer, uint16_t Count );
BOOL	STM32_Flash_Load( uint16_t VirtualAddress, void       * Buffer, uint16_t Count );


/**
  *	����ԭ��д��һ���հ�ҳ��У��ɹ��󣬲�����һҳ�������������һҳ�������ݵģ�
  */

#define	EBASE_PAGE1		(( uint16_t * )0x08003C00u)
//	#define	EBASE_PAGE1		(( uint16_t * )0x08003800u)
const	uint16_t	EBASE_PAGE0[1024]		__at(0x08003800u);
//	uint16_t	__attribute__((section(".ARM.__at_0x8000")));
//#define	EBASE_PAGE1	(( uint16_t * )EBASE_PAGE_A1)
#define FLASH_KEY1      0x45670123u
#define FLASH_KEY2		0xCDEF89ABu

#define	EraseTimeout	10000u
#define	ProgramTimeout	1000u

/**
  * @brief  Waits for a Flash operation to complete or a TIMEOUT to occur.
  * @param  Timeout: FLASH progamming Timeout
  * @retval FLASH Status: The returned value can be: FLASH_ERROR_PG,
  *   FLASH_ERROR_WRP, FLASH_COMPLETE or FLASH_TIMEOUT.
  */
static	BOOL	Flash_isBusy( uint32_t iRetry )
{ 
	while (( 0u != iRetry ) && READ_BIT( FLASH->SR, FLASH_SR_BSY ))
	{
		delay_us( 1u );
		--iRetry;
	}
	
	if ( READ_BIT( FLASH->SR, FLASH_SR_BSY ))
	{
		return	TRUE;
	}
	else
	{
		return	FALSE;
	}
}

/**
  * @brief  Unlocks the FLASH Program Erase Controller.
  * @param  None
  * @retval None
  */
static	void Flash_Unlock(void)
{
	/* Authorize the FPEC Access */
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;
}

/**
  * @brief  Locks the FLASH Program Erase Controller.
  * @param  None
  * @retval None
  */
static	void Flash_Lock(void)
{
	/* Set the Lock Bit to lock the FPEC and the FCR */
	SET_BIT( FLASH->CR, FLASH_CR_LOCK );
}


static	BOOL	Flash_Page_Erase( uint16_t const * EPAGE_BASE )
{
	if ( Flash_isBusy( EraseTimeout )){ return	FALSE; }
	
	SET_BIT( FLASH->CR, FLASH_CR_PER );
	FLASH->AR = (uint32_t)EPAGE_BASE; 
	SET_BIT( FLASH->CR, FLASH_CR_STRT );

	if ( Flash_isBusy( EraseTimeout )){	return	FALSE; }
	
	CLEAR_BIT( FLASH->CR, FLASH_CR_PER );

	return	TRUE;
}

//	ע����֣�16λ������
static	BOOL	Flash_Page_Write(  uint16_t const * EPAGE_BASE, uint16_t Offset, void const * Buffer, uint16_t Count )
{
	uint16_t Data16;
	uint16_t * dst;
	uint8_t  const * src; 
	uint16_t i;

	if ( Flash_isBusy( ProgramTimeout )){  return  FALSE; }

	SET_BIT( FLASH->CR, FLASH_CR_PG );

	src = Buffer;
	dst = ( void * )EPAGE_BASE;
	dst += Offset;
	i = 0u;

	if ((uint32_t)dst & 0x00000001u )
	{	//	Ӳ��Ҫ����뵽16λ
		dst = (uint16_t * )((uint32_t)dst & (~0x00000001u ));
		Data16 = *dst;
		Data16 <<= 8u;
		Data16 |= *src++;
		*dst++ = Data16;
		if ( Flash_isBusy( ProgramTimeout )){  return  FALSE; }
		
		i += 1u;
	}

	while ( i < Count )
	{	//	���ܻ᲻�ɱ���Ķ�д��8λ����
		Data16 = *src++;
		Data16 <<= 8u;
		Data16 |= *src++;
		*dst++ = Data16;
		if ( Flash_isBusy( ProgramTimeout )){  return  FALSE; }
		
		i += 2u;
	}

    CLEAR_BIT( FLASH->CR, FLASH_CR_PG );
    return	TRUE;
}

static	BOOL	Flash_Page_Read(  uint16_t const * EPAGE_BASE, uint16_t Offset, void * Buffer, uint16_t Count )
{
	uint8_t * dst;
	uint8_t const * src; 
	uint16_t i;

	dst = Buffer;
	src = ( void * )EPAGE_BASE;
	src += Offset;
	i = 0; 
	while ( i < Count )
	{
		*dst++ = *src++;
		++i;
	}

	return	TRUE;
}

static	BOOL	Flash_Page_Check( uint16_t const * EPAGE_BASE )
{
	uint8_t const * p = ( void const * )EPAGE_BASE;
	uint8_t chksum = 0u;
	uint16_t len = 1024u;
	while ( len-- )
	{
		chksum += * p++;
	}
	
	chksum &= 0xFFu;
	
	return	0u == chksum;
}

BOOL	STM32_Flash_Save( uint16_t VirtualAddress, void const * Buffer, uint16_t Count )
{
	if ( ! Flash_Page_Check( EBASE_PAGE0 ))
	{
		Flash_Unlock();
		Flash_Page_Write( EBASE_PAGE0, VirtualAddress, Buffer, Count );
		if ( Flash_Page_Check( EBASE_PAGE0 ))
		{
			Flash_Page_Erase( EBASE_PAGE1 );
			Flash_Lock();
			return	TRUE;
		}
		else
		{
			;	//	У��ʧ�ܣ�������ǰ����
			Flash_Lock();
			return	FALSE;
		}
	}
	else
	{
		Flash_Unlock();
		Flash_Page_Write( EBASE_PAGE1, VirtualAddress, Buffer, Count );
		if ( Flash_Page_Check( EBASE_PAGE1 ))
		{
			Flash_Page_Erase( EBASE_PAGE0 );
			Flash_Lock();
			return	TRUE;
		}
		else
		{
			;	//	У��ʧ�ܣ�������ǰ����
			Flash_Lock();
			return	FALSE;
		}
	}
}

BOOL	STM32_Flash_Load( uint16_t VirtualAddress, void       * Buffer, uint16_t Count )
{
	if ( Flash_Page_Check( EBASE_PAGE0 ))
	{
		return	Flash_Page_Read( EBASE_PAGE0, VirtualAddress, Buffer, Count );
	}
	else
	{
		return	Flash_Page_Read( EBASE_PAGE1, VirtualAddress, Buffer, Count );
	}
}


BOOL	Flash_InPage_Save( uint16_t const * Buffer, uint16_t Count )
{
	uint16_t	i;

	Flash_Unlock();

	if ( !Flash_Page_Erase( EBASE_PAGE0 )){  return  FALSE; }

	if ( Flash_isBusy( ProgramTimeout )){  return  FALSE; }

	SET_BIT( FLASH->CR, FLASH_CR_PG );

	for ( i = 0; i < Count; ++i )
	{
		(( __IO uint16_t * )EBASE_PAGE0)[i] = ~ Buffer[i];
		if ( Flash_isBusy( ProgramTimeout )){  return  FALSE; }
	}

    CLEAR_BIT( FLASH->CR, FLASH_CR_PG );
	
	Flash_Lock();

    return	TRUE;
}

BOOL	Flash_InPage_Load( uint16_t * Buffer, uint16_t Count )
{
	uint16_t	i;

	for ( i = 0; i < Count; ++i )
	{
		Buffer[i] = ~ EBASE_PAGE0[i];
	}
    return	TRUE;
}

/****************** (C) COPYRIGHT 2011 STMicroelectronics **** End of File ****/