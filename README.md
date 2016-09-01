#KServer
[![Build Status][1]][2] 
[1]: https://api.travis-ci.org/zentelfong/KServer.svg?branch=master
[2]: https://travis-ci.org/zentelfong/KServer
## 简介 ##
是C++语言编写的跨平台的高性能UDP可靠传输服务器及客户端的实现。

## 特性 ##
1. 底层使用UDP协议，使用KCP算法确保传输的可靠性及实时性。
2. 无需握手来建立连接。
3. 异步非阻塞模型，使用小根堆算法，实现了类似于epoll模型，大大提高了并发数。
4. 支持ipv4及ipv6地址。
5. 使用内存池，提高性能。
6. 基于范德蒙矩阵的丢包纠错算法（8个数据包，生成2个冗余包，如果传输过程丢失了两个包则能通过其他8个包还原为未丢包状态。）

## 待完善 ##
1. 传输层数据压缩。
2. http协议的实现。
