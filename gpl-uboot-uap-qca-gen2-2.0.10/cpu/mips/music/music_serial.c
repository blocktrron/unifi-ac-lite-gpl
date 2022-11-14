#include <asm/addrspace.h>
#include <asm/types.h>
#include <config.h>
#include <music_soc.h>
#include "music_serial.h"
#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

#define MUSIC_REG_EFUSE_HIGH      0x10
#define MUSIC_REG_SWITCH_EFUSE    MUSIC_REG_EFUSE_HIGH
#define MUSIC_REG_CPU_SPEED_MASK  0x00000007

int
music_bus_freq_get(void)
{
    /*000=200MHz, 001=300MHz, 010=400MHz, 011=480MHz,
       100=533MHz, 101=600MHz. 110 and 111 Reserved. */
    int cpu_speed_mhz[8] = { 200, 300, 400, 480,
						 533, 600, 300, 300 };
    int reg_val = music_reg_rd(MUSIC_REG_SWITCH_BASE +
							   MUSIC_REG_SWITCH_EFUSE) &
						   MUSIC_REG_CPU_SPEED_MASK;
    return ((cpu_speed_mhz[reg_val] * 1000 * 1000) / 2);
}

s32 serial_setup(u32 uart_id, u32 uart_baud)
{
    u32 div = 0;
    u32 fifo_val = 0x07;

    div = music_bus_freq_get() / (16 * uart_baud);

    /* enable FIFO */
    UART_WRITE_REG(uart_id, OFS_FIFO_CONTROL, (fifo_val | 0x01));

    /* reset transmit FIFO and recieve FIFO*/
    UART_WRITE_REG(uart_id, OFS_FIFO_CONTROL, (fifo_val | 0x06));

    /*set DIAB bit*/
    UART_WRITE_REG(uart_id, OFS_LINE_CONTROL, 0x80);

    /* set divisor */
    UART_WRITE_REG(uart_id, OFS_DIVISOR_LSB, (div & 0xff));
    UART_WRITE_REG(uart_id, OFS_DIVISOR_MSB, ((div >> 8) & 0xff));

    /* clear DIAB bit*/
    UART_WRITE_REG(uart_id, OFS_LINE_CONTROL, 0x00);

    /* set data format */
    UART_WRITE_REG(uart_id, OFS_DATA_FORMAT, 0x3);

    UART_WRITE_REG(uart_id, OFS_INTR_ENABLE, 0);

	return 0;
}

static u32 *uart_init_id = (u32 *)0xbd004000;

void serial_id_init(u32 uart_id)
{
	*uart_init_id = uart_id;
}

u32 serial_id_get(void)
{
	u32 uart_id = UART_DEFAULT_ID;

	if(UART_ID_IS_VALID(*uart_init_id)) {
		uart_id = *uart_init_id;
	}
	return uart_id;
}

s32 serial_init(void)
{
	bd_t *bd;
	u32 uart_id = UART_DEFAULT_ID, uart_id_tmp;
	u32 uart_baud = UART_DEFAULT_BAUD;
	char *s;

	s = NULL;
	if((s = getenv ("uart_id"))) {
		uart_id_tmp = simple_strtoul (s, NULL, 10);
		if(UART_ID_IS_VALID(uart_id_tmp)) {
			uart_id = uart_id_tmp;
		}
	}

	if(UART_BAUD_IS_VALID(gd->baudrate)) {
		uart_baud = gd->baudrate;
	}

	serial_id_init(uart_id);

	serial_setup(uart_id, uart_baud);

	return 0;
}

s32 serial_tstc (void)
{
	u32 uart_id = serial_id_get();

    return(UART_READ_REG(uart_id, OFS_LINE_STATUS) & 0x1);
}

s32 serial_getc(void)
{
	u32 uart_id = serial_id_get();

    while(!serial_tstc()) {
        eth_net_polling();
    }

    return UART_READ_REG(uart_id, OFS_RCV_BUFFER);
}

static void _serial_putc_by_id(u32 uart_id, u8 byte)
{
    while(((UART_READ_REG(uart_id, OFS_LINE_STATUS)) & 0x20) == 0x0);

    UART_WRITE_REG(uart_id, OFS_SEND_BUFFER, byte);
}

void serial_putc(const char byte)
{
	u32 uart_id = serial_id_get();

    if (byte == '\n')  {
        _serial_putc_by_id (uart_id, '\r');
    }

    _serial_putc_by_id(uart_id, byte);
}

void serial_setbrg (void)
{
	serial_init();
}

void serial_puts (const char *s)
{
    while (*s) {
        serial_putc (*s ++);
    }
}
