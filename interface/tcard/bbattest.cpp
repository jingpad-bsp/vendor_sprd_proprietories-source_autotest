#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <cutils/log.h>
#include "interface.h"
#include "tcard.h"

#define SYS_PATH  "/sys/devices/platform/soc/"
#define DT_PATH   "/sys/firmware/devicetree/base/soc/"
#define NODE_PATH "/dev/block/"
#define PATH_LENGTH 256
#define READ_BUFFER_LENGTH 8

char *path[]= {"ap-ahb/","ap-apb/"};

static bool is_sd_type(const char *path)
{
	int fd = -1;
	int ret = -1;
	char buf[READ_BUFFER_LENGTH] = {0};
	char file_path[PATH_LENGTH] = {0};

	strcat(file_path, path);
	strcat(file_path,"/sprd,name");
	LOGD("BBAT_SD is_sd_type file_path = %s \n",file_path);
	if((fd = open(file_path, O_RDONLY)) < 0){
		LOGD("%s open failed", __FUNCTION__);
		return false;
	}
	if((ret = read(fd, buf, READ_BUFFER_LENGTH)) < 0){
		LOGD("%s read failed", __FUNCTION__);
		return false;
	}
	close(fd);

	if (strncmp(buf, "sdio_sd", sizeof("sdio_sd")) == 0)
		return true;
	else
		return false;
}

static bool read_node(const char *path )
{
	DIR *dp;
	struct dirent *entry;
	char file_path[PATH_LENGTH] = {0};
	char node_path[PATH_LENGTH] = {0};
	char buf[READ_BUFFER_LENGTH] = {0};
	char file_default_path[PATH_LENGTH] = {0};
	int fd = -1;
	int ret = -1;

	strcat(file_path, path);
	strcat(node_path, NODE_PATH);
	getcwd(file_default_path, sizeof(file_default_path));

	strcat(file_path, "/block/");
	if ((dp = opendir(file_path)) == NULL) {
		LOGD("can't open dir %s. \n", file_path);
		return false;
	}

	chdir(file_path);
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(".", entry->d_name) == 0 ||
		    strcmp("..", entry->d_name) == 0 ||
		    strncmp(entry->d_name, "mmc", 3) != 0 )
			continue;

		strcat(node_path, entry->d_name);
		if((fd = open(node_path, O_RDONLY)) < 0){
			LOGD("%s open failed", __FUNCTION__);
			return false;
		}
		if((ret = read(fd, buf, READ_BUFFER_LENGTH)) < 0){
			LOGD("%s read failed", __FUNCTION__);
			return false;
		}
		close(fd);
		closedir(dp);
		chdir(file_default_path);
		if (ret == READ_BUFFER_LENGTH)
			return true;

		return false;
	}
	closedir(dp);
	chdir(file_default_path);

	return false;
}

int get_sdcard_path(char **sd1_path, char **sd2_path)
{
	DIR *dp;
	struct dirent *entry;
	char file_path[PATH_LENGTH] = {0};
	char file_default_path[PATH_LENGTH] = {0};
	int i = 0;
	int found = 0;
	int path_id = 0;

	getcwd(file_default_path, sizeof(file_default_path));

	for (i = 0; i < sizeof(path); i++) {
		memset(file_path, 0, PATH_LENGTH);
		strcat(file_path,DT_PATH);
		strcat(file_path, path[i]);
		if ((dp = opendir(file_path)) == NULL) {
			LOGD("can't open dir %s. \n", file_path);
			return 1;
		}
		chdir(file_path);
		LOGD("full path file_path = %s ",file_path);
		while ((entry = readdir(dp)) != NULL) {
			if (strcmp(".", entry->d_name) == 0 ||
			    strcmp("..", entry->d_name) == 0 ||
			    strstr(entry->d_name, "sdio") == NULL)
				continue;

			found = 1;
			memset(file_path, 0, PATH_LENGTH);
			strcat(file_path,DT_PATH);
			strcat(file_path, path[i]);
			strcat(file_path, entry->d_name);

			if (is_sd_type(file_path)) {
				path_id++;
				if (path_id == 1) {
					*sd1_path = (char*)malloc(PATH_LENGTH);
					if (!*sd1_path) {
						chdir(file_default_path);
						return 0;
					}
					memset(*sd1_path, 0, PATH_LENGTH);
					strcat(*sd1_path,SYS_PATH);
					strcat(*sd1_path, "soc:");
					strcat(*sd1_path, path[i]);
					strcat(*sd1_path, (char *)(strstr(entry->d_name, "@") + 1));
					strcat(*sd1_path, ".sdio");
					LOGD("BBAT_SD *sd1_path = %s \n",*sd1_path);
				} else {
					*sd2_path = (char*)malloc(PATH_LENGTH);
					if (!*sd2_path) {
						chdir(file_default_path);
						return 0;
					}
					memset(*sd2_path, 0, PATH_LENGTH);
					strcat(*sd2_path,SYS_PATH);
					strcat(*sd2_path, "soc:");
					strcat(*sd2_path, path[i]);
					strcat(*sd2_path, (char *)(strstr(entry->d_name, "@") + 1));
					strcat(*sd2_path, ".sdio");
					LOGD("BBAT_SD *sd1_path = %s \n",*sd2_path);
				}
			}
		}
		closedir(dp);
		if(found)
			break;
	}
	chdir(file_default_path);

	return found;
}

bool is_sdcard_exist(char *path)
{
	DIR *dp;
	DIR *dp_host;
	struct dirent *entry;
	struct dirent *entry_host;
	char file_path[PATH_LENGTH] = {0};
	char file_default_path[PATH_LENGTH] = {0};
	int found = 0;
	bool read_data = false;

	if (path == NULL)
		return false;

	getcwd(file_default_path, sizeof(file_default_path));
	strcat(file_path, path);
	strcat(file_path, "/mmc_host");
	if ((dp = opendir(file_path)) == NULL) {
		LOGD("can't open dir.");
		return false;
	}

	chdir (file_path);
	while ((entry = readdir(dp)) != NULL) {
		if (strcmp(".", entry->d_name) == 0 ||
		    strcmp("..", entry->d_name) == 0 ||
		    strncmp(entry->d_name,"mmc",3) != 0 )
			continue;

		strcat(file_path, "/");
		strcat(file_path, entry->d_name);
		if ((dp_host = opendir(file_path)) == NULL) {
			LOGD("can't open dir.");
			chdir(file_default_path);
			return false;
		}
		chdir(file_path);
		while ((entry_host = readdir(dp_host)) != NULL) {
			if (strcmp(".", entry_host->d_name) == 0 ||
			    strcmp("..", entry_host->d_name) == 0 ||
			    strncmp(entry_host->d_name, "mmc", 3) != 0)
				continue;

			found = 1;
			strcat(file_path, "/");
			strcat(file_path, entry_host->d_name);
			read_data = read_node(file_path);
		}
		closedir(dp_host);
	}
	closedir(dp);
	chdir(file_default_path);
	return found && read_data;
}
