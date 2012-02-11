/*
 * MMC definitions for OMAP2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [START]
#include "hsmmc.h"
// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [END]

struct twl4030_hsmmc_info {
	u8	mmc;		/* controller 1/2/3 */
	u8	wires;		/* 1/4/8 wires */
	bool	transceiver;	/* MMC-2 option */
	bool	ext_clock;	/* use external pin for input clock */
	bool	cover_only;	/* No card detect - just cover switch */
	bool	nonremovable;	/* Nonremovable e.g. eMMC */
	bool	power_saving;	/* Try to sleep or power off when possible */
	int	gpio_cd;	/* or -EINVAL */
	int	gpio_wp;	/* or -EINVAL */
	char	*name;		/* or NULL for default */
	struct device *dev;	/* returned: pointer to mmc adapter */
	int	ocr_mask;	/* temporary HACK */
};

extern unsigned get_last_off_on_transaction_id(struct device *dev);

// prime@sdcmicro.com Removed the following code because it is duplicated with hsmmc.h [START]
#if defined(CONFIG_MMC_EMBEDDED_SDIO) && !defined(CONFIG_TIWLAN_SDIO)
int omap_wifi_status_register(void (*callback)(int card_present,
       void *dev_id), void *dev_id);
int omap_wifi_status(int irq);
#endif
// prime@sdcmicro.com Removed the following code because it is duplicated with hsmmc.h [END]

#if defined(CONFIG_REGULATOR) && \
	(defined(CONFIG_MMC_OMAP) || defined(CONFIG_MMC_OMAP_MODULE) || \
	 defined(CONFIG_MMC_OMAP_HS) || defined(CONFIG_MMC_OMAP_HS_MODULE))

// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [START]
//void twl4030_mmc_init(struct twl4030_hsmmc_info *);
void twl4030_mmc_init(struct omap2_hsmmc_info *controllers);
// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [END]

#else

// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [START]
//static inline void twl4030_mmc_init(struct twl4030_hsmmc_info *info)
static inline void twl4030_mmc_init(struct omap2_hsmmc_info *controllers)
// prime@sdcmicro.com Modified HSMMC initialization code to adapt to 2.6.35 kernel [END]
{
}

#endif
