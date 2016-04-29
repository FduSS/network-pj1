import os
import fcntl
import time
import platform
import struct
import socket
import select

PLATFORM = platform.system()
MTU=1500

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

        self.fd = os.open(TUN_DEVICE, os.O_RDWR | os.O_NONBLOCK)
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

def to_ip_str(data):
    return "%d.%d.%d.%d" % (ord(data[0]), ord(data[1]), ord(data[2]), ord(data[3]))

def parse_ip(ip):
    return ''.join(map(chr, map(int, ip.split('.'))))

class Task:

    def __init__(self, read_fd, write_fd, remote_ip, local_ip, nat_src_ip, nat_dst_ip):
        self.read_fd = read_fd
        self.write_fd = write_fd
        self.remote_ip = parse_ip(remote_ip)
        self.local_ip = parse_ip(local_ip)
        self.nat_src_ip = parse_ip(nat_src_ip)
        self.nat_dst_ip = parse_ip(nat_dst_ip)

    def do_transfer(self):
        # TODO use while with EAGAIN
        packet = os.read(self.read_fd, MTU)
        if len(packet) < 20:
            print('Wrong packet: too short')
            return

        src_ip = packet[16:20]
        dst_ip = packet[20:24]
        print("%s -> %s" % (to_ip_str(src_ip), to_ip_str(dst_ip)))
        if src_ip != self.remote_ip or dst_ip != self.local_ip:
            print("discard unexcepted ip")
            return

        # TODO modify the checksum
        packet = ''.join([packet[:16], self.nat_src_ip, self.nat_dst_ip, packet[24:]])
        os.write(self.write_fd, packet)


tun_a = TunDevice("tun7")
tun_b = TunDevice("tun8")
tun_a.setIpAddr("172.19.0.2", "172.19.0.1", MTU)
tun_b.setIpAddr("172.20.0.1", "172.20.0.2", MTU)

task_a = Task(tun_a.fd, tun_b.fd, "172.19.0.2", "172.19.0.1", "172.20.0.2", "172.20.0.1")
task_b = Task(tun_b.fd, tun_a.fd, "172.20.0.1", "172.20.0.2", "172.19.0.1", "172.19.0.2")


while 1:
    rlist, wlist, xlist = select.select([tun_a.fd, tun_b.fd], [], [], 1)
    print(rlist)
    for fd in rlist:
        if fd == tun_a.fd:
            task_a.do_transfer()
        if fd == tun_b.fd:
            task_b.do_transfer()

