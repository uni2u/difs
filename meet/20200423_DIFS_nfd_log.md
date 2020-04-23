# DIFS NFD log

multi DIFS 구성 후 각 NODE 에서 발생한 NFD 로그는 다음과 같다.

## 환경구성

* repo0
  * IP: 100.0.0.2

```
repo0.conf

repo
{
  data
  {
    registration-subset 1
    prefix "ndn:/data"
  }

  command
  {
    prefix "ndn:/etri/repo"
  }

  cluster
  {
    size 3
    id 0
    prefix "ndn:/etri/repo"
  }

  storage
  {
    method "fs"              ; Currently, only sqlite storage engine is supported
    path "/var/lib/ndn/repo/0"  ; Path to repo-ng storage folder
    max-packets 2147483648
  }

  tcp_bulk_insert
  {
    ; host "localhost"  ; Set to listen on a different IP address or hostname
    ; port 7376         ; Set to listen on a different port number
  }
  
  validator
  {
    ; The following rule disables all security in the repo
    trust-anchor
    {
      type any
    }

  }
}
```

* repo1
  * IP: 100.0.0.3

```
repo0.conf

repo
{
  data
  {
    registration-subset 1
    prefix "ndn:/data"
  }

  command
  {
    prefix "ndn:/etri/repo"
  }

  cluster
  {
    size 3
    id 1
    prefix "ndn:/etri/repo"
  }

  storage
  {
    method "fs"              ; Currently, only sqlite storage engine is supported
    path "/var/lib/ndn/repo/1"  ; Path to repo-ng storage folder
    max-packets 2147483648
  }

  tcp_bulk_insert
  {
    ; host "localhost"  ; Set to listen on a different IP address or hostname
    ; port 7376         ; Set to listen on a different port number
  }
  
  validator
  {
    ; The following rule disables all security in the repo
    trust-anchor
    {
      type any
    }

  }
}
```

* repo2
  * IP: 100.0.0.4

```
repo0.conf

repo
{
  data
  {
    registration-subset 1
    prefix "ndn:/data"
  }

  command
  {
    prefix "ndn:/etri/repo"
  }

  cluster
  {
    size 3
    id 2
    prefix "ndn:/etri/repo"
  }

  storage
  {
    method "fs"              ; Currently, only sqlite storage engine is supported
    path "/var/lib/ndn/repo/2"  ; Path to repo-ng storage folder
    max-packets 2147483648
  }

  tcp_bulk_insert
  {
    ; host "localhost"  ; Set to listen on a different IP address or hostname
    ; port 7376         ; Set to listen on a different port number
  }
  
  validator
  {
    ; The following rule disables all security in the repo
    trust-anchor
    {
      type any
    }

  }
}
```

## NFD 실행

### repo0

```
1587598832.654212 INFO: [Main] NFD version 0.6.2 starting
1587598832.654242 INFO: [Main] Built with GNU C++ version 7.5.0, with GNU libstdc++ version 20191114, with Boost version 1.65.1, with libpcap version 1.8.1, with WebSocket++ version 0.7.0, with ndn-cxx version 0.6.2
1587598832.949776 INFO: [CsPolicy] setLimit 10
1587598832.949871 INFO: [StrategyChoice] setDefaultStrategy /localhost/nfd/strategy/best-route/%FD%05
1587598832.951854 INFO: [InternalForwarderTransport] [id=0,local=null://,remote=null://] Creating transport
1587598832.951889 INFO: [FaceTable] Added face id=255 remote=null:// local=null://
1587598832.951908 INFO: [InternalForwarderTransport] [id=0,local=contentstore://,remote=contentstore://] Creating transport
1587598832.951917 INFO: [FaceTable] Added face id=254 remote=contentstore:// local=contentstore://
1587598832.951974 INFO: [InternalForwarderTransport] [id=0,local=internal://,remote=internal://] Creating transport
1587598832.954328 INFO: [FaceTable] Added face id=1 remote=internal:// local=internal://
1587598832.955842 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598832.956306 INFO: [StrategyChoice] changeStrategy(/ndn/broadcast) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598832.956354 INFO: [StrategyChoice] changeStrategy(/localhost) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598832.956389 INFO: [StrategyChoice] changeStrategy(/localhost/nfd) /localhost/nfd/strategy/multicast/%FD%03 -> /localhost/nfd/strategy/best-route/%FD%05
1587598832.956427 INFO: [CsPolicy] setLimit 65536
1587598832.956436 INFO: [CsPolicy] setLimit 65536
1587598832.956462 INFO: [EthernetFactory] enabling multicast on 01:00:5e:00:17:aa
1587598832.956527 INFO: [TcpChannel] [tcp4://0.0.0.0:6363] Creating channel
1587598832.956627 INFO: [TcpChannel] [tcp6://[::]:6363] Creating channel
1587598832.956799 INFO: [UdpChannel] [udp4://0.0.0.0:6363] Creating channel
1587598832.956874 INFO: [UdpChannel] [udp6://[::]:6363] Creating channel
1587598832.956951 INFO: [UdpFactory] enabling multicast on 224.0.23.170:56363
1587598832.956977 INFO: [UdpFactory] enabling multicast on [ff02::1234]:56363
1587598832.957123 INFO: [UnixStreamChannel] [unix:///run/nfd.sock] Creating channel
1587598832.957315 FATAL: [Main] bind: Permission denied
uni2u@sandbox03:~/difs$ sudo su
root@sandbox03:/home/uni2u/difs# nfd
1587598849.703710 INFO: [Main] NFD version 0.6.2 starting
1587598849.703723 INFO: [Main] Built with GNU C++ version 7.5.0, with GNU libstdc++ version 20191114, with Boost version 1.65.1, with libpcap version 1.8.1, with WebSocket++ version 0.7.0, with ndn-cxx version 0.6.2
1587598849.707778 INFO: [CsPolicy] setLimit 10
1587598849.707832 INFO: [StrategyChoice] setDefaultStrategy /localhost/nfd/strategy/best-route/%FD%05
1587598849.708009 INFO: [InternalForwarderTransport] [id=0,local=null://,remote=null://] Creating transport
1587598849.708025 INFO: [FaceTable] Added face id=255 remote=null:// local=null://
1587598849.708037 INFO: [InternalForwarderTransport] [id=0,local=contentstore://,remote=contentstore://] Creating transport
1587598849.708041 INFO: [FaceTable] Added face id=254 remote=contentstore:// local=contentstore://
1587598849.708066 INFO: [InternalForwarderTransport] [id=0,local=internal://,remote=internal://] Creating transport
1587598849.708130 INFO: [FaceTable] Added face id=1 remote=internal:// local=internal://
1587598849.708589 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598849.708802 INFO: [StrategyChoice] changeStrategy(/ndn/broadcast) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598849.708830 INFO: [StrategyChoice] changeStrategy(/localhost) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598849.708849 INFO: [StrategyChoice] changeStrategy(/localhost/nfd) /localhost/nfd/strategy/multicast/%FD%03 -> /localhost/nfd/strategy/best-route/%FD%05
1587598849.708860 INFO: [CsPolicy] setLimit 65536
1587598849.708865 INFO: [CsPolicy] setLimit 65536
1587598849.708879 INFO: [EthernetFactory] enabling multicast on 01:00:5e:00:17:aa
1587598849.708909 INFO: [TcpChannel] [tcp4://0.0.0.0:6363] Creating channel
1587598849.708963 INFO: [TcpChannel] [tcp6://[::]:6363] Creating channel
1587598849.709024 INFO: [UdpChannel] [udp4://0.0.0.0:6363] Creating channel
1587598849.709056 INFO: [UdpChannel] [udp6://[::]:6363] Creating channel
1587598849.709077 INFO: [UdpFactory] enabling multicast on 224.0.23.170:56363
1587598849.709089 INFO: [UdpFactory] enabling multicast on [ff02::1234]:56363
1587598849.709142 INFO: [UnixStreamChannel] [unix:///run/nfd.sock] Creating channel
1587598849.709213 INFO: [WebSocketChannel] [ws://[::]:9696] Creating channel
1587598849.709550 INFO: [CommandAuthenticator] clear-authorizations
1587598849.709580 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598849.709594 INFO: [CommandAuthenticator] authorize module=faces signer=any
1587598849.709601 INFO: [CommandAuthenticator] authorize module=fib signer=any
1587598849.709605 INFO: [CommandAuthenticator] authorize module=cs signer=any
1587598849.709608 INFO: [CommandAuthenticator] authorize module=strategy-choice signer=any
1587598849.709813 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598849.710197 INFO: [EthernetChannel] [dev://enp5s0f0] Creating channel
1587598849.712514 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598849.712864 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598849.719846 INFO: [RibManager] Start monitoring face create/destroy events
1587598849.804648 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598849.804970 INFO: [FaceTable] Added face id=256 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f0
1587598849.808747 INFO: [EthernetChannel] [dev://enp5s0f1] Creating channel
1587598849.884640 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598849.884961 INFO: [FaceTable] Added face id=257 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f1
1587598849.887006 INFO: [EthernetChannel] [dev://eno1] Creating channel
1587598849.956639 INFO: [MulticastEthernetTransport] [id=0,local=dev://eno1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598849.956960 INFO: [FaceTable] Added face id=258 remote=ether://[01:00:5e:00:17:aa] local=dev://eno1
1587598849.959098 INFO: [EthernetChannel] [dev://enp5s0f2] Creating channel
1587598850.020639 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.020959 INFO: [FaceTable] Added face id=259 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f2
1587598850.022992 INFO: [EthernetChannel] [dev://enp5s0f3] Creating channel
1587598850.084638 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.084962 INFO: [FaceTable] Added face id=260 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f3
1587598850.086991 INFO: [EthernetChannel] [dev://ens6f0] Creating channel
1587598850.156639 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.156951 INFO: [FaceTable] Added face id=261 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f0
1587598850.158967 INFO: [EthernetChannel] [dev://ens6f1] Creating channel
1587598850.228638 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.228988 INFO: [FaceTable] Added face id=262 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f1
1587598850.231002 INFO: [EthernetChannel] [dev://ens6f2] Creating channel
1587598850.304641 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.304988 INFO: [FaceTable] Added face id=263 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f2
1587598850.307020 INFO: [EthernetChannel] [dev://ens6f3] Creating channel
1587598850.380639 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.380969 INFO: [FaceTable] Added face id=264 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f3
1587598850.382984 INFO: [EthernetChannel] [dev://docker0] Creating channel
1587598850.444637 INFO: [MulticastEthernetTransport] [id=0,local=dev://docker0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598850.444924 INFO: [FaceTable] Added face id=265 remote=ether://[01:00:5e:00:17:aa] local=dev://docker0
1587598850.446980 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://63] Creating transport
1587598850.447011 INFO: [FaceTable] Added face id=266 remote=fd://63 local=unix:///run/nfd.sock
1587598850.449585 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.449672 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.449750 INFO: [MulticastUdpTransport] [id=0,local=udp4://publicIP:54857,remote=udp4://224.0.23.170:56363] Creating transport
1587598850.449801 INFO: [FaceTable] Added face id=267 remote=udp4://224.0.23.170:56363 local=udp4://publicIP:54857
1587598850.451518 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.451599 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.451660 INFO: [MulticastUdpTransport] [id=0,local=udp4://192.0.0.2:56667,remote=udp4://224.0.23.170:56363] Creating transport
1587598850.451677 INFO: [FaceTable] Added face id=268 remote=udp4://224.0.23.170:56363 local=udp4://192.0.0.2:56667
1587598850.453461 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.453544 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.453605 INFO: [MulticastUdpTransport] [id=0,local=udp4://100.0.0.2:40569,remote=udp4://224.0.23.170:56363] Creating transport
1587598850.453625 INFO: [FaceTable] Added face id=269 remote=udp4://224.0.23.170:56363 local=udp4://100.0.0.2:40569
1587598850.455337 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.455411 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.455470 INFO: [MulticastUdpTransport] [id=0,local=udp4://172.17.0.1:60505,remote=udp4://224.0.23.170:56363] Creating transport
1587598850.455487 INFO: [FaceTable] Added face id=270 remote=udp4://224.0.23.170:56363 local=udp4://172.17.0.1:60505
1587598850.460748 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.460826 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.460921 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::325a:3aff:fee1:8426%eno1]:60061,remote=udp6://[ff02::1234%eno1]:56363] Creating transport
1587598850.460934 INFO: [FaceTable] Added face id=271 remote=udp6://[ff02::1234%eno1]:56363 local=udp6://[fe80::325a:3aff:fee1:8426%eno1]:60061
1587598850.461832 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.461891 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.461954 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::6e61:98a3:d22d:d61e%enp5s0f3]:56728,remote=udp6://[ff02::1234%enp5s0f3]:56363] Creating transport
1587598850.461966 INFO: [FaceTable] Added face id=272 remote=udp6://[ff02::1234%enp5s0f3]:56363 local=udp6://[fe80::6e61:98a3:d22d:d61e%enp5s0f3]:56728
1587598850.462825 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598850.462888 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598850.462941 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::4fba:3813:6c83:ce7c%ens6f3]:52693,remote=udp6://[ff02::1234%ens6f3]:56363] Creating transport
1587598850.462953 INFO: [FaceTable] Added face id=273 remote=udp6://[ff02::1234%ens6f3]:56363 local=udp6://[fe80::4fba:3813:6c83:ce7c%ens6f3]:52693
1587598850.463872 INFO: [AutoPrefixPropagator] local registration only for /localhost/nfd
```

### repo1

```
1587598846.791960 INFO: [Main] NFD version 0.6.2 starting
1587598846.791972 INFO: [Main] Built with GNU C++ version 7.5.0, with GNU libstdc++ version 20191114, with Boost version 1.65.1, with libpcap version 1.8.1, with WebSocket++ version 0.7.0, with ndn-cxx version 0.6.2
1587598846.793918 INFO: [CsPolicy] setLimit 10
1587598846.793970 INFO: [StrategyChoice] setDefaultStrategy /localhost/nfd/strategy/best-route/%FD%05
1587598846.794132 INFO: [InternalForwarderTransport] [id=0,local=null://,remote=null://] Creating transport
1587598846.794146 INFO: [FaceTable] Added face id=255 remote=null:// local=null://
1587598846.794157 INFO: [InternalForwarderTransport] [id=0,local=contentstore://,remote=contentstore://] Creating transport
1587598846.794162 INFO: [FaceTable] Added face id=254 remote=contentstore:// local=contentstore://
1587598846.794184 INFO: [InternalForwarderTransport] [id=0,local=internal://,remote=internal://] Creating transport
1587598846.794250 INFO: [FaceTable] Added face id=1 remote=internal:// local=internal://
1587598846.794627 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598846.794819 INFO: [StrategyChoice] changeStrategy(/ndn/broadcast) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598846.794844 INFO: [StrategyChoice] changeStrategy(/localhost) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598846.794865 INFO: [StrategyChoice] changeStrategy(/localhost/nfd) /localhost/nfd/strategy/multicast/%FD%03 -> /localhost/nfd/strategy/best-route/%FD%05
1587598846.794883 INFO: [CsPolicy] setLimit 65536
1587598846.794888 INFO: [CsPolicy] setLimit 65536
1587598846.794900 INFO: [EthernetFactory] enabling multicast on 01:00:5e:00:17:aa
1587598846.794961 INFO: [TcpChannel] [tcp4://0.0.0.0:6363] Creating channel
1587598846.795016 INFO: [TcpChannel] [tcp6://[::]:6363] Creating channel
1587598846.795077 INFO: [UdpChannel] [udp4://0.0.0.0:6363] Creating channel
1587598846.795115 INFO: [UdpChannel] [udp6://[::]:6363] Creating channel
1587598846.795141 INFO: [UdpFactory] enabling multicast on 224.0.23.170:56363
1587598846.795156 INFO: [UdpFactory] enabling multicast on [ff02::1234]:56363
1587598846.795213 INFO: [UnixStreamChannel] [unix:///run/nfd.sock] Creating channel
1587598846.795283 INFO: [WebSocketChannel] [ws://[::]:9696] Creating channel
1587598846.795347 INFO: [CommandAuthenticator] clear-authorizations
1587598846.795388 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598846.795401 INFO: [CommandAuthenticator] authorize module=faces signer=any
1587598846.795408 INFO: [CommandAuthenticator] authorize module=fib signer=any
1587598846.795415 INFO: [CommandAuthenticator] authorize module=cs signer=any
1587598846.795418 INFO: [CommandAuthenticator] authorize module=strategy-choice signer=any
1587598846.795584 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598846.795976 INFO: [EthernetChannel] [dev://enp5s0f0] Creating channel
1587598846.798255 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598846.798591 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598846.804496 INFO: [RibManager] Start monitoring face create/destroy events
1587598846.868285 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598846.868568 INFO: [FaceTable] Added face id=256 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f0
1587598846.872638 INFO: [EthernetChannel] [dev://enp5s0f1] Creating channel
1587598846.944281 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598846.944609 INFO: [FaceTable] Added face id=257 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f1
1587598846.946600 INFO: [EthernetChannel] [dev://eno1] Creating channel
1587598847.020281 INFO: [MulticastEthernetTransport] [id=0,local=dev://eno1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.020578 INFO: [FaceTable] Added face id=258 remote=ether://[01:00:5e:00:17:aa] local=dev://eno1
1587598847.022672 INFO: [EthernetChannel] [dev://enp5s0f2] Creating channel
1587598847.092282 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.092595 INFO: [FaceTable] Added face id=259 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f2
1587598847.094580 INFO: [EthernetChannel] [dev://enp5s0f3] Creating channel
1587598847.160283 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.160600 INFO: [FaceTable] Added face id=260 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f3
1587598847.162573 INFO: [EthernetChannel] [dev://ens6f0] Creating channel
1587598847.228281 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.228615 INFO: [FaceTable] Added face id=261 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f0
1587598847.230587 INFO: [EthernetChannel] [dev://ens6f1] Creating channel
1587598847.300282 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.300607 INFO: [FaceTable] Added face id=262 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f1
1587598847.302586 INFO: [EthernetChannel] [dev://ens6f2] Creating channel
1587598847.372285 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.372594 INFO: [FaceTable] Added face id=263 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f2
1587598847.374569 INFO: [EthernetChannel] [dev://ens6f3] Creating channel
1587598847.448247 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.448559 INFO: [FaceTable] Added face id=264 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f3
1587598847.450527 INFO: [EthernetChannel] [dev://docker0] Creating channel
1587598847.512280 INFO: [MulticastEthernetTransport] [id=0,local=dev://docker0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598847.512562 INFO: [FaceTable] Added face id=265 remote=ether://[01:00:5e:00:17:aa] local=dev://docker0
1587598847.514565 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://63] Creating transport
1587598847.514590 INFO: [FaceTable] Added face id=266 remote=fd://63 local=unix:///run/nfd.sock
1587598847.517180 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.517267 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.517357 INFO: [MulticastUdpTransport] [id=0,local=udp4://publicIP:36539,remote=udp4://224.0.23.170:56363] Creating transport
1587598847.517381 INFO: [FaceTable] Added face id=267 remote=udp4://224.0.23.170:56363 local=udp4://publicIP:36539
1587598847.519457 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.519541 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.519613 INFO: [MulticastUdpTransport] [id=0,local=udp4://192.0.0.3:42012,remote=udp4://224.0.23.170:56363] Creating transport
1587598847.519634 INFO: [FaceTable] Added face id=268 remote=udp4://224.0.23.170:56363 local=udp4://192.0.0.3:42012
1587598847.521702 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.521791 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.521861 INFO: [MulticastUdpTransport] [id=0,local=udp4://100.0.0.3:52614,remote=udp4://224.0.23.170:56363] Creating transport
1587598847.521885 INFO: [FaceTable] Added face id=269 remote=udp4://224.0.23.170:56363 local=udp4://100.0.0.3:52614
1587598847.523913 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.523999 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.524092 INFO: [MulticastUdpTransport] [id=0,local=udp4://172.17.0.1:42636,remote=udp4://224.0.23.170:56363] Creating transport
1587598847.524106 INFO: [FaceTable] Added face id=270 remote=udp4://224.0.23.170:56363 local=udp4://172.17.0.1:42636
1587598847.526551 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.526612 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.526707 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::325a:3aff:fee1:876e%eno1]:49557,remote=udp6://[ff02::1234%eno1]:56363] Creating transport
1587598847.526720 INFO: [FaceTable] Added face id=271 remote=udp6://[ff02::1234%eno1]:56363 local=udp6://[fe80::325a:3aff:fee1:876e%eno1]:49557
1587598847.527533 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.527592 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.527649 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::b48d:a093:2f33:4b3a%enp5s0f3]:44574,remote=udp6://[ff02::1234%enp5s0f3]:56363] Creating transport
1587598847.527661 INFO: [FaceTable] Added face id=272 remote=udp6://[ff02::1234%enp5s0f3]:56363 local=udp6://[fe80::b48d:a093:2f33:4b3a%enp5s0f3]:44574
1587598847.528579 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598847.528639 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598847.528694 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::bbc6:2bc8:dc00:6ad7%ens6f3]:50378,remote=udp6://[ff02::1234%ens6f3]:56363] Creating transport
1587598847.528706 INFO: [FaceTable] Added face id=273 remote=udp6://[ff02::1234%ens6f3]:56363 local=udp6://[fe80::bbc6:2bc8:dc00:6ad7%ens6f3]:50378
1587598847.529570 INFO: [AutoPrefixPropagator] local registration only for /localhost/nfd
```

### repo2

```
1587598844.078535 INFO: [Main] NFD version 0.6.2 starting
1587598844.078563 INFO: [Main] Built with GNU C++ version 7.5.0, with GNU libstdc++ version 20191114, with Boost version 1.65.1, with libpcap version 1.8.1, with WebSocket++ version 0.7.0, with ndn-cxx version 0.6.2
1587598844.083651 INFO: [CsPolicy] setLimit 10
1587598844.083726 INFO: [StrategyChoice] setDefaultStrategy /localhost/nfd/strategy/best-route/%FD%05
1587598844.085497 INFO: [InternalForwarderTransport] [id=0,local=null://,remote=null://] Creating transport
1587598844.085524 INFO: [FaceTable] Added face id=255 remote=null:// local=null://
1587598844.085538 INFO: [InternalForwarderTransport] [id=0,local=contentstore://,remote=contentstore://] Creating transport
1587598844.085544 INFO: [FaceTable] Added face id=254 remote=contentstore:// local=contentstore://
1587598844.085581 INFO: [InternalForwarderTransport] [id=0,local=internal://,remote=internal://] Creating transport
1587598844.087577 INFO: [FaceTable] Added face id=1 remote=internal:// local=internal://
1587598844.088789 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598844.089081 INFO: [StrategyChoice] changeStrategy(/ndn/broadcast) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598844.089118 INFO: [StrategyChoice] changeStrategy(/localhost) /localhost/nfd/strategy/best-route/%FD%05 -> /localhost/nfd/strategy/multicast/%FD%03
1587598844.089139 INFO: [StrategyChoice] changeStrategy(/localhost/nfd) /localhost/nfd/strategy/multicast/%FD%03 -> /localhost/nfd/strategy/best-route/%FD%05
1587598844.089152 INFO: [CsPolicy] setLimit 65536
1587598844.089159 INFO: [CsPolicy] setLimit 65536
1587598844.089176 INFO: [EthernetFactory] enabling multicast on 01:00:5e:00:17:aa
1587598844.089239 INFO: [TcpChannel] [tcp4://0.0.0.0:6363] Creating channel
1587598844.089297 INFO: [TcpChannel] [tcp6://[::]:6363] Creating channel
1587598844.089367 INFO: [UdpChannel] [udp4://0.0.0.0:6363] Creating channel
1587598844.089408 INFO: [UdpChannel] [udp6://[::]:6363] Creating channel
1587598844.089447 INFO: [UdpFactory] enabling multicast on 224.0.23.170:56363
1587598844.089465 INFO: [UdpFactory] enabling multicast on [ff02::1234]:56363
1587598844.089544 INFO: [UnixStreamChannel] [unix:///run/nfd.sock] Creating channel
1587598844.089630 INFO: [WebSocketChannel] [ws://[::]:9696] Creating channel
1587598844.089949 INFO: [CommandAuthenticator] clear-authorizations
1587598844.090003 WARNING: [CommandAuthenticator] 'certfile any' is intended for demo purposes only and SHOULD NOT be used in production environments
1587598844.090020 INFO: [CommandAuthenticator] authorize module=faces signer=any
1587598844.090027 INFO: [CommandAuthenticator] authorize module=fib signer=any
1587598844.090032 INFO: [CommandAuthenticator] authorize module=cs signer=any
1587598844.090036 INFO: [CommandAuthenticator] authorize module=strategy-choice signer=any
1587598844.090241 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.090683 INFO: [EthernetChannel] [dev://enp5s0f0] Creating channel
1587598844.092959 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598844.093309 INFO: [AutoPrefixPropagator] Load auto_prefix_propagate section in rib section
1587598844.100868 INFO: [RibManager] Start monitoring face create/destroy events
1587598844.172405 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.172704 INFO: [FaceTable] Added face id=256 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f0
1587598844.177179 INFO: [EthernetChannel] [dev://enp5s0f1] Creating channel
1587598844.260417 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.260705 INFO: [FaceTable] Added face id=257 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f1
1587598844.262337 INFO: [EthernetChannel] [dev://eno1] Creating channel
1587598844.336413 INFO: [MulticastEthernetTransport] [id=0,local=dev://eno1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.336700 INFO: [FaceTable] Added face id=258 remote=ether://[01:00:5e:00:17:aa] local=dev://eno1
1587598844.338813 INFO: [EthernetChannel] [dev://enp5s0f2] Creating channel
1587598844.408407 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.408704 INFO: [FaceTable] Added face id=259 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f2
1587598844.410719 INFO: [EthernetChannel] [dev://enp5s0f3] Creating channel
1587598844.472403 INFO: [MulticastEthernetTransport] [id=0,local=dev://enp5s0f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.472708 INFO: [FaceTable] Added face id=260 remote=ether://[01:00:5e:00:17:aa] local=dev://enp5s0f3
1587598844.474716 INFO: [EthernetChannel] [dev://ens6f0] Creating channel
1587598844.540415 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.540729 INFO: [FaceTable] Added face id=261 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f0
1587598844.542587 INFO: [EthernetChannel] [dev://ens6f1] Creating channel
1587598844.624416 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f1,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.624733 INFO: [FaceTable] Added face id=262 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f1
1587598844.626260 INFO: [EthernetChannel] [dev://ens6f2] Creating channel
1587598844.708416 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f2,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.708710 INFO: [FaceTable] Added face id=263 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f2
1587598844.710609 INFO: [EthernetChannel] [dev://ens6f3] Creating channel
1587598844.780403 INFO: [MulticastEthernetTransport] [id=0,local=dev://ens6f3,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.780710 INFO: [FaceTable] Added face id=264 remote=ether://[01:00:5e:00:17:aa] local=dev://ens6f3
1587598844.782712 INFO: [EthernetChannel] [dev://docker0] Creating channel
1587598844.860434 INFO: [MulticastEthernetTransport] [id=0,local=dev://docker0,remote=ether://[01:00:5e:00:17:aa]] Creating transport
1587598844.860743 INFO: [FaceTable] Added face id=265 remote=ether://[01:00:5e:00:17:aa] local=dev://docker0
1587598844.862782 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://63] Creating transport
1587598844.862808 INFO: [FaceTable] Added face id=266 remote=fd://63 local=unix:///run/nfd.sock
1587598844.865475 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.865568 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.865663 INFO: [MulticastUdpTransport] [id=0,local=udp4://publicIP:41034,remote=udp4://224.0.23.170:56363] Creating transport
1587598844.865687 INFO: [FaceTable] Added face id=267 remote=udp4://224.0.23.170:56363 local=udp4://publicIP:41034
1587598844.867833 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.867918 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.867991 INFO: [MulticastUdpTransport] [id=0,local=udp4://192.0.0.4:48245,remote=udp4://224.0.23.170:56363] Creating transport
1587598844.868012 INFO: [FaceTable] Added face id=268 remote=udp4://224.0.23.170:56363 local=udp4://192.0.0.4:48245
1587598844.869039 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.869121 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.869191 INFO: [MulticastUdpTransport] [id=0,local=udp4://100.0.0.4:35888,remote=udp4://224.0.23.170:56363] Creating transport
1587598844.869214 INFO: [FaceTable] Added face id=269 remote=udp4://224.0.23.170:56363 local=udp4://100.0.0.4:35888
1587598844.870801 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.870876 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.870934 INFO: [MulticastUdpTransport] [id=0,local=udp4://172.17.0.1:59115,remote=udp4://224.0.23.170:56363] Creating transport
1587598844.870954 INFO: [FaceTable] Added face id=270 remote=udp4://224.0.23.170:56363 local=udp4://172.17.0.1:59115
1587598844.874191 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.874257 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.874361 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::fa32:e4ff:fe6d:f270%eno1]:48860,remote=udp6://[ff02::1234%eno1]:56363] Creating transport
1587598844.874378 INFO: [FaceTable] Added face id=271 remote=udp6://[ff02::1234%eno1]:56363 local=udp6://[fe80::fa32:e4ff:fe6d:f270%eno1]:48860
1587598844.875376 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.875438 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.875499 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::cd6a:3934:f585:444%enp5s0f3]:57269,remote=udp6://[ff02::1234%enp5s0f3]:56363] Creating transport
1587598844.875512 INFO: [FaceTable] Added face id=272 remote=udp6://[ff02::1234%enp5s0f3]:56363 local=udp6://[fe80::cd6a:3934:f585:444%enp5s0f3]:57269
1587598844.876542 INFO: [PrivilegeHelper] elevated to effective uid=0 gid=0
1587598844.876604 INFO: [PrivilegeHelper] dropped to effective uid=0 gid=0
1587598844.876668 INFO: [MulticastUdpTransport] [id=0,local=udp6://[fe80::92c8:13fc:3b78:e678%ens6f3]:36138,remote=udp6://[ff02::1234%ens6f3]:56363] Creating transport
1587598844.876680 INFO: [FaceTable] Added face id=273 remote=udp6://[ff02::1234%ens6f3]:56363 local=udp6://[fe80::92c8:13fc:3b78:e678%ens6f3]:36138
1587598844.877712 INFO: [AutoPrefixPropagator] local registration only for /localhost/nfd
```

## DIFS 실행

* 'repo-0.conf' 파일을 기반으로 prefix 가 등록됨
* 등록 내용은 다음과 같음
  * _/etri/repo_: put
  * _/etri/repo/{repo num}_: get (manifest)
  * _/etri/repo/{repo num}/data_: get (data)
  * _/get_: get (cli to repo)

> FaceTable 에 로컬 face id 생성

> RibManager 를 통하여 자신의 prefix 에 대한 route (face id) 를 등록

### repo0

```
ndn-repo-ng -c repo-0.conf

1587598955.181494 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://78] Creating transport
1587598955.181530 INFO: [FaceTable] Added face id=274 remote=fd://78 local=unix:///run/nfd.sock
1587598955.185876 INFO: [RibManager] Adding route /etri/repo nexthop=274 origin=app cost=0
1587598955.189834 INFO: [RibManager] Adding route /etri/repo/0 nexthop=274 origin=app cost=0
1587598955.191764 INFO: [RibManager] Adding route /etri/repo/0/data nexthop=274 origin=app cost=0
1587598955.193775 INFO: [RibManager] Adding route /get nexthop=274 origin=app cost=0
1587598955.194018 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo
1587598955.198455 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/0
1587598955.201374 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/0/data
1587598955.204207 INFO: [AutoPrefixPropagator] no signing identity available for: /get
```

### repo1

```
ndn-repo-ng -c repo-0.conf

1587598970.429277 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://78] Creating transport
1587598970.429311 INFO: [FaceTable] Added face id=274 remote=fd://78 local=unix:///run/nfd.sock
1587598970.433574 INFO: [RibManager] Adding route /etri/repo nexthop=274 origin=app cost=0
1587598970.437540 INFO: [RibManager] Adding route /etri/repo/1 nexthop=274 origin=app cost=0
1587598970.439530 INFO: [RibManager] Adding route /etri/repo/1/data nexthop=274 origin=app cost=0
1587598970.441515 INFO: [RibManager] Adding route /get nexthop=274 origin=app cost=0
1587598970.441694 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo
1587598970.446258 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/1
1587598970.449291 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/1/data
1587598970.452363 INFO: [AutoPrefixPropagator] no signing identity available for: /get
```

### repo2

```
ndn-repo-ng -c repo-0.conf

1587598984.882374 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://78] Creating transport
1587598984.882410 INFO: [FaceTable] Added face id=274 remote=fd://78 local=unix:///run/nfd.sock
1587598984.886742 INFO: [RibManager] Adding route /etri/repo nexthop=274 origin=app cost=0
1587598984.890727 INFO: [RibManager] Adding route /etri/repo/2 nexthop=274 origin=app cost=0
1587598984.892739 INFO: [RibManager] Adding route /etri/repo/2/data nexthop=274 origin=app cost=0
1587598984.894684 INFO: [RibManager] Adding route /get nexthop=274 origin=app cost=0
1587598984.894929 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo
1587598984.899581 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/2
1587598984.903138 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/2/data
1587598984.906356 INFO: [AutoPrefixPropagator] no signing identity available for: /get
```

## face create

* 자신을 제외한 다른 DIFS Node IP 를 사용하여 face 생성
  * _[UnicastUdpTransport]_ 를 통해 자신의 IP 를 local 로 두고 상대 Node IP 를 remote 로 구성하여 포트 6363 을 사용하는 transport 생성
  * _[FaceTable]_ 에 face id 생성 (local=자신의 IP:6363, remote=상대 Node IP:6363)

### repo0

```
nfdc face create udp://100.0.0.3
nfdc face create udp://100.0.0.4

1587599354.196401 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599354.196469 INFO: [FaceTable] Added face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599354.198899 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.2:6363,remote=udp4://100.0.0.3:6363] Creating transport
1587599354.198941 INFO: [FaceTable] Added face id=276 remote=udp4://100.0.0.3:6363 local=udp4://100.0.0.2:6363
1587599354.203142 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599354.203201 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599354.205489 INFO: [FaceTable] Removed face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599358.226584 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599358.226617 INFO: [FaceTable] Added face id=277 remote=fd://79 local=unix:///run/nfd.sock
1587599358.229009 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.2:6363,remote=udp4://100.0.0.4:6363] Creating transport
1587599358.229044 INFO: [FaceTable] Added face id=278 remote=udp4://100.0.0.4:6363 local=udp4://100.0.0.2:6363
1587599358.234865 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599358.234925 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599358.237245 INFO: [FaceTable] Removed face id=277 remote=fd://79 local=unix:///run/nfd.sock
```

### repo1

```
nfdc face create udp://100.0.0.2
nfdc face create udp://100.0.0.4

1587599388.247564 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599388.247598 INFO: [FaceTable] Added face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599388.250056 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.3:6363,remote=udp4://100.0.0.2:6363] Creating transport
1587599388.250089 INFO: [FaceTable] Added face id=276 remote=udp4://100.0.0.2:6363 local=udp4://100.0.0.3:6363
1587599388.253777 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599388.253814 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599388.254958 INFO: [FaceTable] Removed face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599394.751821 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599394.751861 INFO: [FaceTable] Added face id=277 remote=fd://79 local=unix:///run/nfd.sock
1587599394.754298 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.3:6363,remote=udp4://100.0.0.4:6363] Creating transport
1587599394.754334 INFO: [FaceTable] Added face id=278 remote=udp4://100.0.0.4:6363 local=udp4://100.0.0.3:6363
1587599394.759099 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599394.759148 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599394.760594 INFO: [FaceTable] Removed face id=277 remote=fd://79 local=unix:///run/nfd.sock
```

### repo2

```
nfdc face create udp://100.0.0.2
nfdc face create udp://100.0.0.3

1587599399.974577 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599399.974612 INFO: [FaceTable] Added face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599399.977052 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.4:6363,remote=udp4://100.0.0.2:6363] Creating transport
1587599399.977084 INFO: [FaceTable] Added face id=276 remote=udp4://100.0.0.2:6363 local=udp4://100.0.0.4:6363
1587599399.981436 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599399.981488 INFO: [Transport] [id=275,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599399.983371 INFO: [FaceTable] Removed face id=275 remote=fd://79 local=unix:///run/nfd.sock
1587599402.074755 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587599402.074788 INFO: [FaceTable] Added face id=277 remote=fd://79 local=unix:///run/nfd.sock
1587599402.077160 INFO: [UnicastUdpTransport] [id=0,local=udp4://100.0.0.4:6363,remote=udp4://100.0.0.3:6363] Creating transport
1587599402.077195 INFO: [FaceTable] Added face id=278 remote=udp4://100.0.0.3:6363 local=udp4://100.0.0.4:6363
1587599402.082901 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587599402.082961 INFO: [Transport] [id=277,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587599402.084875 INFO: [FaceTable] Removed face id=277 remote=fd://79 local=unix:///run/nfd.sock
```

## route add

* 생성된 face id 에 대한 라우팅 룰을 설정
  * _[RibManager]_ 를 통해 상대 Node 의 _prefix_ 에 해당하는 face id 등록 (이때 face id 는 'face create' 과정에서 생성된 id)

> face create 과정에서 생성된 face id 는

> route add 를 통하여 라우팅 정보가 생성됨

### repo0

```
nfdc route add /etri/repo/1 udp://100.0.0.3
nfdc route add /etri/repo/2 udp://100.0.0.4

1587600349.951891 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600349.951926 INFO: [FaceTable] Added face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600349.960987 INFO: [RibManager] Adding route /etri/repo/1 nexthop=276 origin=static cost=0
1587600349.961337 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600349.961391 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600349.963352 INFO: [FaceTable] Removed face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600349.968185 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/1
1587600357.472397 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600357.472454 INFO: [FaceTable] Added face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600357.481600 INFO: [RibManager] Adding route /etri/repo/2 nexthop=278 origin=static cost=0
1587600357.482067 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600357.482122 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600357.484041 INFO: [FaceTable] Removed face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600357.488993 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/2
```

### repo1

```
nfdc route add /etri/repo/0 udp://100.0.0.2
nfdc route add /etri/repo/2 udp://100.0.0.4

1587600400.934807 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600400.934846 INFO: [FaceTable] Added face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600400.944156 INFO: [RibManager] Adding route /etri/repo/0 nexthop=276 origin=static cost=0
1587600400.944458 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600400.944522 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600400.947905 INFO: [FaceTable] Removed face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600400.951657 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/0
1587600407.980982 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600407.981017 INFO: [FaceTable] Added face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600407.989744 INFO: [RibManager] Adding route /etri/repo/2 nexthop=278 origin=static cost=0
1587600407.990196 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600407.990244 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600407.992179 INFO: [FaceTable] Removed face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600407.996493 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/2
```

### repo2

```
nfdc route add /etri/repo/0 udp://100.0.0.2
nfdc route add /etri/repo/1 udp://100.0.0.3

1587600442.537041 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600442.537078 INFO: [FaceTable] Added face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600442.545104 INFO: [RibManager] Adding route /etri/repo/0 nexthop=276 origin=static cost=0
1587600442.545721 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600442.545779 INFO: [Transport] [id=279,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600442.547704 INFO: [FaceTable] Removed face id=279 remote=fd://79 local=unix:///run/nfd.sock
1587600442.552868 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/0
1587600456.384416 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600456.384449 INFO: [FaceTable] Added face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600456.393598 INFO: [RibManager] Adding route /etri/repo/1 nexthop=278 origin=static cost=0
1587600456.394073 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600456.394115 INFO: [Transport] [id=280,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600456.395440 INFO: [FaceTable] Removed face id=280 remote=fd://79 local=unix:///run/nfd.sock
1587600456.400841 INFO: [AutoPrefixPropagator] no signing identity available for: /etri/repo/1
```

## put file

* 콘텐츠를 서비스 할 이름인 _Content Name_ 으로 repository 에 파일을 insert 함
  * _[FaceTable]_ 에서 'face id' 를 하나 생성하고
  * _[RibManager]_ 에서 라우팅을 위한 이름인 _Content Name_ 과 이에 대한 'face id' 를 등록
  * 라우팅 등록이 완료되면 'face id' 삭제

**이 과정이 _manifest_ 에 대한 내용으로 이해하면 되는지 의문**

### repo0

* repo0 에 `hello01` 이라는 _Content Name_ 으로 `ndnputfile` 실행
* _/etri/repo/2_ (즉, repo2) 에 _manifest_ 생성됨
* repo1, repo2 의 NFD 는 변화 없음

```
build/tools/ndnputfile -D /etri/repo /hello01 /tmp/hello01.txt

1587600589.549372 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600589.549409 INFO: [FaceTable] Added face id=281 remote=fd://79 local=unix:///run/nfd.sock
1587600589.553069 INFO: [RibManager] Adding route /hello01 nexthop=281 origin=app cost=0
1587600589.556691 INFO: [AutoPrefixPropagator] no signing identity available for: /hello01
1587600590.571765 INFO: [Transport] [id=281,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600590.571824 INFO: [Transport] [id=281,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600590.573858 INFO: [FaceTable] Removed face id=281 remote=fd://79 local=unix:///run/nfd.sock
1587600590.574193 INFO: [AutoPrefixPropagator] no signing identity available for: /hello01
```

## get file

* _ndnputfile_ 과정을 통해 등록한 _Content Name_ 을 사용하여 데이터를 수집
  * _[FaceTable]_ 에서 'face id' 를 하나 생성

**_manifest_ 파일을 수집하게 되고 이후 _/etri/repo/{repo num}/data/{Content Name}/#_ 를 보낼 것인데 어떻게 진행되는지 모르겠음**

### repo1

* repo1 에서 `hello01` 이라는 _Content Name_ (repo0 에서 등록한) 으로 `ndnputfile` 실행
* repo0 에 Content 가 있고 repo2 에 manifest 가 있는 이유로 repo1 에서 실행함
* _/etri/repo/2_ 로 부터 manifest 받음
* manifest 내용을 확인하고 _interest_ 발생

```
build/tools/ndngetfile /hello01

1587600707.206667 INFO: [UnixStreamTransport] [id=0,local=unix:///run/nfd.sock,remote=fd://79] Creating transport
1587600707.206702 INFO: [FaceTable] Added face id=281 remote=fd://79 local=unix:///run/nfd.sock
1587600707.224963 INFO: [Transport] [id=281,local=unix:///run/nfd.sock,remote=fd://79] setState UP -> CLOSING
1587600707.225000 INFO: [Transport] [id=281,local=unix:///run/nfd.sock,remote=fd://79] setState CLOSING -> CLOSED
1587600707.225833 INFO: [FaceTable] Removed face id=281 remote=fd://79 local=unix:///run/nfd.sock
```

## nfdc route list

### repo0

```
prefix=/get nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/0 nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/0/data nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/1 nexthop=276 origin=static cost=0 flags=child-inherit expires=never
prefix=/etri/repo/2 nexthop=278 origin=static cost=0 flags=child-inherit expires=never
prefix=/localhost/nfd nexthop=266 origin=app cost=0 flags=child-inherit expires=never
```

### repo1

```
prefix=/get nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/0 nexthop=276 origin=static cost=0 flags=child-inherit expires=never
prefix=/etri/repo/1 nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/1/data nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/2 nexthop=278 origin=static cost=0 flags=child-inherit expires=never
prefix=/localhost/nfd nexthop=266 origin=app cost=0 flags=child-inherit expires=never
```

### repo2

```
prefix=/get nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/0 nexthop=276 origin=static cost=0 flags=child-inherit expires=never
prefix=/etri/repo/1 nexthop=278 origin=static cost=0 flags=child-inherit expires=never
prefix=/etri/repo/2 nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/etri/repo/2/data nexthop=274 origin=app cost=0 flags=child-inherit expires=never
prefix=/localhost/nfd nexthop=266 origin=app cost=0 flags=child-inherit expires=never
```
