#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <poll.h>

#include <termios.h>

#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH	31
#define PF_BLUETOOTH	AF_BLUETOOTH
#endif

#define BTPROTO_L2CAP	0
#define BTPROTO_HCI	1
#define BTPROTO_SCO	2
#define BTPROTO_RFCOMM	3
#define BTPROTO_BNEP	4
#define BTPROTO_CMTP	5
#define BTPROTO_HIDP	6
#define BTPROTO_AVDTP	7
#define BTPROTO_ISO	8

#ifndef N_HCI
#define N_HCI	15
#endif

#define HCIUARTSETPROTO		_IOW('U', 200, int)
#define HCIUARTGETPROTO		_IOR('U', 201, int)
#define HCIUARTGETDEVICE	_IOR('U', 202, int)
#define HCIUARTSETFLAGS		_IOW('U', 203, int)
#define HCIUARTGETFLAGS		_IOR('U', 204, int)

#define HCI_UART_H4	0
#define HCI_UART_BCSP	1
#define HCI_UART_3WIRE	2
#define HCI_UART_H4DS	3
#define HCI_UART_LL	4
#define HCI_UART_ATH3K  5
#define HCI_UART_INTEL	6
#define HCI_UART_BCM	7
#define HCI_UART_QCA	8
#define HCI_UART_AG6XX	9
#define HCI_UART_NOKIA	10
#define HCI_UART_MRVL	11

#define HCI_UART_RAW_DEVICE	0
#define HCI_UART_RESET_ON_INIT	1
#define HCI_UART_CREATE_AMP	2
#define HCI_UART_INIT_PENDING	3
#define HCI_UART_EXT_CONFIG	4
#define HCI_UART_VND_DETECT	5

struct sockaddr_hci {
	sa_family_t	hci_family;
	unsigned short	hci_dev;
	unsigned short  hci_channel;
};
#define HCI_DEV_NONE	0xffff

#define HCI_CHANNEL_RAW		0
#define HCI_CHANNEL_USER	1
#define HCI_CHANNEL_MONITOR	2
#define HCI_CHANNEL_CONTROL	3
#define HCI_CHANNEL_LOGGING	4

static inline unsigned int tty_get_speed(int speed)
{
	switch (speed) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
	case 1000000:
		return B1000000;
	case 1152000:
		return B1152000;
	case 1500000:
		return B1500000;
	case 2000000:
		return B2000000;
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B3710000
	case 3710000:
		return B3710000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	}

	return 0;
}

static int create_socket(uint16_t index, uint16_t channel)
{
	struct sockaddr_hci addr;
	int fd;

	fd = socket(PF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK,
								BTPROTO_HCI);
	if (fd < 0)
		return -1;

	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = index;
	addr.hci_channel = channel;

	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

static int open_serial(const char *path, unsigned int speed, bool flowctl)
{
	struct termios ti;
	int fd, saved_ldisc, ldisc = N_HCI;

	fd = open(path, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror("Failed to open serial port");
		return -1;
	}

	if (tcflush(fd, TCIOFLUSH) < 0) {
		perror("Failed to flush serial port");
		close(fd);
		return -1;
	}

	if (ioctl(fd, TIOCGETD, &saved_ldisc) < 0) {
		perror("Failed get serial line discipline");
		close(fd);
		return -1;
	}

	/* Switch TTY to raw mode */
	memset(&ti, 0, sizeof(ti));
	cfmakeraw(&ti);

	ti.c_cflag |= (speed | CLOCAL | CREAD);

	if (flowctl) {
		/* Set flow control */
		ti.c_cflag |= CRTSCTS;
	}

	if (tcsetattr(fd, TCSANOW, &ti) < 0) {
		perror("Failed to set serial port settings");
		close(fd);
		return -1;
	}

	if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
		perror("Failed set serial line discipline");
		close(fd);
		return -1;
	}

	fprintf(stderr, "Switched line discipline from %d to %d\n", saved_ldisc, ldisc);

	return fd;
}

static bool bt_hci_new_user_channel(uint16_t index)
{
	int fd;

	fd = create_socket(index, HCI_CHANNEL_USER);
	if (fd < 0) return false;
	return true;
}

static int attach_proto(const char *path, unsigned int proto,
			unsigned int speed, bool flowctl, unsigned int flags)
{
	int fd, dev_id;

	fd = open_serial(path, speed, flowctl);
	if (fd < 0)
		return -1;

	if (ioctl(fd, HCIUARTSETFLAGS, flags) < 0) {
		perror("Failed to set flags");
		close(fd);
		return -1;
	}

	if (ioctl(fd, HCIUARTSETPROTO, proto) < 0) {
		perror("Failed to set protocol");
		close(fd);
		return -1;
	}

	dev_id = ioctl(fd, HCIUARTGETDEVICE);
	if (dev_id < 0) {
		perror("Failed to get device id");
		close(fd);
		return -1;
	}

	fprintf(stderr, "Device index %d attached\n", dev_id);

	if (flags & (1 << HCI_UART_RAW_DEVICE)) {
		unsigned int attempts = 6;
		bool hci;

		while (attempts-- > 0) {
			hci = bt_hci_new_user_channel(dev_id);
			if (hci)
				break;

			usleep(250 * 1000);
		}

		if (!hci) {
			fprintf(stderr, "Failed to open HCI user channel\n");
			close(fd);
			return -1;
		}
	}

	return fd;
}


static void usage(void)
{
	fprintf(stderr, "btattach - Bluetooth serial utility\n"
		"Usage:\n");
	fprintf(stderr, "\tbtattach [options]\n");
	fprintf(stderr, "options:\n"
		"\t-B, --bredr <device>   Attach Primary controller\n"
		"\t-A, --amp <device>     Attach AMP controller\n"
		"\t-P, --protocol <proto> Specify protocol type\n"
		"\t-S, --speed <baudrate> Specify which baudrate to use\n"
		"\t-N, --noflowctl        Disable flow control\n"
		"\t-h, --help             Show help options\n");
}

static const struct option main_options[] = {
	{ "bredr",    required_argument, NULL, 'B' },
	{ "amp",      required_argument, NULL, 'A' },
	{ "protocol", required_argument, NULL, 'P' },
	{ "speed",    required_argument, NULL, 'S' },
	{ "noflowctl",no_argument,       NULL, 'N' },
	{ "version",  no_argument,       NULL, 'v' },
	{ "help",     no_argument,       NULL, 'h' },
	{ }
};

static const struct {
	const char *name;
	unsigned int id;
} proto_table[] = {
	{ "h4",    HCI_UART_H4    },
	{ "bcsp",  HCI_UART_BCSP  },
	{ "3wire", HCI_UART_3WIRE },
	{ "h4ds",  HCI_UART_H4DS  },
	{ "ll",    HCI_UART_LL    },
	{ "ath3k", HCI_UART_ATH3K },
	{ "intel", HCI_UART_INTEL },
	{ "bcm",   HCI_UART_BCM   },
	{ "qca",   HCI_UART_QCA   },
	{ "ag6xx", HCI_UART_AG6XX },
	{ "nokia", HCI_UART_NOKIA },
	{ "mrvl",  HCI_UART_MRVL  },
	{ }
};

int main(int argc, char *argv[])
{
	struct erlcmd *handler = malloc(sizeof(struct erlcmd));
    erlcmd_init(handler, handle_elixir_request, NULL);

	const char *bredr_path = NULL, *amp_path = NULL, *proto = NULL;
	bool flowctl = true, raw_device = false;
	int exit_status = -1, count = 0, proto_id = HCI_UART_H4;
	unsigned int speed = B115200;

	for (;;) {
		int opt;

		opt = getopt_long(argc, argv, "B:A:P:S:NRvh", main_options, NULL);
		if (opt < 0) break;

		switch (opt) {
		case 'B':
			bredr_path = optarg;
			break;
		case 'A':
			amp_path = optarg;
			break;
		case 'P':
			proto = optarg;
			break;
		case 'S':
			speed = tty_get_speed(atoi(optarg));
			if (!speed) {
				fprintf(stderr, "Invalid speed: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'N':
			flowctl = false;
			break;
		case 'R':
			raw_device = true;
			break;
		case 'v':
			return EXIT_SUCCESS;
		case 'h':
			usage();
			return EXIT_SUCCESS;
		default:
			return EXIT_FAILURE;
		}
	}

	if (argc - optind > 0) {
		fprintf(stderr, "Invalid command line parameters\n");
		return EXIT_FAILURE;
	}

	if (proto) {
		unsigned int i;

		for (i = 0; proto_table[i].name; i++) {
			if (!strcmp(proto_table[i].name, proto)) {
				proto_id = proto_table[i].id;
				break;
			}
		}

		if (!proto_table[i].name) {
			fprintf(stderr, "Invalid protocol\n");
			return EXIT_FAILURE;
		}
	}

	if (bredr_path) {
		unsigned long flags;
		int fd;

		fprintf(stderr, "Attaching Primary controller to %s\n", bredr_path);

		flags = (1 << HCI_UART_RESET_ON_INIT);

		if (raw_device)
			flags = (1 << HCI_UART_RAW_DEVICE);

		fd = attach_proto(bredr_path, proto_id, speed, flowctl, flags);
		if (fd >= 0) { count++; }
	}

	if (count < 1) {
		fprintf(stderr, "No controller attached\n");
		return EXIT_FAILURE;
	}

    for (;;) {
        struct pollfd fdset[3];

        fdset[0].fd = STDIN_FILENO;
        fdset[0].events = POLLIN;
        fdset[0].revents = 0;

        int timeout = -1; // Wait forever unless told by otherwise
        int count = hcidev_add_poll_events(uart, &fdset[1], &timeout);

        int rc = poll(fdset, count + 1, timeout);
        if (rc < 0) {
            // Retry if EINTR
            if (errno == EINTR)
                continue;

            err(EXIT_FAILURE, "poll");
        }

        if (fdset[0].revents & (POLLIN | POLLHUP)) {
            if (erlcmd_process(handler))
                break;
        }

        // Call hcidev_process if it added any events
        if (count)
            hcidev_process(uart, &fdset[1]);
    }

	return exit_status;
}
