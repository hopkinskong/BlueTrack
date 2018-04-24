#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := bluetrack-firmware

#COMPONENTS=bootloader bt coap cxx driver esp32 esptool_py ethernet expat freertos jsmn json log lwip micro-ecc newlib nvs_flash partition_table soc spi_flash tcpip_adapter vfs wear_levelling wpa_supplicant xtensa-debug-module camera heap app_trace bootloader_support mbedtls

include $(IDF_PATH)/make/project.mk
