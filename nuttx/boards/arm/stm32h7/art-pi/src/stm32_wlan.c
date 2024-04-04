/*
 * Copyright 2023 wtcat
 */
#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>

#include <arch/board/board.h>
#include <nuttx/sdio.h>
#include <nuttx/wireless/ieee80211/bcmf_board.h>
#include <nuttx/wireless/ieee80211/bcmf_sdio.h>
#include <sys/cdefs.h>

#include "stm32_gpio.h"
#include "stm32_sdmmc.h"

#define NVRAM_GENERATED_MAC_ADDRESS "macaddr=02:0A:F7:fe:86:1c"

#define SDIO_WLAN_SLOT 1
#define SDIO_WLAN_MINOR 0

static struct sdio_dev_s *wlan_sdio;

const char ap6212_nvram_image[] __aligned(CONFIG_IEEE80211_BROADCOM_DMABUF_ALIGNMENT) = {
	// # The following parameter values are just placeholders, need to be updated.
	"manfid=0x2d0"
	"\x00"
	"prodid=0x0726"
	"\x00"
	"vendid=0x14e4"
	"\x00"
	"devid=0x43e2"
	"\x00"
	"boardtype=0x0726"
	"\x00"
	"boardrev=0x1101"
	"\x00"
	"boardnum=22"
	"\x00"
	"xtalfreq=26000"
	"\x00"
	"sromrev=11"
	"\x00"
	"boardflags=0x00404201"
	"\x00"
	"boardflags3=0x08000000"
	"\x00" NVRAM_GENERATED_MAC_ADDRESS "\x00"
	"nocrc=1"
	"\x00"
	"ag0=255"
	"\x00"
	"aa2g=1"
	"\x00"
	"ccode=ALL"
	"\x00"
	// #Antenna diversity
	"swdiv_en=1"
	"\x00"
	"swdiv_gpio=2"
	"\x00"

	"pa0itssit=0x20"
	"\x00"
	"extpagain2g=0"
	"\x00"
	// #PA parameters for 2.4GHz, measured at CHIP OUTPUT
	"pa2ga0=-215,5267,-656"
	"\x00"
	"AvVmid_c0=0x0,0xc8"
	"\x00"
	"cckpwroffset0=5"
	"\x00"
	// # PPR params
	"maxp2ga0=80"
	"\x00"
	"txpwrbckof=6"
	"\x00"
	"cckbw202gpo=0x6666"
	"\x00"
	"legofdmbw202gpo=0xaaaaaaaa"
	"\x00"
	"mcsbw202gpo=0xbbbbbbbb"
	"\x00"
	"propbw202gpo=0xdd"
	"\x00"
	// # OFDM IIR :
	"ofdmdigfilttype=18"
	"\x00"
	"ofdmdigfilttypebe=18"
	"\x00"
	// # PAPD mode:
	"papdmode=1"
	"\x00"
	"papdvalidtest=1"
	"\x00"
	"pacalidx2g=32"
	"\x00"
	"papdepsoffset=-36"
	"\x00"
	"papdendidx=61"
	"\x00"
	// # LTECX flags
	// "ltecxmux=1"                                                         "\x00"
	//"ltecxpadnum=0x02030401"                                             "\x00"
	// "ltecxfnsel=0x3003"                                                  "\x00"
	// "ltecxgcigpio=0x3012"                                                "\x00"
	// #il0macaddr=00:90:4c:c5:12:38
	"wl0id=0x431b"
	"\x00"
	"deadman_to=0xffffffff"
	"\x00"
	// # muxenab: 0x1 for UART enable, 0x2 for GPIOs, 0x8 for JTAG, 0x10 for HW OOB
	"muxenab=0x11"
	"\x00"
	// # CLDO PWM voltage settings - 0x4 - 1.1 volt
	// #cldo_pwm=0x4                                                      "\x00"
	// #VCO freq 326.4MHz
	"spurconfig=0x3"
	"\x00"
	"\x00\x00"};
const size_t ap6212_nvram_image_len = sizeof(ap6212_nvram_image);

void bcmf_board_reset(int minor, bool reset) {
	if (minor == SDIO_WLAN_MINOR)
		stm32_gpiowrite(GPIO_WL_REG_ON, !reset);
}

void bcmf_board_power(int minor, bool power) {
	(void)minor;
	(void)power;
}

void bcmf_board_initialize(int minor) {
	if (minor == SDIO_WLAN_MINOR) {
		/* Configure reset pin */
		stm32_configgpio(GPIO_WL_REG_ON);
		/* Put wlan chip in reset state */
		bcmf_board_reset(minor, true);
	}
}

void bcmf_board_setup_oob_irq(int minor, int (*func)(void *), void *arg) {
	if (minor == SDIO_WLAN_MINOR) {
		if (wlan_sdio != NULL)
			sdio_set_sdio_card_isr(wlan_sdio, func, arg);
	}
}

bool bcmf_board_etheraddr(struct ether_addr *ethaddr) {
	ethaddr->ether_addr_octet[0] = 0x1c;
	ethaddr->ether_addr_octet[1] = 0x86;
	ethaddr->ether_addr_octet[2] = 0xfe;
	ethaddr->ether_addr_octet[3] = 0xf7;
	ethaddr->ether_addr_octet[4] = 0x0a;
	ethaddr->ether_addr_octet[5] = 0x02;
	return true;
}

int stm32_sdiowifi_initialize(void) {
	struct sdio_dev_s *sdio;
	int err;

	sdio = sdio_initialize(SDIO_WLAN_SLOT);
	if (!sdio) {
		ferr("ERROR: Failed to initialize SDIO slot %d\n", SDIO_WLAN_SLOT);
		return -ENODEV;
	}

	err = bcmf_sdio_initialize(0, sdio);
	if (err) {
		ferr("ERROR: Failed to initialize bcmf sdio device!\n");
		return err;
	}

	wlan_sdio = sdio;
	return 0;
}

int arm_netinitialize(void) {
	return 0;
}