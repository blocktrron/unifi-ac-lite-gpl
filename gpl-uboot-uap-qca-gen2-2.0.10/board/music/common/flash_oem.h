#include <common.h>
#include <config.h>
#include <asm/types.h>

#define MAGIC_LEN           4
#define OEM_ID_LEN          16
#define PRODUCT_LEN         16
#define BOARD_VERSION_LEN   4
#define SERIAL_NUM_LEN      32
#define MAC_ADDRESS_LEN     6
#define BOARD_OEM_MAGIC     0xC0DEC0DE

typedef struct
{
	unsigned int magic;
	char oem_id[OEM_ID_LEN];
	char product_id[PRODUCT_LEN];
	char board_version[BOARD_VERSION_LEN];
	char serial_num[SERIAL_NUM_LEN];
	unsigned char mac_address[MAC_ADDRESS_LEN];
	unsigned int capwap_base;
	unsigned int capwap_len;
} board_info_t;

#ifdef ASDK_SPI_BOOTROM_MODE
#define MUSIC_SPI_BASE 0x19000000
#else
#define MUSIC_SPI_BASE 0x1f000000
#endif
#define MUSIC_SPI_START ((MUSIC_SPI_BASE)|0xa0000000)

void
music_flash_write_oem(board_info_t board_info_new);
s32
music_flash_oem_get(board_info_t *board_infop);
