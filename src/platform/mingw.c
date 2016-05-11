#include <stdbool.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#include "../common.h"
#include "./tap-windows.h"


static char *device_info = NULL;
HANDLE fd_map[10];
int fd_map_top = 1;
WSAOVERLAPPED overlappedRx[10], overlappedTx[10];

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

const char* inet_ntop(int af, const void* src, char* dst, int cnt) {
  struct sockaddr_in srcaddr;

  memset(&srcaddr, 0, sizeof(struct sockaddr_in));
  memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

  srcaddr.sin_family = af;
  if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
    DWORD rv = WSAGetLastError();
    printf("WSAAddressToString() : %s\n", winerror(rv));
    return NULL;
  }
  return dst;
}


static void enable_device(HANDLE handle) {
  ULONG status = 1;
  DWORD len;

  DeviceIoControl(handle, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof status, &status, sizeof status, &len, NULL);
}

static void disable_device(HANDLE handle) {
  DWORD len;
  ULONG status = 0;
  DeviceIoControl(handle, TAP_WIN_IOCTL_SET_MEDIA_STATUS, &status, sizeof status, &status, sizeof status, &len, NULL);
}


int create_tun(char* iface) {
  int status;
  HANDLE handle = NULL;
  HKEY key, key2;
  int i;

  char regpath[1024];
  char adapterid[1024];
  char adaptername[1024];
  char tapname[1024];
  DWORD len;

  int found = false;
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

    snprintf(tapname, sizeof tapname, USERMODEDEVICEDIR "%s" TAP_WIN_SUFFIX, adapterid);
    handle = CreateFile(tapname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                        OPEN_EXISTING , FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

    if (handle != INVALID_HANDLE_VALUE) {
      found = true;
      break;
    }
  }

  RegCloseKey(key);

  if(!found) {
    printf("No Windows tap device found for %s\n", iface);
    printf("Please add tap-window adapter and rename one of your adapter to %s\n", iface);
    return -1;
  }

  device_info = "Windows tap device";
  printf("%s (%s) is a %s\n", adapterid, iface, device_info);

  int fd = fd_map_top++;
  fd_map[fd] = handle;
  memset(overlappedRx + fd, 0, sizeof(OVERLAPPED));
  memset(overlappedTx + fd, 0, sizeof(OVERLAPPED));
  overlappedRx[fd].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  overlappedTx[fd].hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  disable_device(handle);

  return fd;
}

void setup_tun(int fd, char* name, char* src, char* dst, int mtu) {
  char cmd[128];
  int ret;
  DWORD len;

  sprintf(cmd, "netsh interface ipv4 set subinterface %s mtu=%d store=persistent", name, mtu);
  ret = system(cmd);
  if (ret) {
    printf("Set MTU failed, return %d\n", ret);
    exit(-1);
  };

  sprintf(cmd, "netsh interface ip set address \"%s\" static %s 255.255.255.0 %s", name, src, dst);
  ret = system(cmd);
  if (ret) {
    printf("Set ip addr failed, return %d\n", ret);
    exit(-1);
  };


  struct tun_info {
    uint32_t src, dst, mask;
  } info;
  info.src = (inet_addr(src));
  info.mask = (inet_addr("255.255.255.0"));
  info.dst = (inet_addr(dst)) & info.mask;

  enable_device(fd_map[fd]);
  ret = DeviceIoControl(fd_map[fd], TAP_WIN_IOCTL_CONFIG_TUN, &info, sizeof(info), &info, sizeof(info), &len, NULL);
  if (ret == 0) {
    printf("CONFIG_TUN failed: %s\n", winerror(GetLastError()));
//    exit(-1);
  }

}

ssize_t read_tun(int fd, char* data, size_t len) {
  HANDLE handle = fd_map[fd];
  DWORD ret;
  overlappedRx[fd].Offset = 0;
  overlappedRx[fd].OffsetHigh = 0;
  int val = ReadFile(handle, data, len, &ret, overlappedRx + fd);
  if (val == 0) {
    if (GetLastError() != ERROR_IO_PENDING) {
      printf("READ failed: %s\n", winerror(GetLastError()));
    } else {
      val = GetOverlappedResult(handle, overlappedRx + fd, &ret, TRUE);
      if (val == 0) {
        printf("READ failed: %s\n", winerror(GetLastError()));
      }
    }
  }
  return ret;
}

ssize_t write_tun(int fd, char* data, size_t len) {
  HANDLE handle = fd_map[fd];
  DWORD ret;
  overlappedTx[fd].Offset = 0;
  overlappedTx[fd].OffsetHigh = 0;
  int val = WriteFile(handle, data, len, &ret, overlappedTx + fd);
  if (val == 0) {
    if (GetLastError() != ERROR_IO_PENDING) {
      printf("WRITE failed: %s\n", winerror(GetLastError()));
    } else {
      val = GetOverlappedResult(handle, overlappedTx + fd, &ret, TRUE);
      if (val == 0) {
        printf("WRITE failed: %s\n", winerror(GetLastError()));
      }
    }
  }

  return ret;
}

struct timespec get_now() {
  SYSTEMTIME systemtime;
  GetSystemTime(&systemtime);

  struct timespec now;
  now.tv_sec = systemtime.wSecond + systemtime.wMinute*60 + systemtime.wHour*60*60;
  now.tv_nsec = systemtime.wMilliseconds * 1000000LL;
  return now;
}
