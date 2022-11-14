#ifndef _MUSIC_SOC_H
#define _MUSIC_SOC_H

#include <asm/addrspace.h>

typedef unsigned int music_reg_t;

#define music_reg_rd(_phys)    \
	(*(volatile music_reg_t *)KSEG1ADDR(_phys))

#define music_reg_wr_nf(_phys, _val) \
    ((*(volatile music_reg_t *)KSEG1ADDR(_phys)) = (_val))

#define music_reg_wr(_phys, _val) do { \
    music_reg_wr_nf(_phys, _val); \
    music_reg_rd(_phys); \
} while(0)

#define music_reg_rmw_set(_reg, _mask) do { \
    music_reg_wr((_reg), (music_reg_rd((_reg)) | (_mask))); \
    music_reg_rd((_reg)); \
} while(0)

#define music_reg_rmw_clear(_reg, _mask) do { \
    music_reg_wr((_reg), (music_reg_rd((_reg)) & ~(_mask))); \
    music_reg_rd((_reg)); \
} while(0)


#define MUSIC_CPU_IRQ_BASE 			MIPS_CPU_IRQ_BASE
#define MUSIC_CPU_IRQ_TIMER 		MUSIC_CPU_IRQ_BASE+7
#define MUSIC_CPU_IRQ_UART  		MUSIC_CPU_IRQ_BASE+6
#define MUSIC_CPU_IRQ_DMA   		MUSIC_CPU_IRQ_BASE+5
#define MUSIC_CPU_IRQ_SWITCH_EXTERN MUSIC_CPU_IRQ_BASE+4
#define MUSIC_CPU_IRQ_WATCHDOG 		MUSIC_CPU_IRQ_BASE+3
#define MUSIC_CPU_IRQ_SWITCH_INTERN MUSIC_CPU_IRQ_BASE+2


#define MUSIC_UART0_ID   	0
#define MUSIC_UART1_ID   	1
#define MUSIC_REG_UART_BASE 	0x18060000
#define MUSIC_REG_UARTX_BASE(id) (MUSIC_REG_UART_BASE+0x10000*(id))

#define MUSIC_REG_RESET      	0x18000000
#define MUSIC_REG_RESET_MASK 	0x80000000

#define MUSIC_REG_SWITCH_BASE 0x18800000

#define MUSIC_REG_GPIO_BASE 	0x18080000
#define MUSIC_REG_GPIO_DATA_IN		(MUSIC_REG_GPIO_BASE + 0x4)
#define MUSIC_REG_GPIO_DATA_OUT 	(MUSIC_REG_GPIO_BASE + 0x8)

int music_bus_freq_get(void);
void serial_print(char *fmt, ...);

#define REG_SWITCH_GLB_CTRL_0_OFFSET	0x18800000
#define SWITCH_GLB_CTRL_0_REV_ID_MASK   0xff
#define MUSIC_VIVO_REV_ID_A0  		0x1
#define MUSIC_VIVO_REV_ID_B0  		0x2

#define MUSIC_IS_VIVO_B0() ((music_reg_rd(REG_SWITCH_GLB_CTRL_0_OFFSET) & \
						SWITCH_GLB_CTRL_0_REV_ID_MASK) == \
						MUSIC_VIVO_REV_ID_B0)

#endif
