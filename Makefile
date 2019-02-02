CC=g++ -I ./include
CFLAGS=-c -Wall
LIBS=-lwiringPi
OBJECTS=lora_transceiver.c lib_log.c lib_str.c lib_cfg.c

all: lora_transceiver

lora_transceiver: lora_transceiver.o
	$(CC) $(OBJECTS) $(LIBS) -o lora_transceiver

clean:
	rm *.o lora_transceiver

install:
	install -m 755 lora_transceiver /usr/bin
	install -m 644 lora.service /lib/systemd/system
	install -D -b -m 644 lora.conf-example /etc/lora/lora.conf
	systemctl daemon-reload
	systemctl restart lora
	systemctl status lora
