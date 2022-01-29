# 1 ツールの概要

- 機能概要: 指定したNICで送受信されるパケットを補足してパケットの概要を表示します
- 動作環境: Linux CentOS8
- 開発言語: C言語
- 関連技術: RAW Etherソケット

# 2 処理概要

## 2.1 ソケットの初期化
socket関数でRAWソケットを取得し、bind関数を用いて指定したNICに接続します。
|[pcap.c: init_socket()](https://github.com/tanuki-project/Networking/blob/main/simple_pcap/pcap.c#L83-L102)|
|-|

## 2.2 パケットの受信
NICで受信したパケットをrecvmsg関数で一個ずつ読み取ります。
|[pcap.c: receive_packet()](https://github.com/tanuki-project/Networking/blob/main/simple_pcap/pcap.c#L125-L139)|
|-|

## 2.3 パケットの解析
受信したパケットをイーサネット、IP、TCP/UDPといった順にヘッダを解析していきます。
 
OSのヘッダファイルには各種プロトコルのヘッダ情報に対応する構造体が定義されています。それらと受信データを対応付けることでヘッダ情報の内容を読み取っていきます。
|[pcap.c: print_packet()](https://github.com/tanuki-project/Networking/blob/main/simple_pcap/pcap.c#L164-L203)|
|-|

例えば、TCPでは以下のように受信データを対応付けることで内容の読み取りが容易になります。
- 受信データの 0～13バイト　→　ethhdr構造体にマッピング
- 受信データの14～33バイト　→　iphdr構造体にマッピング
- 受信データの34～53バイト　→　tcphdr構造体にマッピング

# 3 ツールの実行

##3.1 ツールの起動
以下の書式でツールをルート権限で実行します。

```rb
./pcap {NIC名}
```

##3.2 実行結果

ping等で通信を行います。

```rb
$ ping 192.168.10.1
PING 192.168.10.1 (192.168.10.1) 56(84) bytes of data.
64 bytes from 192.168.10.1: icmp_seq=1 ttl=64 time=1.17 ms
64 bytes from 192.168.10.1: icmp_seq=2 ttl=64 time=0.699 ms
64 bytes from 192.168.10.1: icmp_seq=3 ttl=64 time=0.723 ms
64 bytes from 192.168.10.1: icmp_seq=4 ttl=64 time=0.676 ms
^C
--- 192.168.10.1 ping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3086ms
rtt min/avg/max/mdev = 0.676/0.816/1.166/0.202 ms
```
受信したパケットの概要が表示されることを確認します。

```rb
$ sudo ./pcap enp0s25
ARP REQ(1) b8:ae:ed:72:34:c8/192.168.10.61 -> 0:0:0:0:0:0/192.168.10.1
ARP REP(2) 10:66:82:39:1a:30/192.168.10.1 -> b8:ae:ed:72:34:c8/192.168.10.61
192.168.10.61 -> 192.168.10.1 type = 0001
192.168.10.1 -> 192.168.10.61 type = 0001
192.168.10.61 -> 192.168.10.1 type = 0001
192.168.10.1 -> 192.168.10.61 type = 0001
192.168.10.61 -> 192.168.10.1 type = 0001
192.168.10.1 -> 192.168.10.61 type = 0001
192.168.10.61 -> 192.168.10.1 type = 0001
192.168.10.1 -> 192.168.10.61 type = 0001
ARP REQ(1) 10:66:82:39:1a:30/192.168.10.1 -> 0:0:0:0:0:0/192.168.10.61
ARP REP(2) b8:ae:ed:72:34:c8/192.168.10.61 -> 10:66:82:39:1a:30/192.168.10.1
^C
```
