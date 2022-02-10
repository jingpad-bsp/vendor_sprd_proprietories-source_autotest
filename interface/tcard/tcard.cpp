#include "interface.h"
#include "tcard.h"


#define AT_ASSERT(x)         \
    { int cnd = (x); if(!cnd) {LOGE("!!!!!!!! %s(), line: %d Asserted !!!!!!!!\n", __FUNCTION__, __LINE__); assert(cnd);} }

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

static const char TCARD_VOLUME_NAME[] = "vfat";
static char TCARD_DEV_NAME[128];
static const char SYS_BLOCK_PATH[] = "/sys/block";
static const char TCARD_FILE_NAME[] = "/mnt/sdcard/sciautotest";
static const char TCARD_TEST_CONTENT[] = "SCI TCard: 2012-11-12";
#define TCARD_TESTFILE_NAME 	"bbattestsd.txt"
//----------------------------------------------------------
//add the whale new test tcard function
static char DEV_TCARD_BLOCK_NAME[] = "/dev/block/mmcblk1p1";
static char DEV_TCARD1_BLOCK_NAME[] = "/dev/block/mmcblk2p2";
static const char DEV_TCARD_BLOCK_MNTNAME[] = "/mnt/media_rw/bbat_sd";

//------------------------------------------------------------------------------
int tcard_get_dev_path (char *path)
{
  DIR *dir;
  struct dirent *de;
  int found = 0;
  char dirpath_open[128] = "/sys/devices";
  char tcard_name[16];

  sprintf (path, "../devices");

  dir = opendir (dirpath_open);
  if (dir == NULL)
  {
    LOGD ("%s open fail\n", dirpath_open);
    return -1;
  }
  /*try to find dir: sdio_sd, dt branch */
  /* sdio_sd in 5.0/5.1  replaces the sprd-sdhci.0 in sprdroid4.4 */
  while ((de = readdir (dir)))
  {
    if (strncmp (de->d_name, "sdio_sd", strlen ("sdio_sd")))
    {
      continue;
    }
    sprintf (dirpath_open, "%s/%s", dirpath_open, de->d_name);
    sprintf (path, "%s/%s", path, de->d_name);
    found = 1;
    break;
  }
  /*try to find dir: sdio_sd,no dt branch */
  if (!found)
  {
    sprintf (dirpath_open, "%s/platform", dirpath_open);
    closedir (dir);
    dir = opendir (dirpath_open);
    if (dir == NULL)
    {
      LOGD ("%s open fail\n", dirpath_open);
      return -1;
    }
    while ((de = readdir (dir)))
    {
      if (strncmp (de->d_name, "sdio_sd", strlen ("sdio_sd")))
      {
	continue;
      }
      sprintf (dirpath_open, "%s/%s", dirpath_open, de->d_name);
      sprintf (path, "%s/platform/%s", path, de->d_name);
      found = 1;
      break;
    }
  }
  closedir (dir);
  if (!found)
  {
    LOGD ("sprd-sdhci.0 is not found \n");
    return -1;
  }
  found = 0;
  sprintf (dirpath_open, "%s/mmc_host", dirpath_open);
  dir = opendir (dirpath_open);
  if (dir == NULL)
  {
    LOGD ("%s open fail\n", dirpath_open);
    return -1;
  }
  /*may be mmc0 or mmc1 */
  while ((de = readdir (dir)))
  {
    if (strstr (de->d_name, "mmc") != NULL)
    {
      sprintf (dirpath_open, "%s/%s", dirpath_open, de->d_name);
      sprintf (path, "%s/mmc_host/%s", path, de->d_name);
      break;
    }
  }
  strncpy (tcard_name, de->d_name, sizeof (tcard_name) - 1);
  if (strlen (de->d_name) < 16)
  {
    tcard_name[strlen (de->d_name)] = 0;
  }
  else
  {
    tcard_name[15] = 0;
  }
  closedir (dir);
  dir = opendir (dirpath_open);	/*open dir: sprd-sdhci.0/mmc_host/mmc0 or mmc1 */
  if (dir == NULL)
  {
    LOGD ("%s open fail\n", dirpath_open);
    return -1;
  }
  /* try to find: sprd-sdhci.0/mmc_host/mmcx/mmcx:xxxx,  if can find this dir ,T card work well */
  while ((de = readdir (dir)))
  {
    if (strstr (de->d_name, tcard_name) == NULL)
    {
      continue;
    }
    sprintf (dirpath_open, "%s/%s", dirpath_open, de->d_name);
    sprintf (path, "%s/%s", path, de->d_name);
    found = 1;
    break;
  }
  closedir (dir);
  sprintf (dirpath_open, "%s/block", dirpath_open);
  dir = opendir (dirpath_open);
  if (dir == NULL)
  {
    LOGD ("%s open fail\n", dirpath_open);
    return -1;
  }
  while ((de = readdir (dir)))	/*may be mmcblk0 or mmcblk1 */
  {
    if (strstr (de->d_name, "mmcblk") != NULL)
    {
      sprintf (dirpath_open, "%s/%s", dirpath_open, de->d_name);
      sprintf (path, "%s/block/%s", path, de->d_name);
      break;
    }
  }
  closedir (dir);
  if (found)
  {
    LOGD ("the tcard dir is %s \n", path);
    return 0;
  }
  else
  {
    LOGD ("the tcard dir is not found \n");
    return -1;
  }
}

//------------------------------------------------------------------------------
int tcardOpen (void)
{
  int ret = 0;
  ret = tcard_get_dev_path (TCARD_DEV_NAME);
  LOGD ("ret %d \n", ret);
  return ret;
}

//------------------------------------------------------------------------------
int tcardIsPresent (void)
{
  DIR *dirBlck = opendir (SYS_BLOCK_PATH);
  if (NULL == dirBlck)
  {
    LOGD ("opendir '%s' fail: %s(%d)\n", SYS_BLOCK_PATH, strerror (errno),
	  errno);
    return -1;
  }
  int present = -2;

  char realName[256];
  char linkName[128];
  strncpy (linkName, SYS_BLOCK_PATH, sizeof (linkName) - 1);
  char *pname = linkName + strlen (linkName);
  *pname++ = '/';

  struct dirent *de;
  while ((de = readdir (dirBlck)))
  {
    if (de->d_name[0] == '.' || DT_LNK != de->d_type)
      continue;

    strncpy (pname, de->d_name, 64);

    int len = readlink (linkName, realName, sizeof (realName) - 1);
    if (len < 0)
    {
      LOGD ("readlink error: %s(%d)\n", strerror (errno), errno);
      continue;
    }
    if (len < 256)
    {
      realName[len] = 0;
    }
    else
    {
      realName[255] = 0;
    }

    LOGD ("link name = %s, real name = %s, TCARD_DEV_NAME=%s\n", linkName,
	  realName, TCARD_DEV_NAME);
    if (strstr (realName, TCARD_DEV_NAME) != NULL)
    {
      present = 1;
      LOGD ("TCard is present.\n");
      break;
    }
  }

  closedir (dirBlck);
  return present;
}

int tcardIsMount (char *mount_path ,int card_id)
{
  char device[256] = { 0 };
  //char mount_path[256]= {0};
  char sd_format_name[256] = { 0 };
  FILE *fp;
  char line[1024];
  int num = 0;

  int mount = 0;
  char mkcmd[256] = { 0 };
  char mntcmd[256] = { 0 };
  char umntcmd[256] = { 0 };
  char * card_name ;

  if (card_id == 0)
  {
     card_name = DEV_TCARD_BLOCK_NAME;
  }
  else if (card_id == 1)
  {
     card_name = DEV_TCARD1_BLOCK_NAME;
  }

  sprintf (mkcmd, "mkdir %s", DEV_TCARD_BLOCK_MNTNAME);
  sprintf (mntcmd, "mount -t vfat %s %s", card_name,
   	DEV_TCARD_BLOCK_MNTNAME);
  sprintf (umntcmd, "umount %s", DEV_TCARD_BLOCK_MNTNAME);
  if (0 == access (card_name, F_OK))
  {
    //char mount_bbat_patch[256] = "/mnt/media_rw/bbat_sd";
    while ((-1 == access (DEV_TCARD_BLOCK_MNTNAME, F_OK)) && ++num < 10)
    {
      system (mkcmd);
      usleep (100 * 1000);
    }
    system (mntcmd);
    usleep (1000);
    if (!(fp = fopen ("/proc/mounts", "r")))
    {
      LOGD ("Error opening /proc/mounts (%s)", strerror (errno));
      return -1;
    }

    while (fgets (line, sizeof (line), fp))
    {
      sscanf (line, "%255s %255s %255s\n", device, mount_path,
	      sd_format_name);
      if (NULL != strstr (mount_path, DEV_TCARD_BLOCK_MNTNAME))
      {
	LOGD ("dev = %s, mount = %s,sd_format_name = %s\n", device,
	      mount_path, sd_format_name);
	mount = 1;
	break;
      }
      memset (line, 0, sizeof (line));
    }
    LOGD ("TCard mount path: %s\n", mount_path);
    fclose (fp);
  }
  else
  {
    LOGD ("TCard has no exist!!!!\n");
  }
  //->add the umount the mount tcard piont
  system (umntcmd);
  LOGD ("TFlash Card mount OK.\n");
  return mount;
}

int tcardRWTest (char *mount_path)
{
  char bbattest_file[256] = { 0 };
  char umnt_cmd[256] = { 0 };
  sprintf (bbattest_file, "%s/%s", mount_path, TCARD_TESTFILE_NAME);
  //add the umount cmd
  sprintf (umnt_cmd, "umount %s", DEV_TCARD_BLOCK_MNTNAME);
  FILE *fp = fopen (bbattest_file, "w+");
  if (NULL == fp)
  {
    LOGD ("open '%s'(rw) fail: %s(%d)\n", bbattest_file, strerror (errno),
	  errno);
    return -1;
  }

  if (fwrite (TCARD_TEST_CONTENT, 1, sizeof (TCARD_TEST_CONTENT), fp) !=
      sizeof (TCARD_TEST_CONTENT))
  {
    LOGD ("write '%s' fail: %s(%d)\n", bbattest_file, strerror (errno),
	  errno);
    fclose (fp);
    return -2;
  }
  fclose (fp);

  fp = fopen (bbattest_file, "r");
  if (NULL == fp)
  {
    LOGD ("open '%s'(ronly) fail: %s(%d)\n", bbattest_file, strerror (errno),
	  errno);
    return -3;
  }

  AT_ASSERT (sizeof (TCARD_TEST_CONTENT) < 128);

  char buf[128];

  if (fread (buf, 1, sizeof (TCARD_TEST_CONTENT), fp) !=
      sizeof (TCARD_TEST_CONTENT))
  {
    LOGD ("read '%s' fail: %s(%d)\n", bbattest_file, strerror (errno), errno);
    fclose (fp);
    return -4;
  }
  fclose (fp);

  unlink (bbattest_file);

  if (strncmp (buf, TCARD_TEST_CONTENT, sizeof (TCARD_TEST_CONTENT) - 1))
  {
    LOGD ("read = %s, dst = %s\n", buf, TCARD_TEST_CONTENT);
    return -5;
  }
  //->add the umount the mount tcard piont
  system (umnt_cmd);
  LOGD ("TFlash Card rw OK.\n");
  return 0;
}

//------------------------------------------------------------------------------
int tcardClose (void)
{
  return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//--} // namespace
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
