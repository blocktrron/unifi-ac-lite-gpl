#ifndef __MISC_H
#define __MISC_H

/* Validate hex character */
static inline int
_is_hex(char c) {
	return (((c >= '0') && (c <= '9')) ||
			((c >= 'A') && (c <= 'F')) ||
			((c >= 'a') && (c <= 'f')));
}

/* Convert a single hex nibble */
static inline int
_from_hex(char c) {
	int ret = 0;

	if ((c >= '0') && (c <= '9')) {
		ret = (c - '0');
	} else if ((c >= 'a') && (c <= 'f')) {
		ret = (c - 'a' + 0x0a);
	} else if ((c >= 'A') && (c <= 'F')) {
		ret = (c - 'A' + 0x0A);
	}
	return ret;
}

#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))
#define UNUSED(x) ((void)(x))

#if 0
unsigned int gpio_set(unsigned char* gpios, int count);
unsigned int gpio_input_get(u8 num);
u32 gpio_bitbang_re(u8 clk_pin, u8 data_pin, u8 bitcount, u32 value);
#endif

#endif /* __MISC_H */
