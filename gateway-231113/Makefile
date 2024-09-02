CC := $(CROSS_COMPILE)gcc

ifeq ($(CROSS_COMPILE),)
LIB :=
else
LIB := -Llib
endif

CFLAGS := -Ithirdparty -lpaho-mqtt3c -lcurl -lcrypto -Wall -Wextra -O2 $(LIB)

SRC := thirdparty/log.c/log.c \
       thirdparty/cJSON/cJSON.c \
	   app/app_buffer.c \
	   app/app_task.c \
	   app/app_common.c \
	   app/app_device.c \
	   app/app_serial.c \
	   app/app_bluetooth.c \
	   app/app_message.c \
	   app/app_mqtt.c \
	   app/app_router.c \
	   app/app_runner.c \
	   daemon/daemon_runner.c \
	   daemon/daemon_subprocess.c \
	   ota/ota_http.c \
	   ota/ota_update.c

OBJ := $(SRC:.c=.o)

TARGET := atguigu-gateway

PEER := root@192.168.31.47
INSTALL_DIR := /usr/bin/

.PHONY: all clean cross

all: $(TARGET)

clean:
	$(RM) $(TARGET) main.o test.o $(OBJ)

cross:
	make clean
	CROSS_COMPILE=host/bin/arm-buildroot-linux-musleabihf- make all -j16
	scp $(TARGET) $(PEER):$(INSTALL_DIR)
	scp initd/S99atguigu-gateway $(PEER):/etc/init.d/

$(TARGET): main.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

test: test.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	-./$@
	$(RM) $@ $^