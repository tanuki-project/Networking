# netlinkでネットワークの挙動をモニターする

#1 概要

netlinkとはLinuxにおいてユーザ空間とカーネル空間の間で情報をやり取りするための機構です。socketインタフェースを用いて、以下のようなイベントを検出できます。

- リンクの追加・削除
- IPアドレスの追加・削除
- ルーティングテーブルの追加・削除
- Neighborテーブルの追加・削除
- IPv6プレフィックスの受信
- etc ...

本ツールでは実際にnetlinkによりLinuxネットワークにおけるこれらのイベントをリアルタイムに検出します。

#2 netlinkソケットの取得

初期処理としてnetlinkで扱う[情報種別を指定してソケットを取得します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L45-L64)
情報の種別としては以下のものがあります。

| フラグ             |          説明                 　　　|
|:------------------|:------------------------------------|
|RTMGRP_LINK        | リンク状態                           |
|RTMGRP_NOTIFY      | 通知                                |
|RTMGRP_NEIGH       | neighborテーブル                     |
|RTMGRP_IPV4_IFADDR | IPv4インタフェースアドレス            |
|RTMGRP_IPV6_IFADDR | IPv6インタフェースアドレス            |
|RTMGRP_IPV4_ROUTE  | IPv4ルーティングテーブル              |
|RTMGRP_IPV4_MROUTE | IPv4マルチキャストルーティングテーブル  |
|RTMGRP_IPV4_RULE   | IPv4ポリシーベースルーティングテーブル  |
|RTMGRP_IPV6_ROUTE  | IPv6ルーティングテーブル              |
|RTMGRP_IPV6_MROUTE | IPv6マルチキャストルーティングテーブル  |
|RTMGRP_IPV6_IFINFO | IPv6インタフェース情報                |
|RTMGRP_IPV6_PREFIX | IPv6ネットワークプレフィックス         |


#3 イベントの受信・解析

取得したsocketに対して、[poll()でイベントを待ち合せ、recv()でメッセージ刈り取ります。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L73-L93)

#3.1 リンクの追加・削除
メッセージ種別が RTM_NEWLINK および RTM_DELLINK の場合、[追加・削除されたリンクを表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L125-L133)

#3.2 IPアドレスの追加・削除
メッセージ種別が RTM_NEWADDR および RTM_DELADDR の場合、[追加・削除されたIPアドレスを表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L136-L156)

#3.3 経路情報の追加・削除
メッセージ種別が RTM_NEWROUTE および RTM_DELROUTE の場合、[追加・削除された経路情報を表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L158-L179)

#3.4 neighborテーブルの追加・削除
メッセージ種別が RTM_NEWNEIGH および RTM_DELNEIGH の場合、[追加・削除されたneighborテーブルを表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L182-L216)

#3.5 ポリシーベースルーティングテーブルの追加・削除
メッセージ種別が RTM_NEWRULE および RTM_DELRULE の場合、[追加・削除されたポリシーベースルーティングテーブルを表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L219-L241)

#3.6 IPv6ネットワークプレフィックス
メッセージ種別が RTM_NEWPREFIX の場合、[通知されたネットワークプレフィックスを表示します。](https://github.com/tanuki-project/Networking/blob/main/watch_netlink/linkmonitor.c#L244-L254)

#4 実行例

サンプルコードを実行し、ネットワークの構成や状態がリアルタイムにモニターできることを確認してみます。

#4.1 リンクの追加

ip link add コマンドでvether pairを作成すると、

```rb
# ip link add veth0 type veth peer name veth1
```

新しいリンク veth0 と veth1 が作成されたことが判ります。

```rb
# ./linkmonitor
[21:21:19.605] new link:  dev=veth1 flags=1002 change=-1
[21:21:19.606] new link:  dev=veth0 flags=1002 change=-1
```

#4.2 IPアドレスの追加

ip address add コマンドでveth1にIPアドレスを設定すると、

```rb
# ip address add 192.168.100.101/24 dev veth1
```

veth1に対してIPアドレスと付随する経路情報が設定されたことが判ります。

```rb
# ./linkmonitor
[21:26:54.897] new addr:  dev=veth1 address=192.168.100.101
[21:26:54.898] new route: dev=veth1 dst=192.168.100.101
```

#4.3 neighborテーブルの追加

ARP未解決の状態で隣接ノードに対してpingを実行すると、

```rb
# ping 192.168.0.62
```

ARP解決が行われ新しいneighborテーブルが追加されたことが判ります。

```rb
# ./linkmonitor
[21:36:16.172] new neigh: dev=br0 dst=192.168.0.62 laddr=52:54:0:20:1d:56 reachable
```

#5 補足

ip monitor コマンドを使用すると、サンプルコードど同様の処理を行い、ネットワークの状態をモニターすることができます。詳細はオンラインマニュアル IP-MONITOR(8) を参照してください。
