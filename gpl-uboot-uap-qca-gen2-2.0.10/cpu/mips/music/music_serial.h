#ifndef _MUSIC_SERIAL_H_
#define _MUSIC_SERIAL_H_

#define UART_DEFAULT_ID 	0
#define UART_DEFAULT_BAUD 	115200

#define UART_ID_IS_VALID(id) ((id == 0) || (id == 1))
#define UART_BAUD_IS_VALID(baud) ((baud == 9600) || \
								  (baud == 19200) || \
								  (baud == 38400) || \
								  (baud == 57600) || \
								  (baud == 115200) || \
								  (baud == 230400))
/* register offset */
#define	REG_OFFSET		4

#define OFS_RCV_BUFFER		(0*REG_OFFSET)
#define OFS_TRANS_HOLD		(0*REG_OFFSET)
#define OFS_SEND_BUFFER		(0*REG_OFFSET)
#define OFS_INTR_ENABLE		(1*REG_OFFSET)
#define OFS_INTR_ID		    (2*REG_OFFSET)
#define OFS_FIFO_CONTROL    (2*REG_OFFSET)
#define OFS_DATA_FORMAT		(3*REG_OFFSET)
#define OFS_LINE_CONTROL	(3*REG_OFFSET)
#define OFS_MODEM_CONTROL	(4*REG_OFFSET)
#define OFS_RS232_OUTPUT	(4*REG_OFFSET)
#define OFS_LINE_STATUS		(5*REG_OFFSET)
#define OFS_MODEM_STATUS	(6*REG_OFFSET)
#define OFS_RS232_INPUT		(6*REG_OFFSET)
#define OFS_SCRATCH_PAD		(7*REG_OFFSET)

#define OFS_DIVISOR_LSB		(0*REG_OFFSET)
#define OFS_DIVISOR_MSB		(1*REG_OFFSET)

#define MUSIC_UART0_BASE    0x18060000
#define MUSIC_UARTX_BASE(id) (MUSIC_UART0_BASE+0x10000*(id))

#define UART_READ_REG(id, reg)	    music_reg_rd((MUSIC_UARTX_BASE(id) + reg))
#define UART_WRITE_REG(id, reg, val)   music_reg_wr((MUSIC_UARTX_BASE(id) + reg), val)

s32 soc_bus_freq_get(void);
extern void eth_net_polling(void);

#endif
