#include <stdbool.h>
#include <memory.h>
#include "../common.h"

#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)
#define ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define NETWORK_CONNECTIONS_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define USERMODEDEVICEDIR "\\\\.\\Global\\"
#define SYSDEVICEDIR      "\\Device\\"
#define USERDEVICEDIR     "\\DosDevices\\Global\\"
#define TAPSUFFIX         ".tap"
#define TAP_COMPONENT_ID "tap0801"


static char *device_info = NULL;

const char *winerror(int err) {
  static char buf[1024], *ptr;

  ptr = buf + snprintf(buf, sizeof buf, "(%d) ", err);

  if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ptr, sizeof(buf) - (ptr - buf), NULL)) {
    strncpy(buf, "(unable to format errormessage)", sizeof(buf));
  };

  if((ptr = strchr(buf, '\r')))
    *ptr = '\0';

  return buf;
}


int create_tun(struct tun_device* tun, char* iface) {
  char* device = NULL;
  int status;
  HANDLE handle = NULL;
  HKEY key, key2;
  int i;

  char regpath[1024];
  char adapterid[1024];
  char adaptername[1024];
  char tapname[1024];
  DWORD len;

  bool found = false;

  int err;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, NETWORK_CONNECTIONS_KEY, 0, KEY_READ, &key)) {
    printf("Unable to read registry: %s\n", winerror(GetLastError()));
    return -1;
  }

  for (i = 0; ; i++) {
    len = sizeof adapterid;
    if(RegEnumKeyEx(key, i, adapterid, &len, 0, 0, 0, NULL))
      break;

    snprintf(regpath, sizeof regpath, "%s\\%s\\Connection", NETWORK_CONNECTIONS_KEY, adapterid);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, regpath, 0, KEY_READ, &key2))
      continue;

    len = sizeof adaptername;
    err = RegQueryValueEx(key2, "Name", 0, 0, (LPBYTE)adaptername, &len);

    RegCloseKey(key2);

    if(err)
      continue;

    snprintf(tapname, sizeof tapname, USERMODEDEVICEDIR "%s" TAPSUFFIX, adapterid);
    handle = CreateFile(tapname, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
    if (handle != INVALID_HANDLE_VALUE) {
      found = true;
      break;
    }
  }

  RegCloseKey(key);

  if(!found) {
    printf("No Windows tap device found for %s\n", iface);
    printf("Please rename one of your tap-window adapter to %s\n", iface);
    return -1;
  }

  device = strdup(adapterid);

  if(handle == INVALID_HANDLE_VALUE) {
    snprintf(tapname, sizeof tapname, USERMODEDEVICEDIR "%s" TAPSUFFIX, device);
    handle = CreateFile(tapname, GENERIC_WRITE | GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);
  }

  if(handle == INVALID_HANDLE_VALUE) {
    printf("%s (%s) is not a usable Windows tap device: %s\n", device, iface, winerror(GetLastError()));
    return -1;
  }

  /* Get version information from tap device */

  {
    ULONG info[3] = {0};
    DWORD len;
    if(!DeviceIoControl(handle, TAP_IOCTL_GET_VERSION, &info, sizeof info, &info, sizeof info, &len, NULL))
      printf("Could not get version information from Windows tap device %s (%s): %s\n", device, iface, winerror(GetLastError()));
    else {
      printf("TAP-Windows driver version: %lu.%lu%s\n", info[0], info[1], info[2] ? " (DEBUG)" : "");

      /* Warn if using >=9.21. This is because starting from 9.21, TAP-Win32 seems to use a different, less efficient write path. */
      if(info[0] == 9 && info[1] >= 21)
        printf("You are using the newer (>= 9.0.0.21, NDIS6) series of TAP-Win32 drivers. "
                 "Using these drivers with tinc is not recommanded as it can result in poor performance. "
                 "You might want to revert back to 9.0.0.9 instead.");
    }
  }

  device_info = "Windows tap device";

  printf("%s (%s) is a %s\n", device, iface, device_info);

  tun->handle = handle;
  return 0;
}

void setup_tun(struct tun_device* tun, char* name, char* src, char* dst, int mtu) {
  char cmd[128];

  sprintf(cmd, "ifconfig %s %s %s up mtu %d", name, src, dst, mtu);
  system(cmd);
}

void get_now() {
  SYSTEMTIME systemtime;
  GetSystemTime(&systemtime);
  now.tv_sec = systemtime.wSecond + systemtime.wMinute*60 + systemtime.wHour*60*60;
  now.tv_nsec = systemtime.wMilliseconds * 1000000LL;
}

ssize_t write_tun(struct tun_device* tun, void* data, size_t len) {
  DWORD ret;
  WriteFile(tun->handle, data, len, &ret, NULL);
  return ret;
}

ssize_t read_tun(struct tun_device* tun, void* data, size_t len) {
  DWORD ret;
  ReadFile(tun->handle, data, len, &ret, NULL);
  return ret;
}

int poll_read(struct task* tasks, int count) {
  fd_set FDS;
  return 0;
}
