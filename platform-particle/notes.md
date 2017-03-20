## Example configuration
reset
sensor0 pin 3 negative tim
base0 origin -1.528180 2.433750 -1.969390 matrix -0.841840 0.332160 -0.425400 -0.046900 0.740190 0.670760 0.537680 0.584630 -0.607540
base1 origin 1.718700 2.543170 0.725060 matrix 0.458350 -0.649590 0.606590 0.028970 0.693060 0.720300 -0.888300 -0.312580 0.336480
object0 sensor0 0.0000 0.0000 0.0000
serial2 9600  # Wifi
stream0 position object0 > serial2


To get the outputs via wifi, use socat:
```
socat - udp4-recv:3300,reuseaddr
```

## Build sequence for 'main' project.
dependent modules: user wiring hal system services communication platform wiring_globals

    include ../build/platform-id.mk
        PLATFORM_ID = 6
        ARCH=arm
        PLATFORM=photon
        STM32_DEVICE=STM32F2XX
        STM32_DEVICE_LC  = $(shell echo $(STM32_DEVICE) | tr A-Z a-z)  # Lowercase
        PLATFORM_NAME=photon
        PLATFORM_MCU=STM32F2xx
        PLATFORM_NET=BCM9WCDUSI09
        PRODUCT_DESC=Production Photon
        USBD_VID_SPARK=0x2B04
        USBD_PID_DFU=0xD006
        USBD_PID_CDC=0xC006
        PLATFORM_DYNALIB_MODULES=photon
        DEFAULT_PRODUCT_ID=6
        PLATFORM_STM32_STDPERIPH_EXCLUDE=y
        PLATFORM_DFU ?= 0x08020000
        PLATFORM_THREADING=1
        CFLAGS += -DSTM32_DEVICE -D$(STM32_DEVICE)
        PLATFORM_THREADING ?= 0
        CFLAGS += -DPLATFORM_THREADING=$(PLATFORM_THREADING)
        CFLAGS += -DPLATFORM_ID=$(PLATFORM_ID) -DPLATFORM_NAME=$(PLATFORM_NAME)
        CFLAGS += -DUSBD_VID_SPARK=$(USBD_VID_SPARK)
        CFLAGS += -DUSBD_PID_DFU=$(USBD_PID_DFU)
        CFLAGS += -DUSBD_PID_CDC=$(USBD_PID_CDC)

    include ../build/top-level-module.mk
        PROJECT_ROOT ?= ..
        MODULE_PATH=.
        COMMON_BUILD=$(PROJECT_ROOT)/build
        BUILD_PATH_BASE?=$(COMMON_BUILD)/target

    include ../build/macros.mk
        ...

    PLATFORM_DFU_LEAVE = y
    BUILD_PATH_EXT = $(BUILD_TARGET_PLATFORM)$(USER_FLAVOR)
    USE_PRINTF_FLOAT ?= y

    # It's critical that user comes before wiring so that the setup()/loop() functions are linked in preference
    # to the weak functions from wiring
    # NOTE: MAKE_DEPENDENCIES will be make-d; DEPENDENCIES will be include.mk-d.
    MAKE_DEPENDENCIES = newlib_nano user wiring hal system services communication platform wiring_globals
    DEPENDENCIES = $(MAKE_DEPENDENCIES) dynalib

    LIBS += $(MAKE_DEPENDENCIES)
    LIB_DEPS += $(USER_LIB_DEP) $(WIRING_LIB_DEP) $(SYSTEM_LIB_DEP) $(SERVICES_LIB_DEP) $(COMMUNICATION_LIB_DEP) $(HAL_LIB_DEP) $(PLATFORM_LIB_DEP) $(WIRING_GLOBALS_LIB_DEP)
    LIB_DIRS += $(dir $(LIB_DEPS))

    # Target this makefile is building.
    TARGET=elf bin lst hex size

    include ../build/arm-tlm.mk
    include arm-tools.mk
        ...toolchain

    include module.mk
        SOURCE_PATH ?= $(MODULE_PATH)
        include $(MODULE_PATH)/import.mk  # import this module's symbols
        # .. pull in the include.mk files from each dependency in $(DEPENDENCIES)
        include module-defaults.mk
            ..CFLAGS, 
            .. add include dirs INCLUDE_DIRS
            .. add linker dirs LIB_DIRS
            .. LDFLAGS += $(LIBS_EXT)
            .. LDFLAGS += -L$(COMMON_BUILD)/arm/linker
            .. WHOLE_ARCHIVE=y
            .. set TARGET_NAME, TARGET_PATH, TARGET_BASE, BUILD_PATH, BUILD_TARGET_PLATFORM, 
        include modules' all build.mk
            GLOBAL_DEFINES += MODULE_VERSION=$(MAIN_MODULE_VERSION)
            GLOBAL_DEFINES += MODULE_FUNCTION=$(MODULE_FUNCTION_MONO_FIRMWARE)
            GLOBAL_DEFINES += MODULE_DEPENDENCY=0,0,0
            .. set TARGET_FILE_NAME, TARGET_DIR_NAME
            .. TARGET_MAIN_SRC_PATH,
            .. append CPPSRC, CSRC.
        .. add GLOBAL_DEFINES as -D.
        CFLAGS += -D_WINSOCK_H -D_GNU_SOURCE
        .. unused: append CPPSRC, CSRC all .c/.cpp files from MODULE_LIBSV1/MODULE_LIBSV2 (set by user/build.mk)
        collect all .c/.cpp in BUILD_PATH to ALLOBJ & ALLDEPS

        goal 'all': TARGET (for main: elf bin lst hex size)
        for all of these: $(TARGET_BASE).<ext>

        goal 'build_dependencies': $(MAKE_DEPENDENCIES)
        goal '*.elf': build_dependencies $(ALLOBJ) $(LIB_DEPS) $(LINKER_DEPS) -> link everything with $(LDFLAGS)
        goal '*.bin': objcopy -binary + replace crc block at the end (append sha256 and crc)
        goal '*.hex': objcopy -O ihex

        include recurse.mk
            make all modules in MAKE_DEPENDENCIES by running make on them, with SUBDIR_GOALS
            pass GLOBAL_DEFINES and MODULAR_FIRMWARE to them.
            add "make_deps" and "clean_deps" goals.

## Include directories
-I../user/inc
-I../wiring/inc
-I../hal/inc
-I../hal/shared
-I../hal/src/photon
-I../hal/src/stm32f2xx
-I../hal/src/stm32
-I../hal/src/photon/api
-I../system/inc
-I../services/inc
-I../communication/src
-I../platform/shared/inc
-I../platform/MCU/STM32F2xx/CMSIS/Include
-I../platform/MCU/STM32F2xx/CMSIS/Device/ST/Include
-I../platform/MCU/STM32F2xx/SPARK_Firmware_Driver/inc
-I../platform/MCU/shared/STM32/inc
-I../platform/MCU/STM32F2xx/STM32_StdPeriph_Driver/inc
-I../platform/MCU/STM32F2xx/STM32_USB_Device_Driver/inc
-I../platform/MCU/STM32F2xx/STM32_USB_Host_Driver/inc
-I../platform/MCU/STM32F2xx/STM32_USB_OTG_Driver/inc
-I../dynalib/inc
-I.
