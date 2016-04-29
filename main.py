import os
import fcntl
import time
import platform
import struct
import socket

PLATFORM = platform.system()

if PLATFORM == "Darwin":
    pass
elif PLATFORM == "Linux":
    pass
else:
    raise "Unsupported platform"


class TunDevice:

    TUN_MODE = 0x0001
    TUNSETIFF = 0x400454ca

    def __init__(self, ifname):
        self.ifname = ifname

        if PLATFORM == "Darwin":
            TUN_DEVICE = "/dev/" + ifname
        elif PLATFORM == "Linux":
            TUN_DEVICE = "/dev/net/tun"

        self.fd = os.open(TUN_DEVICE, os.O_RDWR)
        if self.fd < 0:
            raise "Cannot open TUN_DEVICE: " + TUN_DEVICE

        fcntl.fcntl(self.fd, fcntl.F_SETFD, fcntl.FD_CLOEXEC)

        if PLATFORM == "Linux":
            fcntl.ioctl(self.fd, self.TUNSETIFF,
                        struct.pack("16sH", ifname, self.TUN_MODE))
        elif PLATFORM == "Darwin":
            pass

    def __del__(self):
        os.close(self.fd)

    def setIpAddr(self, srcip, dstip, mtu):
        if PLATFORM == "Linux":
            os.system("ip link set %s up mtu %d"
                      % (self.ifname, mtu))
            os.system("ip addr add %s/32 dev %s"
                      % (srcip, self.ifname))
            os.system("ip route add %s/32 via %s dev %s"
                      % (dstip, srcip, self.ifname))
        elif PLATFORM == "Darwin":
            os.system("ifconfig %s %s %s up mtu %d"
                      % (self.ifname, srcip, dstip, mtu))


tun_a = TunDevice("tun7")
tun_b = TunDevice("tun8")
tun_a.setIpAddr("172.19.0.2", "172.19.0.1", 1500)
tun_b.setIpAddr("172.20.0.1", "172.20.0.2", 1500)

while 1:
    print "waiting for packet"
    binary_packet = os.read(tun_a.fd, 2048)
    print binary_packet
