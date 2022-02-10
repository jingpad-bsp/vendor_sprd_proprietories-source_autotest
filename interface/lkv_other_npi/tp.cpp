#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <poll.h>

#include "tp.h"

#define LOG_TAG "ENGPC_TP"
#include <log/log.h>

#define TPINF(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define TPERR(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

#define TOUCH_PATH "/sys/touchscreen"
#define TOUCH_NAME_PATH TOUCH_PATH"/input_name"
#define TOUCH_SUSPEND_PATH TOUCH_PATH"/ts_suspend"

#define INPUT_DEV_PATH "/dev/input"

static int write_string(const char *path, const char *value)
{
	int fd;

	if (!path || !value) {
		TPERR("invalid path or value");
		return -1;
	}

	fd = open(path, O_RDWR);
	if (fd < 0) {
		TPERR("open %s failed, errno=%d(%s)\n",
			path, errno, strerror(errno));
		return fd;
	}

	if (write(fd, value, strlen(value)) != strlen(value)) {
		TPERR("write %s to %s failed, errno=%d(%s)\n",
			value, path, errno, strerror(errno));
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int tp_open(int mode)
{
	int fd, ret;
	char tp_name[64] = {0};
	char name[64] = {0};
	char devname[128] = {0};
	char *filename;
	DIR *dir;
	struct dirent *de;

	fd = open(TOUCH_NAME_PATH, O_RDONLY);
	if (fd < 0) {
		TPERR("open %s failed, errno=%d(%s)\n",
			TOUCH_NAME_PATH, errno, strerror(errno));
		return fd;
	}

	ret = read(fd, tp_name, sizeof(tp_name));
	if (ret < 0) {
		TPERR("read %s failed, errno=%d(%s)\n",
			TOUCH_NAME_PATH, errno, strerror(errno));
		close(fd);
		return ret;
	}
	close(fd);
	tp_name[ret - 1] = '\0';

	dir = opendir(INPUT_DEV_PATH);
	if (!dir) {
		TPERR("open %s failed, errno=%d(%s)",
			INPUT_DEV_PATH, errno, strerror(errno));
		return -1;
	}

	strncpy(devname, INPUT_DEV_PATH, strlen(INPUT_DEV_PATH));
	filename = devname + strlen(devname);
	*filename++ = '/';

	while ((de = readdir(dir))) {
		if (!strcmp(de->d_name, ".") ||
			!strcmp(de->d_name, ".."))
			continue;
		strcpy(filename, de->d_name);
		filename[strlen(de->d_name)] = '\0';
		fd = open(devname, mode);
		if (fd >= 0) {
			memset(name, 0, sizeof(name));
			ret = ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name);
			if ((ret >= 0) && !strcmp(name, tp_name)) {
				closedir(dir);
				TPINF("Tp found: %s\n", name);
				return fd;
			}
			close(fd);
		} else
			TPERR("open %s failed, errno=%d(%s)",
				devname, errno, strerror(errno));
	}

	closedir(dir);
	TPERR("no tp device found");
	return -1;
}

static int tp_close(int fd)
{
	return close(fd);
}

int tp_suspend(void)
{
	return write_string(TOUCH_SUSPEND_PATH, "1");
}

int tp_resume(void)
{
	return write_string(TOUCH_SUSPEND_PATH, "0");
}

int tp_test_id(void)
{
	int fd, ret;
	struct input_absinfo abi_x;

	fd = tp_open(O_RDONLY);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abi_x);
	if (ret) {
		TPERR("get tp absinfo failed");
		tp_close(fd);
		return ret;
	}

	if (abi_x.maximum > 0)
		ret = 0;
	else
		ret = -1;

	tp_close(fd);
	return ret;
}

int tp_test_point(struct tp_point *point)
{
	int fd, ret, count, x_ok, y_ok;
	struct pollfd fds;
	struct input_event event;

	if (!point)
		return -1;

	fd = tp_open(O_RDONLY|O_NONBLOCK);
	if (fd < 0)
		return fd;

	count = 0;
	x_ok = 0;
	y_ok = 0;
	while (1) {
		if (count++ > 500) {
			ret = 0;
			TPERR("poll tp timedout");
			break;
		}
		memset(&fds, 0, sizeof(fds));
		fds.fd = fd;
		fds.events = POLLIN;
		ret = poll(&fds, 1, 10);
		if ((ret == 0) || (ret < 0 && errno == EINTR))
			continue;
		else if (ret < 0) {
			TPERR("poll tp failed, errno=%d(%s)",
				errno, strerror(errno));
			break;
		}

		if (fds.revents & POLLIN) {
			memset(&event, 0, sizeof(event));
			ret = read(fd, &event, sizeof(event));
			if (ret != sizeof(event))
				continue;
			if (event.type == EV_ABS) {
				switch (event.code) {
				case ABS_MT_POSITION_X:
					point->x = (int)(event.value);
					x_ok = 1;
					break;
				case ABS_MT_POSITION_Y:
					point->y = (int)(event.value);
					y_ok = 1;
					break;
				default:
					break;
				}
			}

			if (x_ok && y_ok) {
				ret = 0;
				break;
			}
		}
	}

	tp_close(fd);

	return ret;
}
