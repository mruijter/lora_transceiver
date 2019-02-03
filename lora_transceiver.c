/*******************************************************************************
 * This code is a fork of:
 * https://github.com/dragino/rpi-lora-tranceiver/tree/master/dragino_lora_app
 * It contains a number of bug fixes and improvements and makes it really simple
 * to enable LoRa peer to peer communication between two or more nodes.
 *  
 * Copyright (c) 2019 Mark Ruijter - mruijter[AT]gmail.com
 * Copyright (c) 2018 Dragino
 * http://www.dragino.com
 *
 * License: MIT
 *
 *******************************************************************************/

#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <lib_cfg.h>
#include <lib_log.h>
#include <lib_str.h>

/* LORASEND_FIFO : send this to other nodes.*/
#define LORASEND_FIFO "/dev/shm/send_fifo"
/* LORARECEIVE_FIFO : received from other nodes.*/
#define LORARECEIVE_FIFO "/dev/shm/receive_fifo"
/* User id that has access to the FIFO */
#define FIFO_OWNER 33

// #############################################
// #############################################
#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_FIFO_ADDR_PTR           0x0D
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS               0x12
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_SYMB_TIMEOUT_LSB        0x1F
#define REG_PKT_SNR_VALUE           0x19
#define REG_PAYLOAD_LENGTH          0x22
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_MAX_PAYLOAD_LENGTH      0x23
#define REG_HOP_PERIOD              0x24
#define REG_SYNC_WORD               0x39
#define REG_VERSION                 0x42

#define PAYLOAD_LENGTH              0x40

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN                0x20

#define RegDioMapping1              0x40 // common
#define RegDioMapping2              0x41 // common

#define RegPaConfig                 0x09 // common
#define RegPaRamp                   0x0A // common
#define RegPaDac                    0x5A // common

#define SX72_MC2_FSK                0x00
#define SX72_MC2_SF7                0x70
#define SX72_MC2_SF8                0x80
#define SX72_MC2_SF9                0x90
#define SX72_MC2_SF10               0xA0
#define SX72_MC2_SF11               0xB0
#define SX72_MC2_SF12               0xC0

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE  0x01 // mandated for SF11 and SF12

// sx1276 RegModemConfig1
#define SX1276_MC1_BW_7_81               0x00
#define SX1276_MC1_BW_10_41              0x10
#define SX1276_MC1_BW_15_62              0x20
#define SX1276_MC1_BW_20_83              0x30
#define SX1276_MC1_BW_31_25              0x40
#define SX1276_MC1_BW_41_66              0x50
#define SX1276_MC1_BW_62_5               0x60
#define SX1276_MC1_BW_125                0x70
#define SX1276_MC1_BW_250                0x80
#define SX1276_MC1_BW_500                0x90
#define SX1276_MC1_CR_4_5            0x02
#define SX1276_MC1_CR_4_6            0x04
#define SX1276_MC1_CR_4_7            0x06
#define SX1276_MC1_CR_4_8            0x08

#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON    0x01

// sx1276 RegModemConfig2
#define SX1276_MC2_RX_PAYLOAD_CRCON        0x04

// sx1276 RegModemConfig3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE  0x08
#define SX1276_MC3_AGCAUTO                 0x04

// preamble for lora networks (nibbles swapped)
#define LORA_MAC_PREAMBLE                  0x34

#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1 0x0A
#ifdef LMIC_SX1276
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x70
#elif LMIC_SX1272
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2 0x74
#endif

// FRF
#define        REG_FRF_MSB              0x06
#define        REG_FRF_MID              0x07
#define        REG_FRF_LSB              0x08

#define        FRF_MSB                  0xD9 // 868.1 Mhz
#define        FRF_MID                  0x06
#define        FRF_LSB                  0x66

// ----------------------------------------
// Constants for radio registers
#define OPMODE_LORA      0x80
#define OPMODE_MASK      0x07
#define OPMODE_SLEEP     0x00
#define OPMODE_STANDBY   0x01
#define OPMODE_FSTX      0x02
#define OPMODE_TX        0x03
#define OPMODE_FSRX      0x04
#define OPMODE_RX        0x05
#define OPMODE_RX_SINGLE 0x06
#define OPMODE_CAD       0x07

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK 0x80
#define IRQ_LORA_RXDONE_MASK 0x40
#define IRQ_LORA_CRCERR_MASK 0x20
#define IRQ_LORA_HEADER_MASK 0x10
#define IRQ_LORA_TXDONE_MASK 0x08
#define IRQ_LORA_CDDONE_MASK 0x04
#define IRQ_LORA_FHSSCH_MASK 0x02
#define IRQ_LORA_CDDETD_MASK 0x01

// DIO function mappings                D0D1D2D3
#define MAP_DIO0_LORA_RXDONE   0x00  // 00------
#define MAP_DIO0_LORA_TXDONE   0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT   0x00  // --00----
#define MAP_DIO1_LORA_NOP      0x30  // --11----
#define MAP_DIO2_LORA_NOP      0xC0  // ----11--

#define CONFIG_FILE "/etc/lora/lora.conf"
// #############################################
// #############################################
//
typedef bool boolean;
typedef unsigned char byte;

// The SPI device this board is using.
const int SPI_DEVICE = 0;

char message[256];
bool sx1272 = true;
byte receivedbytes;
enum sf_t { SF7=7, SF8, SF9, SF10, SF11, SF12 };

/*******************************************************************************
 *
 * Configure these values in /etc/lora/lora.conf
 *
 *******************************************************************************/

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0  = 7;
int RST   = 0;

// Set spreading factor (SF7 - SF12)
sf_t sf = SF7;
uint8_t bw = SX1276_MC1_BW_125;
uint8_t cr = SX1276_MC1_CR_4_5;
// By default enable CRC checks.
uint8_t crc = 0x04;
bool lora_debug = 0;
int verbose = 2;

// Set center frequency
static uint32_t freq = 868100000; // in Mhz! (868.1)

void hexdump(byte *frame, byte datalen){
    int count=0;

    while(count < datalen) {
        printf("%02X ", frame[count]);
        count += 1;
    }
    printf("\n");
}

/**
 * Returns the current time in microseconds.
 */
long get_micro_time(){
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

void die(const char *s)
{
    perror(s);
    exit(1);
}

void selectreceiver()
{
    digitalWrite(ssPin, LOW);
}

void unselectreceiver()
{
    digitalWrite(ssPin, HIGH);
}

byte readReg(byte addr)
{
    unsigned char spibuf[2];

    selectreceiver();
    spibuf[0] = addr & 0x7F;
    spibuf[1] = 0x00;
    wiringPiSPIDataRW(SPI_DEVICE, spibuf, 2);
    unselectreceiver();

    return spibuf[1];
}

void writeReg(byte addr, byte value)
{
    unsigned char spibuf[2];
    spibuf[0] = addr | 0x80;
    spibuf[1] = value;
    if (lora_debug) {
        printf("writeReg:\n");
        hexdump((byte *)spibuf, 2);
    }
    selectreceiver();
    wiringPiSPIDataRW(SPI_DEVICE, (unsigned char *)spibuf, 2);
    unselectreceiver();
}

static void opmode (uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & ~OPMODE_MASK) | mode);
}

static void opmodeLora() {
    // Fix me low speed
    uint8_t u = OPMODE_LORA;
    if (sx1272 == false)
        u |= 0x8;   // TBD: sx1276 high freq
    if (lora_debug)
        printf("opmodeLora %x -> %x\n", OPMODE_LORA, REG_OPMODE);
    writeReg(REG_OPMODE, u);
}

void SetupLoRa()
{
    digitalWrite(RST, HIGH);
    delay(100);
    digitalWrite(RST, LOW);
    delay(100);

    byte version = readReg(REG_VERSION);

    if (version == 0x22) {
        // sx1272
        if (lora_debug)
            printf("SX1272 detected, starting.\n");
        sx1272 = true;
    } else {
        // sx1276?
        digitalWrite(RST, LOW);
        delay(100);
        digitalWrite(RST, HIGH);
        delay(100);
        version = readReg(REG_VERSION);
        if (version == 0x12) {
            // sx1276
            if (lora_debug)
                printf("SX1276 detected, starting.\n");
            sx1272 = false;
        } else {
            fprintf(stderr, "Unrecognized transceiver.\n");
            exit(1);
        }
    }

    opmode(OPMODE_SLEEP);
    /* Set the register enabled for Lora.
       See page 108 of the Semtech manual.
     */ 
    writeReg(REG_OPMODE, OPMODE_LORA); // Set the register enabled for Lora.

    // set frequency
    uint64_t frf = ((uint64_t)freq << 19) / 32000000;
    writeReg(REG_FRF_MSB, (uint8_t)(frf>>16) );
    writeReg(REG_FRF_MID, (uint8_t)(frf>> 8) );
    writeReg(REG_FRF_LSB, (uint8_t)(frf>> 0) );

    writeReg(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word

    if (sx1272) {
        if (sf == SF11 || sf == SF12) {
            writeReg(REG_MODEM_CONFIG,0x0B);
        } else {
            writeReg(REG_MODEM_CONFIG,0x0A);
        }
        writeReg(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
    } else {
        if (sf == SF11 || sf == SF12) {
            writeReg(REG_MODEM_CONFIG3,0x0C);
        } else {
            writeReg(REG_MODEM_CONFIG3,0x04);
        }
        // Speed and CR settings.
        writeReg(REG_MODEM_CONFIG, bw | cr);
        // Configure crc checks.
        writeReg(REG_MODEM_CONFIG2, sf<<4 | crc);
    }
    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeReg(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeReg(REG_SYMB_TIMEOUT_LSB,0x08);
    }
    writeReg(REG_MAX_PAYLOAD_LENGTH,0x80);
    writeReg(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
    writeReg(REG_HOP_PERIOD,0xFF);
    writeReg(REG_FIFO_ADDR_PTR, readReg(REG_FIFO_RX_BASE_AD));
    writeReg(REG_LNA, LNA_MAX_GAIN);
}

boolean receive(char *payload) {
    // clear rxDone
    writeReg(REG_IRQ_FLAGS, 0x40);
    int irqflags = readReg(REG_IRQ_FLAGS);

    //  payload crc: 0x20
    if((irqflags & 0x20) == 0x20)
    {
        printf("CRC error\n");
        writeReg(REG_IRQ_FLAGS, 0x20);
        return false;
    } else {

        byte currentAddr = readReg(REG_FIFO_RX_CURRENT_ADDR);
        byte receivedCount = readReg(REG_RX_NB_BYTES);
        receivedbytes = receivedCount;
        if (verbose == 2) {
            printf("receive : %i bytes received.\n", receivedbytes);
        }
        writeReg(REG_FIFO_ADDR_PTR, currentAddr);

        for(int i = 0; i < receivedCount; i++)
        {
            payload[i] = (char)readReg(REG_FIFO);
        }
    }
    return true;
}

int receivepacket() {
    int retval = -1;
    long int SNR;
    int rssicorr;

    if(digitalRead(dio0) == 1)
    {
        printf("digitalRead(dio0) == 1\n");
        if(receive(message)) {
            byte value = readReg(REG_PKT_SNR_VALUE);
            if( value & 0x80 ) // The SNR sign bit is 1
            {
                // Invert and divide by 4
                value = ( ( ~value + 1 ) & 0xFF ) >> 2;
                SNR = -value;
            }
            else
            {
                // Divide by 4
                SNR = ( value & 0xFF ) >> 2;
            }
            
            if (sx1272) {
                rssicorr = 139;
            } else {
                rssicorr = 157;
            }
            retval = (int)receivedbytes;
            if (retval > 0 and lora_debug) {
                printf("Packet RSSI: %d, ", readReg(0x1A)-rssicorr);
                printf("RSSI: %d, ", readReg(0x1B)-rssicorr);
                printf("SNR: %li, ", SNR);
            }
        } // received a message
    } // dio0=1
    return(retval);
}

static void configPower (int8_t pw) {
    if (sx1272 == false) {
        // printf("Power boost %i\n", pw);
        // no boost used for now
        if(pw >= 17) {
            pw = 15;
        } else if(pw < 2) {
            pw = 2;
        }
        // check board type for BOOST pin
        writeReg(RegPaConfig, (uint8_t)(0x80|(pw&0xf)));
        writeReg(RegPaDac, readReg(RegPaDac)|0x4);
    } else {
        printf("Power boost\n");
        // set PA config (2-17 dBm using PA_BOOST)
        if(pw > 17) {
            pw = 17;
        } else if(pw < 2) {
            pw = 2;
        }
        writeReg(RegPaConfig, (uint8_t)(0x80|(pw-2)));
    }
}


static void writeBuf(byte addr, byte *value, byte len) {                                                       
    unsigned char spibuf[256];                                                                          
    spibuf[0] = addr | 0x80;                                                                            
    for (int i = 0; i < len; i++) {                                                                         
        spibuf[i + 1] = value[i];                                                                       
    }                                                                                                   
    selectreceiver();                                                                                   
    wiringPiSPIDataRW(SPI_DEVICE, spibuf, len + 1);                                                        
    unselectreceiver();                                                                                 
}

void txlora(byte *frame, byte datalen) {
    int start = 0;
    if (lora_debug)
        hexdump(frame, datalen);
    // Set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|MAP_DIO2_LORA_NOP);
    // Clear all radio IRQ flags
    writeReg(REG_IRQ_FLAGS, 0xFF);
    // mask all IRQs but TxDone
    writeReg(REG_IRQ_FLAGS_MASK, ~IRQ_LORA_TXDONE_MASK);

    // Initialize the payload size and address pointers
    writeReg(REG_FIFO_TX_BASE_AD, 0x00);
    writeReg(REG_FIFO_ADDR_PTR, 0x00);
    writeReg(REG_PAYLOAD_LENGTH, datalen);

    // Download buffer to the radio FIFO
    writeBuf(REG_FIFO, frame, datalen);
    // now we actually start the transmission
    start = get_micro_time();
    opmode(OPMODE_TX);
    // Wait for completion.
    while ((readReg(REG_IRQ_FLAGS) & IRQ_LORA_TXDONE_MASK) == 0) {
        usleep(10);
    }
    if (lora_debug)
        printf("send: %i bytes in %li microseconds\n", datalen, (get_micro_time() - start));
}

int open_create_fifo(const char *fifo_path, int flags){
    /* Create the fifo when fifo_path does
       not exist.
    */
    int retval = -1;
    int fd = -1;
    struct stat fifo_stat;

    retval = stat(fifo_path, &fifo_stat);
    if (retval < 0) {
        printf("Creating fifo %s\n", fifo_path); 
        retval = mkfifo((char *)fifo_path, 0666);
        retval = chown(fifo_path, FIFO_OWNER, FIFO_OWNER);
        printf("Done\n");
    }
    if (retval != 0) {
        printf("create_fifo : failed to create fifo %s",
               fifo_path);
        exit(retval);
    }
    fd = open((char *)fifo_path, flags);
    if (fd < 0 ) {
        printf("create_fifo : failed to open fifo %s - %i\n",
               fifo_path, fd);
        exit(fd);
    }
    printf("Fifo %s is now open\n", fifo_path);
    return(fd);     
}

void get_modem_config() {
    int modem_bw;
    int modem_coding_rate;
    int implicit_header_mode;
    int modem_config; 
    modem_config = readReg(REG_MODEM_CONFIG);

    modem_bw = modem_config >> 4 & 0x0F;
    modem_coding_rate = modem_config >> 1 & 0x07;
    implicit_header_mode = modem_config >> 0x01;
    printf("BW = %x, coding_rate = %x, implicit_header_mode = %x\n",
           modem_bw, modem_coding_rate, implicit_header_mode);
}

void load_config() {
    char *conf_bw = NULL;
    char *conf_sf = NULL;
    char *conf_crc = NULL;
    char *conf_debug = NULL;
    char *conf_verbose = NULL;
    char *conf_frequency = NULL;
    char *conf_coding_rate = NULL;
    /* Read the configuration file and push the variables into the
       environment.
     */
    r_env_cfg((char *)CONFIG_FILE);
    conf_bw = read_val((char *)"bw"); 
    conf_sf = read_val((char *)"sf"); 
    conf_crc = read_val((char *)"crc"); 
    conf_debug = read_val((char *)"debug"); 
    conf_verbose = read_val((char *)"verbose");
    conf_frequency = read_val((char *)"frequency");
    conf_coding_rate = read_val((char *)"coding_rate");
    if (conf_verbose != NULL) {
        verbose = atoi(conf_verbose);
        if (verbose > 0)
            printf("Verbose level: %i\n", verbose);
    }
    if (conf_frequency != NULL)
        freq = atoi(conf_frequency);
    if (conf_bw != NULL) {
        if (strncasecmp(conf_bw, "BW7_81", strlen("BW7_81")) == 0) {
            printf("Selected bandwidth: BW7_81\n"); 
            bw = SX1276_MC1_BW_7_81;
        } else if (strncasecmp(conf_bw, "BW10_41", strlen("BW10_41")) == 0) {
            printf("Selected bandwidth: BW10_41\n"); 
            bw = SX1276_MC1_BW_10_41;
        } else if (strncasecmp(conf_bw, "BW20_83", strlen("BW20_83")) == 0) {
            printf("Selected bandwidth: BW20_83\n"); 
            bw = SX1276_MC1_BW_20_83;
        } else if (strncasecmp(conf_bw, "BW31_25", strlen("BW31_25")) == 0) {
            printf("Selected bandwidth: BW31_25\n"); 
            bw = SX1276_MC1_BW_31_25;
        } else if (strncasecmp(conf_bw, "BW41_66", strlen("BW41_66")) == 0) {
            printf("Selected bandwidth: BW41_66\n"); 
            bw = SX1276_MC1_BW_41_66;
        } else if (strncasecmp(conf_bw, "BW62_5", strlen("BW62_5")) == 0) {
            printf("Selected bandwidth: BW62_5\n"); 
            bw = SX1276_MC1_BW_62_5;
        } else if (strncasecmp(conf_bw, "BW125", strlen("BW125")) == 0) {
            printf("Selected bandwidth: BW125\n"); 
            bw = SX1276_MC1_BW_125;
        } else if (strncasecmp(conf_bw, "BW250", strlen("BW250")) == 0) {
            printf("Selected bandwidth: BW250\n"); 
            bw = SX1276_MC1_BW_250;
        } else if (strncasecmp(conf_bw, "BW500", strlen("BW500")) == 0) {
            printf("Selected bandwidth: BW500\n"); 
            bw = SX1276_MC1_BW_500;
        }
    }
    if (conf_sf != NULL) {
        if (strncasecmp(conf_sf, "SF7", 3) == 0) {
            printf("Selected spreading factor: SF7\n");
            sf = SF7;
        } else if (strncasecmp(conf_sf, "SF8", 3) == 0) {
            printf("Selected spreading factor: SF8\n");
            sf = SF8;
        } else if (strncasecmp(conf_sf, "SF9", 3) == 0) {
            printf("Selected spreading factor: SF9\n");
            sf = SF9;
        } else if (strncasecmp(conf_sf, "SF10", 4) == 0) {
            printf("Selected spreading factor: SF10\n");
            sf = SF10;
        } else if (strncasecmp(conf_sf, "SF11", 4) == 0) {
            printf("Selected spreading factor: SF11\n");
            sf = SF11;
        } else if (strncasecmp(conf_sf, "SF12", 4) == 0) {
            printf("Selected spreading factor: SF12\n");
            sf = SF12;
        }
    }
    if (conf_crc != NULL) {
        if (strncasecmp(conf_crc, "enabled", strlen("enabled")) == 0) {
            printf("CRC checks: enabled\n");
            crc = 0x04;
        } else {
            printf("CRC checks: disabled\n");
            crc = 0x00;
        }
    }
    if (conf_coding_rate != NULL) {
        if (strncasecmp(conf_coding_rate, "CR4_5", strlen("CR4_5")) == 0) {
            printf("Selected coding rate: CR4_5\n");
            cr = SX1276_MC1_CR_4_5;
        } else if (strncasecmp(conf_coding_rate, "CR4_6", strlen("CR4_6")) == 0) {
            printf("Selected coding rate: CR4_6\n");
            cr = SX1276_MC1_CR_4_6;
        } else if (strncasecmp(conf_coding_rate, "CR4_7", strlen("CR4_7")) == 0) {
            printf("Selected coding rate: CR4_7\n");
            cr = SX1276_MC1_CR_4_7;
        } else if (strncasecmp(conf_coding_rate, "CR4_8", strlen("CR4_8")) == 0) {
            printf("Selected coding rate: CR4_8\n");
            cr = SX1276_MC1_CR_4_8;
        }
    }
    if (conf_debug != NULL) {
        if(strncasecmp(conf_debug, "enabled", strlen("enabled")) == 0) {
            printf("Debugging: enabled\n");
            lora_debug = 1;
        } else {
            lora_debug = 0;
        }
    }
    if (verbose)
        printf("Send / receive packets at:  SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
}

int main (int argc, char *argv[]) {
    int rfd = -1;
    int wfd = -1;
    int written = -1;
    int buflen = -1;
    int retv = -1;
    struct pollfd fds[1];
    printf("------------------------------------\n");
    load_config();
    wiringPiSetup();
    pinMode(ssPin, OUTPUT);
    pinMode(dio0, INPUT);
    pinMode(RST, OUTPUT);

    /* This program reads from lora and writes to the
       receive fifo. Clients read from this fifo.
    */
    rfd = open_create_fifo(LORARECEIVE_FIFO, O_RDWR | O_NONBLOCK);
    wfd = open_create_fifo(LORASEND_FIFO, O_RDWR | O_NONBLOCK);

    memset(fds, 0 , sizeof(fds)); 
    fds[0].fd = wfd;
    fds[0].events = POLLIN;

    // SPI bus speed. The lowest speed is sufficient.
    wiringPiSPISetupMode(SPI_DEVICE, 500000, 0);
    SetupLoRa();
    if (lora_debug)
        get_modem_config();
    opmodeLora();
    printf("------------------------------------\n");
    printf("\n\n");
    while(1)  {
        opmode(OPMODE_STANDBY);
        writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08); // set PA ramp-up time 50 uSec
        configPower(23);

        while(1) {
            memset(&message, 0, sizeof(message));
            buflen = 0;
            while (buflen < (int)sizeof(message)) {
                retv = read(wfd, (void *)&message[buflen], 1);
                if (retv > 0)
                    buflen += retv;
                else
                    break;
            }
            if (buflen > 0) {
                if (verbose >= 1) {
                    printf("------------------\n");
                    printf("Sending %i bytes.\n", buflen);
                }
                if (verbose > 1)
                    hexdump((byte *)message, buflen);
                txlora((byte *)message, buflen);
                while ((readReg(REG_IRQ_FLAGS) & IRQ_LORA_TXDONE_MASK) == 0){
                    delay(10);
                }
            } else
                break;
        }
        // radio init
        opmode(OPMODE_STANDBY);
        // reset the IRQ mapping after sending.
        writeReg(RegDioMapping1, 0);
        writeReg(REG_IRQ_FLAGS, 0x0);
        writeReg(REG_IRQ_FLAGS_MASK, 0);
        opmode(OPMODE_RX);
        if (verbose > 1)
            printf("Listening at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
        do {
            memset(message, 0, sizeof(message));
            buflen = receivepacket();
            if (buflen > 0 && verbose >= 1) {
                printf("------------------\n");
                printf("Received %i bytes.\n", buflen);
                if (verbose > 1)
                    hexdump((byte *)message, buflen);
            }
            written = 0;
            if (buflen >= 0) {
                written = write(rfd, (byte *)message, buflen);
                if (written != buflen)
                    printf("Short write %i < %i\n", written, buflen);
            }
        } while (! poll(fds, 1, 0));
    }
    exit(0);
}
