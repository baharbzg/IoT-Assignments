# name of your application
APPLICATION = TempLight

# If no BOARD is found in the environment, use this default:
BOARD ?= nucleo-f401re

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..


UPLINK ?= ethos

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

USEMODULE += gnrc_ipv6_default

USEMODULE += emcute

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

USEMODULE += gnrc_icmpv6_echo

USEMODULE += gnrc_netif_single

USEMODULE += gnrc_ipv6

USEMODULE += gnrc_dhcpv6

USEMODULE += xtimer
USEMODULE += timex



USEMODULE += stdio_ethos 
USEMODULE += gnrc_uhcpc 

USEMODULE += analog_util

FEATURES_REQUIRED += periph_adc
FEATURES_REQUIRED += periph_gpio

ETHOS_BAUDRATE ?= 115200
TAP ?= tap0
USE_DHCPV6 ?= 0
IPV6_PREFIX ?= 2001:0db8::/64 #?
SERVER_ADDR = 2001:0db8::1
SERVER_PORT = 1885

MQTT_TOPIC_OUT = localgateway_to_awsiot
MQTT_TOPIC_IN = awsiot_to_localgateway
CFLAGS += -DSERVER_ADDR='"$(SERVER_ADDR)"'
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
CFLAGS += -DMQTT_TOPIC_OUT='"$(MQTT_TOPIC_OUT)"'
CFLAGS += -DMQTT_TOPIC_IN='"$(MQTT_TOPIC_IN)"'

# Allow for env-var-based override of the nodes name (EMCUTE_ID)
ifneq (,$(EMCUTE_ID))
  CFLAGS += -DEMCUTE_ID=\"$(EMCUTE_ID)\"
endif

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Comment this out to join RPL DODAGs even if DIOs do not contain
# DODAG Configuration Options (see the doc for more info)
# CFLAGS += -DCONFIG_GNRC_RPL_DODAG_CONF_OPTIONAL_ON_JOIN

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(CURDIR)/Makefile.ethos.conf
include $(RIOTBASE)/Makefile.include

.PHONY: host-tools

host-tools:
	$(Q)env -u CC -u CFLAGS $(MAKE) -C $(RIOTTOOLS)