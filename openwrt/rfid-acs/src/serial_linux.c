#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "serial_linux.h"

enum {
	INVALID_FD = -1,
};

static void serial_initialize(serial_t * serial)
{
	serial->fd = INVALID_FD;
}

static void serial_clear(serial_t * serial)
{
	tcdrain(serial->fd);
	tcflush(serial->fd, TCIOFLUSH);
}

int serial_open(serial_t * serial, const char *device, long baudrate)
{
	int flags = 0;
	int ret = 0;

	serial_initialize(serial);

	serial->fd = open(device, O_RDWR | O_NONBLOCK | O_NOCTTY);
	if (serial->fd < 0) {
		return -1;
	}

	flags = fcntl(serial->fd, F_GETFL, 0);

	fcntl(serial->fd, F_SETFL, flags & ~O_NONBLOCK);

	tcgetattr(serial->fd, &serial->oldtio);
	tcgetattr(serial->fd, &serial->newtio);
	serial->newtio.c_iflag = 0;
	serial->newtio.c_oflag = 0;
	serial->newtio.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	serial->newtio.c_cflag |= CS8 | CREAD | CLOCAL;
	serial->newtio.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);

	serial->newtio.c_cc[VMIN] = 0;
	serial->newtio.c_cc[VTIME] = 0;

	ret = serial_set_baudrate(serial, baudrate);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

void serial_close(serial_t * serial)
{
	tcsetattr(serial->fd, TCSANOW, &serial->oldtio);

	if (serial->fd >= 0) {
		close(serial->fd);
		serial->fd = INVALID_FD;
	}
}

int serial_set_baudrate(serial_t * serial, long baudrate)
{
	long baudrate_value = -1;

	switch (baudrate) {
	case 4800:
		baudrate_value = B4800;
		break;

	case 9600:
		baudrate_value = B9600;
		break;

	case 19200:
		baudrate_value = B19200;
		break;

	case 38400:
		baudrate_value = B38400;
		break;

	case 57600:
		baudrate_value = B57600;
		break;

	case 115200:
		baudrate_value = B115200;
		break;

	default:
		return -1;
	}

	cfsetospeed(&serial->newtio, baudrate_value);
	cfsetispeed(&serial->newtio, baudrate_value);
	tcsetattr(serial->fd, TCSADRAIN, &serial->newtio);
	serial_clear(serial);

	return 0;
}

int serial_write(serial_t * serial, const char *data, int size)
{
	if (serial->fd == INVALID_FD) {
		return -1;
	}
	return write(serial->fd, data, size);
}

static int wait_receive(serial_t * serial, int timeout)
{
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(serial->fd, &rfds);

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	if (select(serial->fd + 1, &rfds, NULL, NULL, (timeout < 0) ? NULL : &tv) <= 0) {
		return 0;
	}
	return 1;
}

static int internal_receive(char data[], int data_size_max, serial_t * serial, int timeout)
{
	int filled = 0;

	if (data_size_max <= 0) {
		return 0;
	}

	while (filled < data_size_max) {
		int require_n;
		int read_n;

		if (!wait_receive(serial, timeout)) {
			break;
		}

		require_n = data_size_max - filled;

		read_n = read(serial->fd, &data[filled], require_n);
		if (read_n <= 0) {
			break;
		}
		filled += read_n;
	}
	return filled;
}

int serial_read(serial_t * serial, char *data, int max_size, int timeout)
{
	int filled = 0;

	if (max_size <= 0) {
		return 0;
	}

	if (serial->fd == INVALID_FD) {
		return -1;
	}

	filled += internal_receive(data, max_size, serial, timeout);
	return filled;
}
