# 1. ツールの概要

- 機能概要:　2本のNICが同一LANに接続されているかどうかを確認するためパケットを送受信します
- 動作環境:　Linux CentOS8
- 開発言語:　C言語
- 関連技術:　RAW Etherソケット

# 2. 処理概要
サンプルコード[ sndrcvpkt ](https://github.com/tanuki-project/Networking/blob/main/sndrcvpkt/)を用いて処理概要を説明します。
## 2.1 ソケットの初期化
socket関数で送信用および受信用のRAWソケットを取得し、各々bind関数を用いて指定したNICに接続します。
|[sndrcvpkt.c: main()](https://github.com/tanuki-project/Networking/blob/main/sndrcvpkt/sndrcvpkt.c#L61-L78)|
|-|

## 2.2 パケットの送信
パケットを作成してwrite関数で送信用NICからパケットを送信します。
Ethenetの場合、ethhdr 構造体を送信バッファの0バイト目にマッピングし、宛先MACアドレス[6Byte]、送信元MACアドレス[6Byte]、フレームタイプ[2Byte]を設定することで送信パケットのヘッダ部分が生成されます。
|[sndrcvpkt.c: sndrcv_pkt()](https://github.com/tanuki-project/Networking/blob/main/sndrcvpkt/sndrcvpkt.c#L177-L201)|
|-|

## 2.3 パケットの受信
recvfrom関数で受信用NICでパケットを受信します。
受信したパケットは送信したパケットと内容が同じになっているはずです。
|[sndrcvpkt.c: sndrcv_pkt()](https://github.com/tanuki-project/Networking/blob/main/sndrcvpkt/sndrcvpkt.c#L203-L209)|
|-|

# 3. ツールの実行

## 3.1 事前準備

2本の同一LANに接続されたNICが用意できない場合は、検証用の veth pair を作成します。

```rb
$ sudo ip link add veth0-send type veth peer name veth0-recv
$ ip link show veth0-send
25: veth0-send@veth0-recv: <BROADCAST,MULTICAST,M-DOWN> mtu 1500 qdisc noop state DOWN mode DEFAULT group default qlen 1000
    link/ether 86:03:bb:2c:15:59 brd ff:ff:ff:ff:ff:ff

$ sudo ip link set veth0-recv up
$ sudo ip link set veth0-send up
```
## 3.2 ツールの起動
以下の書式でツールをルート権限で実行します。

```rb
$ ./sndrcvpkt {送信NIC名} {受信NIC名}
```

## 3.3 実行結果
パケットの送信と受信が成功すること、送信データと受信データが同じであることを確認します。

```rb
$ sudo ./sndrcvpkt veth0-send veth0-recv
write(3, 64) = 64
b612a50a5a46 8603bb2c1559 2020 0101
recvmsg(4) =  64
86:3:bb:2c:15:59 -> b6:12:a5:a:5a:46 type = 2020
b612a50a5a46 8603bb2c1559 2020 0101
write(3, 64) = 64
b612a50a5a46 8603bb2c1559 2020 0101
recvmsg(4) =  64
86:3:bb:2c:15:59 -> b6:12:a5:a:5a:46 type = 2020
b612a50a5a46 8603bb2c1559 2020 0101
write(3, 64) = 64
b612a50a5a46 8603bb2c1559 2020 0101
recvmsg(4) =  64
86:3:bb:2c:15:59 -> b6:12:a5:a:5a:46 type = 2020
b612a50a5a46 8603bb2c1559 2020 0101
^C
```
