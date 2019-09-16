#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define BAUDRATE            B9600
#define MODEMDEVICE         "/dev/ttyUSB1"
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

volatile int STOP = FALSE;


long hexstr_to_value(char *str, unsigned int length)
{
	char *copy = malloc((sizeof(char) * length) + 1);

	memcpy(copy, str, sizeof(char) * length);
	copy[length] = '\0';
	// the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
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

	printf("--------\n");
	printf("Message-Head:   %02Xh\n", msg_head);

	printf("Message-Data:   ");
	for (i = 0; i < DATA_VERSION_SIZE; ++i) {
		printf("%c", msg_data_version[i]);
	}
	printf(" (version) + ");

	for (i = 0; i < DATA_TAG_SIZE; ++i) {
		printf("%c", msg_data_tag[i]);
	}
	printf(" (tag)\n");

	printf("Message-ChkSum: ");
	for (i = 0; i < CHECKSUM_SIZE; ++i) {
		printf("%c", msg_checksum[i]);
	}

	printf("\nMessage-Tail:   %02Xh\n", msg_tail);


	for (i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE) {
		long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);

		checksum ^= val;
	}

	printf("--------\n");
	printf("Extract-ChkSum: %02lX", checksum);

	if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) {
		printf(" (OK)\n");

		memset(tag_hex, 0, sizeof(tag_hex));
		strncpy(tag_hex, msg_data_tag, DATA_TAG_SIZE);
		tag_hex[DATA_TAG_SIZE] = '\0';
		*tag_num = strtoull(tag_hex, NULL, 16);
		printf("Extract-TagNum: %llu\n", *tag_num);

		ret = 0;
	} else {
		printf(" (NOT OK)\n");
		ret = 1;
	}

	return ret;
}

int main(int argc, char *argv[])
{
	int fd, c, res, i;
	struct termios oldtio, newtio;

	uint8_t buffer[BUFFER_SIZE];
	unsigned long long tag_num;


	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(MODEMDEVICE);
		exit(-1);
	}

	tcgetattr(fd, &oldtio);				/* save current port settings */
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;					/* set input mode (non−canonical, no echo,...) */
	newtio.c_cc[VTIME] = 0;				/* inter-character timer unused */
	newtio.c_cc[VMIN] = BUFFER_SIZE;	/* blocking read until BUFFER_SIZE chars received */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	while (STOP == FALSE) {
		memset(buffer, 0, sizeof(buffer));
		res = read(fd, buffer, BUFFER_SIZE);
		buffer[res] = '\0';
		printf("buffer=[%s], res=%d\n", buffer, res);
		if (buffer[13] == '\x3') {	// tail offset
#if 0
			for (i = 0; i < sizeof(buffer); i++) {
				printf("buffer[%02d]=[%c]\t%02Xh\n", i, buffer[i], buffer[i]);
			}
#endif
			if (verify_tag(buffer, &tag_num) == 0) {
				printf("card ID: %llu\n", tag_num);
			}
			//STOP = TRUE;
			sleep(1);
		}
	}

	tcsetattr(fd, TCSANOW, &oldtio);

	if (fd) {
		close(fd);
	}
	return 0;
}
