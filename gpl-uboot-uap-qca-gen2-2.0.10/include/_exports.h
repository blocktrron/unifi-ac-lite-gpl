EXPORT_FUNC(get_version)
EXPORT_FUNC(getc)
EXPORT_FUNC(tstc)
EXPORT_FUNC(putc)
EXPORT_FUNC(puts)
EXPORT_FUNC(printf)
#if defined(CONFIG_I386) || defined(CONFIG_PPC)
EXPORT_FUNC(install_hdlr)
EXPORT_FUNC(free_hdlr)
#endif
EXPORT_FUNC(malloc)
EXPORT_FUNC(free)
EXPORT_FUNC(udelay)
EXPORT_FUNC(get_timer)
EXPORT_FUNC(vprintf)
EXPORT_FUNC(do_reset)
EXPORT_FUNC(getenv)
EXPORT_FUNC(setenv)
#if (CONFIG_COMMANDS & CFG_CMD_I2C)
EXPORT_FUNC(i2c_write)
EXPORT_FUNC(i2c_read)
#endif	/* CFG_CMD_I2C */