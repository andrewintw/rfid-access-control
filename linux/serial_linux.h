#ifndef _SERIAL_LINUX_H_
#define _SERIAL_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <termios.h>
#include <sys/select.h>

	typedef struct {
		int fd;
		struct termios oldtio;
		struct termios newtio;
	} serial_t;

	extern int serial_open(serial_t * serial, const char *device, long baudrate);
	extern void serial_close(serial_t * serial);
	extern int serial_set_baudrate(serial_t * serial, long baudrate);
	extern int serial_write(serial_t * serial, const char *data, int size);
	extern int serial_read(serial_t * serial, char *data, int max_size, int timeout);
	extern int serial_readline(serial_t * serial, char *data, int max_size, int timeout);
	extern int serial_error(serial_t * serial, char *error_message, int max_size);

#ifdef __cplusplus
}
#endif
#endif							/* !_SERIAL_LINUX_H_ */
