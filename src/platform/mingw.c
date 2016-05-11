#include <stdbool.h>
#include <memory.h>
#include <io.h>
#include <errno.h>
#include <winsock2.h>
#include "../common.h"
#include "./tap-windows.h"


static char *device_info = NULL;
HANDLE fd_map[2];
int fd_map_top = 0;
WSAOVERLAPPED overlappedRx[2], overlappedTx[2];
char read_buf[2][MTU];
int read_data_waiting[2];
DWORD read_data_len[2];

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

void win_fatal(char* msg) {
  printf("%s failed: %s\n", msg, winerror(GetLastError()));
  exit(-1);
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
    win_fatal("Read registry");
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
  read_data_waiting[fd] = 0;
  read_data_len[fd] = 0xffffffff;
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
    win_fatal("CONFIG_TUN");
  }

}

ssize_t read_tun(int fd, char* data, size_t len) {
  HANDLE handle = fd_map[fd];
  DWORD ret;

  if (read_data_waiting[fd]) {
    if (read_data_len[fd] == 0xffffffff) {
      errno = EAGAIN;
      return -1;
    }
    memcpy(data, read_buf[fd], read_data_len[fd]);
    read_data_waiting[fd] = 0;
    ret = read_data_len[fd];
    read_data_len[fd] = 0xffffffff;
    ResetEvent(overlappedRx[fd].hEvent);
    overlappedRx[fd].Offset = 0;
    overlappedRx[fd].OffsetHigh = 0;
    return ret;
  }

  read_data_len[fd] = 0xffffffff;
  int val = ReadFile(handle, read_buf + fd, len, read_data_len + fd, overlappedRx + fd);
  if (val == 0) {
    if (GetLastError() != ERROR_IO_PENDING) {
      win_fatal("ReadFile");
    } else {
      errno = EAGAIN;
      read_data_waiting[fd] = 1;
      return -1;
    }
  }

  memcpy(data, read_buf + fd, read_data_len[fd]);
  return read_data_len[fd];
}

ssize_t write_tun(int fd, char* data, size_t len) {
  HANDLE handle = fd_map[fd];
  DWORD ret;
  ResetEvent(overlappedTx->hEvent);
  overlappedTx[fd].Offset = 0;
  overlappedTx[fd].OffsetHigh = 0;
  int val = WriteFile(handle, data, len, &ret, overlappedTx + fd);
  if (val == 0) {
    if (GetLastError() != ERROR_IO_PENDING) {
      win_fatal("WriteFile");
    } else {
      val = GetOverlappedResult(handle, overlappedTx + fd, &ret, TRUE);
      if (val == 0) {
        win_fatal("GetOverlappedResult for write");
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

void poll_read(struct task* tasks, int task_count) {
  HANDLE events[task_count];
  for (int i = 0; i < task_count; ++i) {
    events[i] = overlappedRx[tasks[i].fd_in].hEvent;
  }

  int ret = WaitForMultipleObjects(task_count, events, FALSE, 1);
  if (ret == WAIT_TIMEOUT) {
    return;
  }
  if (!(ret >= WAIT_OBJECT_0 && (ret < WAIT_OBJECT_0 + task_count))) {
    if (GetLastError() != ERROR_IO_INCOMPLETE) {
      win_fatal("WaitForMultipleObjects");
    }
  }

  for (int i = 0; i < task_count; ++i) {
    int fd = tasks[i].fd_in;
    int ret = GetOverlappedResult(fd_map[fd], overlappedRx + fd, read_data_len + fd, FALSE);
    if (ret == 0) {
      if (GetLastError() == ERROR_IO_INCOMPLETE) {
        continue;
      }
      win_fatal("GetOverlappedResult for read");
    }
    task_transfer(tasks + i);
  }
}
