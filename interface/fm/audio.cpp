#include "audio.h"
#include <unistd.h>
#define AUDIO_EXT_CONTROL_PIPE "/dev/pipe/mmi.audio.ctrl"
#define AUDIO_EXT_DATA_CONTROL_PIPE "/data/local/media/mmi.audio.ctrl"
int SendAudioTestCmd(const unsigned char * cmd,int bytes)
{
    int fd = -1;
    int ret = -1;
    int bytes_to_read = bytes;
    const unsigned char *write_ptr=NULL;
    if (cmd == NULL) {
	return -1;
    }

    if (fd < 0) {
	fd = open(AUDIO_EXT_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
	if (fd < 0) {
	    fd = open(AUDIO_EXT_DATA_CONTROL_PIPE, O_WRONLY | O_NONBLOCK);
	}
    }

    if (fd < 0) {
	return -1;
    } else {
	write_ptr=cmd;
	do {
	    ret = write(fd, write_ptr, bytes);
	    if (ret > 0) {
		if (ret <= bytes) {
		    bytes -= ret;
		    write_ptr+=ret;
		}
	    } else if ((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
		LOGD("pipe write error %d, bytes read is %d", errno, bytes_to_read - bytes);
		break;
	    } else {
		LOGD("pipe_write_warning: %d, ret is %d", errno, ret);
	    }
	} while (bytes);
    }

    if (fd > 0) {
	close(fd);
    }

    if (bytes == bytes_to_read)
	return ret;
    else
	return (bytes_to_read - bytes);
}