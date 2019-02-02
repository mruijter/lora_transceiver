# lora_transceiver
A lora transceiver for peer to peer communication with the Raspberry PI dragino Lora GPS hat.

## Introduction
The software in this repository is a fork of:

https://github.com/dragino/rpi-lora-tranceiver/tree/master/dragino_lora_app

It depends on the dragino GPS hat for the Raspberry PI:

https://wiki.dragino.com/index.php?title=Lora/GPS_HAT

It contains a number of improvements and bug fixes. The most important bug fix is that it is now actually possible to change the SF, CR and CRC. The settings can now be stored in a configuration file as well. A new feature is that the lora_transceiver process will create two pipes. When a process writes to the /dev/shm/send_fifo the lora_transceiver daemon will send it. Other nodes running the same daemon will receive the message and write it to the /dev/shm/receive_fifo, which another process can read.

## Installation

Clone this repository with git clone:

git clone https://github.com/mruijter/lora_transceiver.git

Verify that wiringPI is installed. If not install it: apt-get install wiringpi

```
cd lora_transceiver
make clean; make; make install
```

You should now have a running lora_transceiver daemon:

```
root@lorabase:/opt/lora_transceiver# systemctl status lora**●** lora.service - Starts the lora transceiver.

   Loaded: loaded (/lib/systemd/system/lora.service; enabled; vendor preset: enabled)

   Active: **active (running)** since Sat 2019-02-02 21:13:42 UTC; 7min ago

 Main PID: 23918 (lora_transceive)

   CGroup: /system.slice/lora.service

​           `└─23918 /usr/bin/lora_transceiver`

Feb 02 21:13:42 lorabase systemd[1]: Started Starts the lora transceiver..

```

## Testing the daemon

Repeat the installation process on another node. Once the daemon is running on the second node test if everything works.

On the first node type:

`hexdump -C /dev/shm/receive_fifo` 

Open a terminal on the second node and type:

`while true; do echo "hello lora" >/dev/shm/send_fifo; sleep 1; done`

The first node should now start to receive messages:

```
root@loraclient01:/opt# hexdump -C /dev/shm/receive_fifo 
00000000  68 65 6c 6c 6f 20 6c 6f  72 61 0a 68 65 6c 6c 6f  |hello lora.hello|
00000010  20 6c 6f 72 61 0a 68 65  6c 6c 6f 20 6c 6f 72 61  | lora.hello lora|
00000020  0a 68 65 6c 6c 6f 20 6c  6f 72 61 0a 68 65 6c 6c  |.hello lora.hell|
00000030  6f 20 6c 6f 72 61 0a 68  65 6c 6c 6f 20 6c 6f 72  |o lora.hello lor|
^C
```

## Changing the configuration

The default configuration file that has been installed is : `/etc/lora/lora.conf`

This file can be used to change the frequency, coding rate and all other LoRa parameters. After changing the configuration file do not forget to restart lora: `systemctl restart lora.` Should the GPS or LoRa transceiver not work than inspect boot/config.txt and compare it with boot/config.txt in this repository.

Enjoy,

Mark Ruijter
