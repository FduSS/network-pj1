import os
import fcntl
import time
import platform
import sys
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

    IFF_TUN = 0x0001
    IFF_NO_PI = 0x1000
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

        #fcntl.fcntl(self.fd, fcntl.F_SETFD, fcntl.FD_CLOEXEC)

        if PLATFORM == "Linux":
            fcntl.ioctl(self.fd, self.TUNSETIFF,
                        struct.pack("16sH", ifname, self.IFF_TUN | self.IFF_NO_PI))
        elif PLATFORM == "Darwin":
            pass

    def __del__(self):
        os.close(self.fd)

    def set_ip_addr(self, srcip, dstip, mtu):
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


def calc_checksum(msg, s=0):
    for i in range(0, len(msg), 2):
        s += ord(msg[i])
        if i + 1 < len(msg):
            s += ord(msg[i+1]) << 8
    while s > 0xffff:
        s = (s & 0xffff) + (s >> 16)
    return ~s & 0xffff


def modify_checksum(msg, offset, init=0):
    a = msg[:offset]
    b = msg[offset+2:]
    checksum = calc_checksum(a + b, init)
    return a + struct.pack('H', checksum) + b


class Task:

    ICMP = 0x01
    TCP = 0x06
    UDP = 0x11

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

        src_ip = packet[12:16]
        dst_ip = packet[16:20]
        #sys.stdout.write("%s -> %s" % (to_ip_str(src_ip), to_ip_str(dst_ip)))
        if src_ip != self.remote_ip or dst_ip != self.local_ip:
            print("discard unexcepted ip")
            return

        header_len = (ord(packet[0]) & 0xf) * 4
        packet_len = (ord(packet[2]) << 8)+ ord(packet[3])
        packet = packet[:packet_len]
        ip_header = modify_checksum(packet[:12] + self.nat_src_ip + self.nat_dst_ip + packet[20:header_len], 10)
        packet = packet[header_len:]

        packet_type = ord(ip_header[9])
        if packet_type == self.ICMP:
            #sys.stdout.write(' ICMP\n')
            packet = modify_checksum(packet, 2)
        elif packet_type == self.TCP:
            #sys.stdout.write(' TCP\n')
            pseudo = struct.pack('!4s4sBBH', self.nat_src_ip, self.nat_dst_ip, 0, self.TCP, packet_len - header_len)
            packet = modify_checksum(packet, 16, ~calc_checksum(pseudo) & 0xffff)
        elif packet_type == self.UDP:
            #sys.stdout.write(' UDP\n')
            pseudo = struct.pack('!4s4sBBH', self.nat_src_ip, self.nat_dst_ip, 0, self.UDP, packet_len - header_len)
            packet = modify_checksum(packet, 6, ~calc_checksum(pseudo) & 0xffff)
            #sys.stdout.write(' unknown\n')

        packet = ip_header + packet

        os.write(self.write_fd, packet)


tun_a = TunDevice("tun8")
tun_b = TunDevice("tun9")
tun_a.set_ip_addr("172.19.0.2", "172.19.0.1", MTU)
tun_b.set_ip_addr("172.20.0.1", "172.20.0.2", MTU)

task_a = Task(tun_a.fd, tun_b.fd, "172.19.0.2", "172.19.0.1", "172.20.0.2", "172.20.0.1")
task_b = Task(tun_b.fd, tun_a.fd, "172.20.0.1", "172.20.0.2", "172.19.0.1", "172.19.0.2")


while 1:
    rlist, wlist, xlist = select.select([tun_a.fd, tun_b.fd], [], [], 5)
    for fd in rlist:
        if fd == tun_a.fd:
            task_a.do_transfer()
        if fd == tun_b.fd:
            task_b.do_transfer()

