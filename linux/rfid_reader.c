#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "serial_linux.h"

#ifdef  DEBUG
#define debug(fmt,args...)  printf(fmt ,##args)
#else
#define debug(fmt,args...)
#endif

#define TTYDEVICE           "/dev/ttyUSB1"
#define _POSIX_SOURCE       1	/* POSIX compliant source */

#define FALSE               0
#define TRUE                1

#define BUFFER_SIZE         14	/* RFID DATA FRAME FORMAT:
								   -  1 byte head (value: 0x2)
								   - 10 byte data (2byte version + 8byte tag)
								   -  2 byte checksum
								   -  1 byte tail (value: 0x3) */
#define HEAD_SIZE            1
#define DATA_SIZE           10	/* 10 byte data (2byte version + 8byte tag) */
#define DATA_VERSION_SIZE    2	/*  2 byte version (actual meaning of these two bytes may vary) */
#define DATA_TAG_SIZE        8	/*  8 byte tag */
#define CHECKSUM_SIZE        2	/*  2 byte checksum */

long hexstr_to_value(uint8_t * str, unsigned int length)
{
	char *copy = malloc((sizeof(char) * length) + 1);

	memcpy(copy, str, sizeof(char) * length);
	copy[length] = '\0';
	long value = strtol(copy, NULL, 16);

	free(copy);
	return value;
}

int verify_tag(uint8_t * buffer, unsigned long long *tag_num)
{
	int i, ret;
	long checksum = 0;
	uint8_t msg_head = buffer[0];
	uint8_t *msg_data = buffer + 1;	// 10 byte => data contains 2byte version + 8byte tag
	uint8_t *msg_data_version = msg_data;
	uint8_t *msg_data_tag = msg_data + 2;
	uint8_t *msg_checksum = buffer + 11;	// 2 byte
	uint8_t msg_tail = buffer[13];

	uint8_t tag_hex[DATA_TAG_SIZE];

	debug("--------\n");
	debug("Message-Head:   %02Xh\n", msg_head);

	debug("Message-Data:   ");
	for (i = 0; i < DATA_VERSION_SIZE; ++i) {
		debug("%c", msg_data_version[i]);
	}
	debug(" (version) + ");

	for (i = 0; i < DATA_TAG_SIZE; ++i) {
		debug("%c", msg_data_tag[i]);
	}
	debug(" (tag)\n");

	debug("Message-ChkSum: ");
	for (i = 0; i < CHECKSUM_SIZE; ++i) {
		debug("%c", msg_checksum[i]);
	}

	debug("\nMessage-Tail:   %02Xh\n", msg_tail);


	for (i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE) {
		long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);

		checksum ^= val;
	}

	debug("--------\n");
	printf("Extract-ChkSum: %02lX", checksum);

	if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) {
		printf(" (OK)\n");

		memset(tag_hex, 0, sizeof(tag_hex));
		strncpy((char *) tag_hex, (char *) msg_data_tag, DATA_TAG_SIZE);
		tag_hex[DATA_TAG_SIZE] = '\0';
		*tag_num = strtoull((char *) tag_hex, NULL, 16);
		debug("Extract-TagNum: %llu\n", *tag_num);

		ret = 0;
	} else {
		printf(" (NOT OK)\n");
		ret = 1;
	}

	return ret;
}

void read_tag_loop(serial_t * serial)
{
	int res, i;
	uint8_t buffer[BUFFER_SIZE];
	unsigned long long tag_num;

	while (TRUE) {
		memset(buffer, 0, sizeof(buffer));
		res = serial_read(serial, (char *) buffer, 14, -1);
		debug("buffer=[%s], res=%d\n", buffer, res);
#if DEBUG
		for (i = 0; i < sizeof(buffer); i++) {
			debug("buffer[%02d]=[%c]\t%02Xh\n", i, buffer[i], buffer[i]);
		}
#endif
		if (verify_tag(buffer, &tag_num) == 0) {
			printf("card ID: %llu\n", tag_num);
		}
	}
}

int main(int argc, char *argv[])
{
	int res;
	char *tty_dev;
	serial_t rfidserial;

	if (argc == 2) {
		tty_dev = argv[1];
	} else {
		tty_dev = TTYDEVICE;
	}

	if (access(tty_dev, F_OK) == -1) {
		printf("cannot access %s: No such file\n", tty_dev);
		exit(-1);
	}

	res = serial_open(&rfidserial, tty_dev, 9600);

	if ((res != 0) || (rfidserial.fd < 0)) {
		perror(TTYDEVICE);
		exit(-1);
	}

	read_tag_loop(&rfidserial);

	serial_close(&rfidserial);

	return 0;
}
