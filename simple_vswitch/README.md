# 1. simple_vswitch の概要

　機能概要: 簡易的なL2スイッチの機能をもつカーネルモジュールおよびその管理コマンドが含みます
 
　動作環境: Linux CentOS8
 
　開発言語: C言語
 
　関連技術: Ethernet, L2スイッチ, デバイスドライバ

## 1.1 カーネルモジュール(vswitch.ko)

vswitch.koはLinuxサーバに搭載された複数のNICをポートとして接続し、ポート間でパケットの転送を行う機能をOSに追加するカーネルモジュールです。最大で24本のNICをポートとして接続し、ポート間でパケットの転送を行います。

## 1.2 管理コマンド(vswconfig)

vswconfigは以下のサブコマンドを持つ管理コマンドです。仮想スイッチの構築や解体に使用します。

| サブコマンド |          説明                  |
|:-----------:|:-----------------------------:|
|add          | vswitchにNICを接続します        |
|delete       | vswitchからNICを切断します      |
|show         | 接続中のNICの一覧を表示します。  |

# 2. 処理概要

## 2.1 受信ハンドラの登録

vswconfig add コマンドを実行すると vswitch.ko ではカーネル関数 netdev_rx_handler_register() により指定されたNICに対してパケットの受信ハンドラを登録し、NICをプロミスキャスモードに設定します。

　　[vswitch_packet.c: vswitch_rx_handler_register()](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_packet.c#L26-L41)

## 2.2 パケットの受信

NICでパケットを受信した場合、2.1で登録した受信ハンドラが呼び出されます。受信ハンドラでは以下の処理を行います。

・受信パケットの送信元MACアドレスと受信したポートをFDB(Forwarding Database)に設定します。

・BPDUやLLDP等、転送の必要がないL2の制御用パケットを破棄します。

・パケットの転送処理を呼び出します。

　　[vswitch_packet.c: vswitch_rx()](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_packet.c#L57-L117)

## 2.3 パケットの転送

・宛先MACアドレスがユニキャストの場合、FDBから転送先のポートを検索します。

・有効な転送先ポートが見つかった場合、カーネル　関数dev_queue_xmit() により該当ポートからパケットを送信します。　

・転送先ポートが見つからない場合や宛先がマルチキャストアドレスの場合、パケットを受信したポートを除く全てのポートからパケットを送信します。

　　[vswitch_packet.c: vswitch_forward()](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_packet.c#L121-L178)

# 3. ツールの実行

## 3.1 事前準備

本ツールはカーネルモジュールを含みますので不具合が発生した場合、OSがクラッシュする場合があります。検証用のVM等、クラッシュしても差し支えのない環境を用意した上で本ツールをダウンロードしてください。

## 3.2 カーネルモジュールのビルド

vswitch.koをコンパイルして所定の場所に配置します。

```rb
$ cd /home/user/Networking/simple_vswitch/src/drv
$ make
make -C /lib/modules/4.18.0-305.12.1.el8_4.x86_64/build M=/home/user/Networking/simple_vswitch/src/drv modules
make[1]: ディレクトリ '/usr/src/kernels/4.18.0-305.12.1.el8_4.x86_64' に入ります
  CC [M]  /home/user/Networking/simple_vswitch/src/drv/vswitch_main.o
  CC [M]  /home/user/Networking/simple_vswitch/src/drv/vswitch_packet.o
  CC [M]  /home/user/Networking/simple_vswitch/src/drv/vswitch_fdb.o
  LD [M]  /home/user/Networking/simple_vswitch/src/drv/vswitch.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/user/Networking/simple_vswitch/src/drv/vswitch.mod.o
  LD [M]  /home/user/Networking/simple_vswitch/src/drv/vswitch.ko
make[1]: ディレクトリ '/usr/src/kernels/4.18.0-305.12.1.el8_4.x86_64' から出ます
[takahiro@mujina drv]$ make install
install -m 0444 vswitch.ko ../../bin/drv
$
```

## 3.3 管理コマンドのビルド

vswconfigをコンパイルして所定の場所に配置します。

```rb
$ cd /home/user/Networking/simple_vswitch/src/cmd
$ make
/usr/bin/gcc -I ../../src/drv -O -Wall    vswconfig.c   -o vswconfig
$ make install
install -m 0555 vswconfig ../../bin/cmd
$
```

## 3.4 カーネルモジュールのロード

スクリプト [Install.sh](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/test/script/Install.sh) を実行してカーネルモジュール vswitck.ko をロードします。

```rb
$ cd /home/user/Networking/simple_vswitch/test/script
$ sudo ./Install.sh
vswitch is installed.
```

## 3.5 テスト用ネットワークの構築

スクリプト [construct-tap-bridge.sh](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/test/script/construct-tap-bridge.sh) を実行し検証用の veth pair を4組作成し、以下のようなテスト用ネットワークを構築します。

|  vswitchのポート | 通信用NIC  | 通信用Namespace |   IPアドレス    |
|:---------------:|:----------:|:--------------:|:--------------:|
| veth1-vsw       | veth1  　　| NS1             |192.168.100.101 |
| veth2-vsw       | veth2 　 　| NS2             |192.168.100.102 |
| veth3-vsw       | veth3 　　 | NS3             |192.168.100.103 |
| veth4-vsw       | veth4 　　 | NS4             |192.168.100.104 |


```rb
$ cd /home/user/Networking/simple_vswitch/test/script
$ sudo ./construct-tap-bridge.sh
103: veth1-vsw@if102: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 72:c8:09:d8:b2:a8 brd ff:ff:ff:ff:ff:ff link-netns NS1
    inet6 fe80::70c8:9ff:fed8:b2a8/64 scope link tentative
       valid_lft forever preferred_lft forever
102: veth1@if103: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 2e:db:1f:fb:38:06 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.100.101/24 scope global veth1
       valid_lft forever preferred_lft forever
    inet6 fe80::2cdb:1fff:fefb:3806/64 scope link tentative
       valid_lft forever preferred_lft forever
105: veth2-vsw@if104: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether de:d6:1e:cf:11:1e brd ff:ff:ff:ff:ff:ff link-netns NS2
    inet6 fe80::dcd6:1eff:fecf:111e/64 scope link tentative
       valid_lft forever preferred_lft forever
104: veth2@if105: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 5e:56:40:f5:29:00 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.100.102/24 scope global veth2
       valid_lft forever preferred_lft forever
    inet6 fe80::5c56:40ff:fef5:2900/64 scope link tentative
       valid_lft forever preferred_lft forever
107: veth3-vsw@if106: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 36:e4:8d:8a:f3:36 brd ff:ff:ff:ff:ff:ff link-netns NS3
    inet6 fe80::34e4:8dff:fe8a:f336/64 scope link tentative
       valid_lft forever preferred_lft forever
106: veth3@if107: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 8e:e1:19:b5:25:42 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.100.103/24 scope global veth3
       valid_lft forever preferred_lft forever
    inet6 fe80::8ce1:19ff:feb5:2542/64 scope link tentative
       valid_lft forever preferred_lft forever
109: veth4-vsw@if108: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 5a:65:18:60:98:72 brd ff:ff:ff:ff:ff:ff link-netns NS4
    inet6 fe80::5865:18ff:fe60:9872/64 scope link tentative
       valid_lft forever preferred_lft forever
108: veth4@if109: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default qlen 1000
    link/ether 46:21:74:96:1a:40 brd ff:ff:ff:ff:ff:ff link-netnsid 0
    inet 192.168.100.104/24 scope global veth4
       valid_lft forever preferred_lft forever
    inet6 fe80::4421:74ff:fe96:1a40/64 scope link tentative
       valid_lft forever preferred_lft forever
$
$ sudo ../../bin/cmd/vswconfig show
veth1-vsw
veth2-vsw
veth3-vsw
veth4-vsw
```

## 3.6 通信の確認

各ネームスペースにおいて、他のネームスペースに対してpingコマンド等を実行し、通信ができることを確認します。通信が疎通しているのであれば switch.koによるパケットの転送が機能していると判断できます。

```rb
$ sudo ip netns exec NS1 192.168.100.102
exec of "192.168.100.102" failed: No such file or directory
[takahiro@mujina script]$ sudo ip netns exec NS1 ping 192.168.100.102
PING 192.168.100.102 (192.168.100.102) 56(84) bytes of data.
64 bytes from 192.168.100.102: icmp_seq=1 ttl=64 time=0.126 ms
64 bytes from 192.168.100.102: icmp_seq=2 ttl=64 time=0.048 ms
64 bytes from 192.168.100.102: icmp_seq=3 ttl=64 time=0.061 ms
^C
--- 192.168.100.102 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2026ms
rtt min/avg/max/mdev = 0.048/0.078/0.126/0.034 ms
$
$ sudo ip netns exec NS2 ping 192.168.100.103
PING 192.168.100.103 (192.168.100.103) 56(84) bytes of data.
64 bytes from 192.168.100.103: icmp_seq=1 ttl=64 time=0.125 ms
64 bytes from 192.168.100.103: icmp_seq=2 ttl=64 time=0.050 ms
64 bytes from 192.168.100.103: icmp_seq=3 ttl=64 time=0.044 ms
^C
--- 192.168.100.103 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2036ms
rtt min/avg/max/mdev = 0.044/0.073/0.125/0.036 ms
$
$ sudo ip netns exec NS3 ping 192.168.100.104
PING 192.168.100.104 (192.168.100.104) 56(84) bytes of data.
64 bytes from 192.168.100.104: icmp_seq=1 ttl=64 time=0.089 ms
64 bytes from 192.168.100.104: icmp_seq=2 ttl=64 time=0.047 ms
64 bytes from 192.168.100.104: icmp_seq=3 ttl=64 time=0.046 ms
^C
--- 192.168.100.104 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2078ms
rtt min/avg/max/mdev = 0.046/0.060/0.089/0.021 ms
$
$ sudo ip netns exec NS4 ping 192.168.100.101
PING 192.168.100.101 (192.168.100.101) 56(84) bytes of data.
64 bytes from 192.168.100.101: icmp_seq=1 ttl=64 time=0.090 ms
64 bytes from 192.168.100.101: icmp_seq=2 ttl=64 time=0.050 ms
64 bytes from 192.168.100.101: icmp_seq=3 ttl=64 time=0.046 ms
^C
--- 192.168.100.101 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2075ms
rtt min/avg/max/mdev = 0.046/0.062/0.090/0.019 ms
```

## 3.7 テスト用ネットワークの解体

スクリプト [destruct-tap-bridge.sh](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/test/script/destruct-tap-bridge.sh) を実行し検証用のネットワークを解体します。

```rb
$ cd /home/user/Networking/simple_vswitch/test/script
$sudo ./destruct-tap-bridge.sh
```

## 3.8 カーネルモジュールのアンロード

スクリプト [Uninstall.sh](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/test/script/Uninstall.sh) を実行してカーネルモジュール vswitck.ko をアンロードします。

```rb
$ cd /home/user/Networking/simple_vswitch/test/script
$ sudo ./Uninstall.sh
vswitch is uninstalled.
```

# 4. パケット転送処理のカスタマイズ

本ツールではごく単純なパケット転送処理しか実装していませんが、これにパケットのフィルタリングやカプセル化、暗号化といった処理を追加することで、Linuxベースで様々な仮想的なネットワーク機器を開発することが可能です。我こそはと思う方はぜひ挑戦してみてください。

# 付録A 技術情報

vswitc.koの技術情報を参考として記載します。

## A.1 テーブル一覧

vswitch.ko では以下の制御情報によりパケットの転送を処理します。
###[vswitch.h](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch.h)

|        データ型      |          説明             |
|:--------------------|:-------------------------|
|pport_t              | vswitchのポート情報        |
|vswitch_fdb_entry    | FDBのレコード              |

## A.2 関数一覧

vswitch.koは以下の関数により実装されています。これらはカーネルから呼び出されるコールバック関数および、そのサブルーチンです。

###[vswitch_main.c](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_main.c)

|           関数         |          説明                     |
|:----------------------|:----------------------------------|
|vswitch_init           | モジュールのロード                  |
|vswitch_exit           | モジュールのアンロード              |
|vswitch_open           | openシステムコールのコールバック関数  |
|vswitch_close          | closeシステムコールのコールバック関数 |
|vswitch_ioctl          | ioctlシステムコールのコールバック関数 |
|vswitch_read           | readシステムコールのコールバック関数  |
|ioc_add_port           | ポートの追加                       |
|ioc_delete_port        | ポートの削除                       |
|remove_allports        | 全ポートの削除                     |
|ioc_get_ports          | ポート一覧の取得                   |
|find_port_by_name      | 名前によるポートの検索              |
|find_port_by_dev       | net_deviceによるポートの検索        |
|vswitch_timer_callback | タイマーのコールバック関数           |
|vswitch_netdev_handler | net_deviceのイベントハンドラ        |

###[vswitch_packet.c](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_packet.c)

|             関数              |          説明           |
|:-----------------------------|:------------------------|
|vswitch_rx_handler_register   | パケット受信ハンドラを登録しプロミスキャスモードを設定 |
|vswitch_rx_handler_unregister | パケット受信ハンドラの削除しプロミスキャスモードを解除 |
|vswitch_rx                    | パケット受信ハンドラ      |
|vswitch_forward               | パケットの転送           |

###[vswitch_fdb.c](https://github.com/tanuki-project/Networking/blob/main/simple_vswitch/src/drv/vswitch_fdb.c)

|           関数         |          説明                  |
|:-------------------------|:----------------------------|
| vswitch_set_fdb          | 簡易FDBの設定・アップデート    |
| vswitch_search_fdb       | 簡易FDBの検索                |
| vswitch_countdown_fdb    | 簡易FDBの有効期限の管理       |
|  vswitch_remove_fdb_list | 簡易FDBの削除                |
| vswitch_disable_fdb      | 特定のポートの簡易FDBを無効化  |
