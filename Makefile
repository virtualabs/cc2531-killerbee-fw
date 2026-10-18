CONTIKI_PROJECT = cc2531-bumblebee

DEFINES+=MODELS_CONF_CC2531_USB_STICK=1 CC2530_RF_CONF_HEXDUMP=1 CC2530_RF_LOW_POWER_RX=0 USB_CONF_CLASS=0 USB_EP2_SIZE=64 USB_EP3_SIZE=64

PROJECT_SOURCEFILES += radio.c kb-usb.c utils.c proto.c

all: $(CONTIKI_PROJECT)

CONTIKI = ./contiki
CONTIKI_WITH_RIME = 1

# The contiki submodule needs patches/contiki/ (scripts/apply-contiki-patches.sh).
# Without --fsigned-char, SDCC >= 3.7 builds an image that never answers USB.
ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifeq ($(shell grep -c -e --fsigned-char $(CONTIKI)/cpu/cc253x/Makefile.cc253x 2>/dev/null),0)
$(error contiki is not patched: run scripts/apply-contiki-patches.sh (see README.md))
endif
endif

include $(CONTIKI)/Makefile.include
