#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
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
#define SIGN_SCRIPT         "/path/to/your/sign/script.sh"
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

typedef struct rfid {
	unsigned long long curr_tag_id;
	unsigned long long last_tag_id;
	unsigned long long last_read_ms;
} rfid_t;

unsigned long millis(void)
{
	struct timeval time;

	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

int is_tag_near(struct rfid *tag)
{
	return ((millis() - tag->last_read_ms) < RDM6300_NEXT_READ_MS);
}

void do_sign(unsigned long long card_id)
{
	char cmd[64];

	sprintf(cmd, "%s %s", SIGN_SCRIPT, "out");
	printf("eip>> sign %llu: %s\n", card_id, cmd);
	system(cmd);
}

long hexstr_to_value(uint8_t * str, unsigned int length)
{
	char *copy = malloc((sizeof(char) * length) + 1);

	memcpy(copy, str, sizeof(char) * length);
	copy[length] = '\0';
	long value = strtol(copy, NULL, 16);

	free(copy);
	return value;
}

int verify_tag(uint8_t * buffer, unsigned long long *tag_id)
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


	if (msg_head != RDM6300_PACKET_BEGIN) {
		return -1;
	}

	if (msg_tail != RDM6300_PACKET_END) {
		return -1;
	}

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
		*tag_id = strtoull((char *) tag_hex, NULL, 16);
		debug("Extract-TagNum: %llu\n", *tag_id);

		ret = 0;
	} else {
		printf(" (NOT OK)\n");
		ret = 1;
	}

	return ret;
}

void read_tag_loop(serial_t * serial, rfid_t * tag)
{
	int res, i;
	uint8_t buffer[BUFFER_SIZE];
	unsigned long long tag_id;

	while (TRUE) {
		memset(buffer, 0, sizeof(buffer));
		res = serial_read(serial, (char *) buffer, 14, -1);
		debug("buffer=[%s], res=%d\n", buffer, res);
#if DEBUG
		for (i = 0; i < sizeof(buffer); i++) {
			debug("buffer[%02d]=[%c]\t%02Xh\n", i, buffer[i], buffer[i]);
		}
#endif
		if (verify_tag(buffer, &tag_id) == 0) {
			/* if a new tag appears- return it */
			if (tag->last_tag_id != tag_id) {
				tag->last_tag_id = tag_id;
				tag->last_read_ms = 0;
			}

			/* if the old tag is still here set tag_id to zero */
			if (is_tag_near(tag)) {
				tag_id = 0;
			}
			tag->last_read_ms = millis();
			tag->curr_tag_id = tag_id;

			if (tag->curr_tag_id != 0) {
				printf("card ID: %llu\n", tag->curr_tag_id);
			}

			if ((access(SIGN_SCRIPT, F_OK) != -1) && (tag->curr_tag_id != 0)) {
				do_sign(tag->curr_tag_id);
			}
		}
		//sleep(5);
	}
}

int main(int argc, char *argv[])
{
	int res;
	char *tty_dev;
	serial_t rfidserial;
	pid_t pid, sid;
	rfid_t tag = { 0, 0, 0 };


	if (argc == 2) {
		tty_dev = argv[1];
	} else {
		tty_dev = TTYDEVICE;
	}

	if (access(tty_dev, F_OK) == -1) {
		printf("cannot access %s: No such file\n", tty_dev);
		exit(EXIT_FAILURE);
	}

	pid = fork();

	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(0);

	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	close(STDIN_FILENO);
	//close(STDOUT_FILENO);
	//close(STDERR_FILENO);

	res = serial_open(&rfidserial, tty_dev, 9600);

	if ((res != 0) || (rfidserial.fd < 0)) {
		perror(TTYDEVICE);
		exit(EXIT_FAILURE);
	}

	read_tag_loop(&rfidserial, &tag);

	serial_close(&rfidserial);

	exit(EXIT_SUCCESS);
}
