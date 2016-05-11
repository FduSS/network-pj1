# Lab1: Reliable Data Transmission Under High Latency And High Packet Loss Rate Network

Due: __2016.6.5__
Start early! This lab is not easy!

## Goal

In this lab, you need to implement a reliable data transmission protocol like TCP. This lab will emulate a high latency and high packet loss rate network environment, and your goal is to maximise the transfer speed under the network.


## How this lab works

### Overview

In short, the data is transferred with the following steps:

1. A program tries to open a __TCP__ connection to `127.0.0.1:7000` , and sends __HTTP GET__ request

2. Your program __Alice__ listened on `127.0.0.1:7000` , accepted the connection, then send data to `172.19.0.1` . It's the IP address of a created TUN device __tunA__

3. The transfer program (provided by this lab) transfers your packet to __tunB__, and the destination IP is change to `172.20.0.1`

4. Your program __Bob__ listened on `172.20.0.1` , get the request from __Alice__, then proxy the TCP connection to `127.0.0.1:8000`

5. There is a HTTP server (or whatever) accepts connection at `127.0.0.1:8000`, replies to the HTTP request. 

Then, __Bob__ should deliver the data from `172.20.0.1` to `172.20.0.2` on __tunB__. The transfer program transforms the packet, from `172.19.0.1` to `172.19.0.2`. __Alice__ will get the packet on __tunA__, then she need to write data back to the origin TCP connection.

You need to implement the __Alice__ and __Bob__. They should be the same program launched with different arguments:
```sh
./lab1 --alice
./lab1 --bob
```

### How TUN works
__TUN__ and __TAP__ are virtual network kernel devices. It's a network interface emulated by software. The program could read and write packets on the interface.

In this lab, the transfer program will create two virtual interface __tunA__ and __tunB__:

__tunA__:  self ip `172.19.0.2`, peer ip `172.19.0.1`
__tunB__:  self ip `172.20.0.1`, peer ip `172.20.0.2`

To explain how the transfer program works, you can try to ping `172.19.0.1`.
```
$ ping 172.19.0.1
PING 172.19.0.1 (172.19.0.1): 56 data bytes
64 bytes from 172.19.0.1: icmp_seq=0 ttl=64 time=0.603 ms
```

If you do a __tcpdump__ on __tunA__, you'll get
```
$ sudo tcpdump -i tunA -n
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on tun8, link-type NULL (BSD loopback), capture size 262144 bytes
22:35:38.209997 IP 172.19.0.2 > 172.19.0.1: ICMP echo request, id 47671, seq 0, length 64
22:35:38.210310 IP 172.19.0.1 > 172.19.0.2: ICMP echo reply, id 47671, seq 0, length 64
```

And on __tunB__:
```
$ sudo tcpdump -i tun9  -n
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on tun9, link-type NULL (BSD loopback), capture size 262144 bytes
22:35:38.210106 IP 172.20.0.2 > 172.20.0.1: ICMP echo request, id 47671, seq 0, length 64
22:35:38.210183 IP 172.20.0.1 > 172.20.0.2: ICMP echo reply, id 47671, seq 0, length 64
```

You can sort up the four packets by their send time:
```
172.19.0.2 > 172.19.0.1, ICMP request on tunA

< transfer from tunA to tunB >

172.20.0.2 > 172.20.0.1, ICMP request on tunB

< system replies to ICMP ping >

172.20.0.1 > 172.20.0.2, ICMP reply on tunB

< transfer from tunB to tunA >

172.19.0.1 > 172.19.0.2, ICMP reply on tunB
```

In this lab, __Alice__ should communicate with __tunA__ only, and __Bob__ should communicate with __tunB__ only. They are not allowed to communicate directly. Direct connection will be restricted on testing by TA. They can use any transfer layer protocol on __tunA__ and __tunB__ such as __TCP__, __UDP__, __ICMP__. The transfer program will emulate latency and packet loss on __tunA__ and __tunB__.

## Environment setting up
You can finish this lab on __Linux__, or __OSX__, or __Windows__. Just choose the platform you want.

First, fetch the material from:
```
https://git.oschina.net/nullmdr/network-pj1
```
Or you can
```
git clone https://git.oschina.net/nullmdr/network-pj1.git
```

This lab provides the transfer program with a __CMakeFiles__. It's a cross platform build script. I've built the binary for you in `Release` Folder. However some extra steps are needed for some platforms.

### Linux
Launch the binary with `sudo`:
```
$ sudo ./Release/transfer-linux -h
```

### Mac OSX
You need to install __tuntaposx__ from:
```
http://tuntaposx.sourceforge.net/download.xhtml
```
Then, launch the binary with `sudo`:
```
$ sudo ./Release/transfer-osx -t 10
```

If you have issue about root permission, see:
```
http://osxdaily.com/2015/10/05/disable-rootless-system-integrity-protection-mac-os-x/
```

### Windows
You need to install __tap-windows__. In `tap-win64` folder, run `addtap.bat`. Then, go to `Control Pannel - Network and Sharing Center - Change adapter settings`, rename the names of the two added __TAP-win32__ adapter: rename __#2__ to __tunA__, rename the other to __tunB__.

Then, launch the admin shell via `Release/windows-run.bat`. Launch __transfer__ in the admin shell:
```
> transfer -l 100
```

#### Tips:
The __tap-windows__ is a component of __OPENVPN__. You can download it (not required for this lab) on
```
https://openvpn.net/index.php/open-source/downloads.html
```
__tap-windows__ install binary is at the bottom of the page. __Notice__: This lab requires two tun devices, so you need to manually install one in addiction after the installation if you want to install it from the __OPENVPN__ website.

## How to start this lab

### Run download test
First, you need a HTTP server for speed test. You can use nginx. In fact you can use any http server you like. I suggest nginx with the following configuration
```
worker_processes  4;

events {
    worker_connections  1024;
}

http {
    include       mime.types;
    default_type  application/octet-stream;
    sendfile      on;
    keepalive_timeout  65;
    
    server {
	    # Change it to 127.0.0.1:8000 after finishing this lab
        listen       8000;

        location / {
	        # Change it to your test dir
            root   html; 
        }

    }
}
```
Then, in the test dir, generate a 100M test file (Windows user can use __Cygwin__ or __MinGW__)
```
$ dd if=/dev/urandom of=100M bs=1M count=100
```
For Mac OSX, you need to change `bs=1M` to `bs=1m`.

After __nginx__ and __transfer__ running up, you can test the transfer bridge
```
$ wget 172.19.0.1:8000/100M -O 100M.test
$ diff 100M 100M.test
```
After you finishing up this lab, you can test it in several ways
```
$ dd if=/dev/urandom of=1G bs=1M count=1024
$ wget 127.0.0.1:7000/1G -O 1G.test
$ diff 1G 1G.test

Test for multi thread downloading
$ axel -n 64 127.0.0.1:7000/1G -O 1G.test
$ mwget -n=128 127.0.0.1:7000/1G -O 1G.test
```

### TCP Connection Proxy
This lab requires you to implement a TCP connection proxy in this lab. __Alice__ should listen on 127.0.0.1:7000, and __Bob__ should proxy all the connection to 127.0.0.1:8000. They communicate via tunA and tunB, or, __Alice__ should only listen on 172.19.0.2 and send packet to 172.19.0.1, while __Bob__ should only listen on 172.20.0.1 and send packet to 172.20.0.2. The address in the packet will be translated by __transfer__. Afterward __transfer__ will send the transformed packet to the other tun interface. You can assume that for __Alice__, __Bob__ is on 172.19.0.1; for __Bob__, __Alice__ is on 172.20.0.2.

They _should_ work correctly under multithread downloading. Use __diff__ to check the correctness of data transfer. 

### TCP Congestion Control Algorithms
__transfer__ can emulate a high latency, high packet loss rate network. As you can see, the system's default congestion control algorithm doesn't work well in this situation. __You need to implement a congestion control algorithm in this lab.__ Maybe you can start up here:
```
https://en.wikipedia.org/wiki/TCP_congestion_control#Algorithms
```
You can try different algorithms, benchmark them, choose the best one, or implement one yourself.

### Coding
This lab doesn't limit the platform or programming language you use. Please write down how to build your program in README. Add your test platform, compiler version and computer configuration to README, too.
Describe how do you implement this lab (e.g. which algorithm to use, why) in README.

MarkDown or PDF format is preferred for README. Please submit all of your source code and your README doc to ftp before due. Make a tarball or zip file like `13302010023_何天成.tar.gz` before submission.

### Testing and Grading
#### 30%
Sanity test. Your code should work correctly for single-thread and multi-thread downloading. Download speed should reach and keep at least 75% of the limit speed (option `-l`) with no delay and packet loss.

#### 70%
Performance test. Your code will be tested under vary of configurations. Limit: delay will not exceed 300ms (rtt ~600ms), speed limit will not exceed 100Mbps, delay thrashing will not exceed 50%, packet loss rate: ???SECRET
The one wins the best performance will get full score in this part. The others' scores  are based on their code's performance and the best performance of all.


## How to build the transfer program
If you have trouble running the binary, or maybe you are interested in how to build the binary, here is the instruction:

### Linux
Install __cmake__. If you are using __Ubuntu__:
```
$ sudo apt-get install cmake
```

Then, you can build the program
```
$ cmake .
$ make
```

### Mac OSX
Install build toolchain:
```
$ xcode-select --install
```

Install __cmake__ via __homebrew__:
```
$ /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
$ brew install cmake
```

Then, build:
```
$ cmake .
$ make
```

### Windows
Install __cmake__
```
https://cmake.org/
```
Install __Visual Studio 2015__
```
http://mvls.fudan.edu.cn/develope%20tools/SW_DVD9_NTRL_Visual_Studio_Ent_2015_ChnSimp_FPP_VL_MLF_X20-29937.ISO
```
Build in __VS2015 MSBuild Command-line__
```
> cmake .
> msbuild ALL_BUILD.vcxproj /p:Configuration=Release
```