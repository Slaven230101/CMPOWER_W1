![](./docs/cmpower_w1.png)
# **第一部分**

## **1. 简介**

中移铁通智能插座 (型号: CMPOWER W1) 基于乐鑫 **ESP8266 WiFi** 芯片设计。由于早已没有官方 APP 支持，于是重新开发一版固件适配该硬件基本功能。

固件基于乐鑫 **ESP8266_RTOS_SDK** 开发，通信协议采用 **MQTT-TCP** 方式控制插座的两个继电器。其中 **MQTT Broker** 可以根据实际情况自定义，默认使用: `broker.emqx.io`。配合例如: **MyMQTT APP** 可以实现远程控制插座的主继电器和子继电器。

*你也可以基于此固件对接私有的 MQTT broker*

## **2. 配网 APP**

使用 EspTouch，详细介绍可以点击链接:
[安卓](https://github.com/EspressifApp/EsptouchForAndroid)
[IOS](https://github.com/EspressifApp/EsptouchForIOS)

可以通过手机应用市场下载安装。

手机连接 WiFi (最好是非混合的2.4G)，打开 APP 后选择 EspTouch V2 方式，输入 WiFi 密码和需要配网的设备数量。

##### **关于自定义数据说明**

由于使用 Public broker，因此有可能出现不稳定情况，为了便于更换 broker 通过自定义数据方式传给设备。除此之外，为了避免所有设备都订阅相同的主题，用户可以自定义主题的一部分，从而避免设备被其他人控制(*如果大家使用相同 broker，订阅相同主题*)，自定义字符串也是通过自定义数据方式传给设备，因此，自定义数据格式如下:

`"broker url":"user string"`

使用 `:` 分隔，如果自定义数据缺省，则默认使用:

`broker.emqx.io:Slaven`

当然也可以缺省部分，比如缺省 `broker`:

`:Slaven`

或者缺省 `user string`:

`broker.emqx.io:`

![](./docs/esptouch.android.jpg)

*图片以自定义 broker: broker.hivemq.com，自定义 user string: Demo 为例说明*

## **3. 控制 APP**

**MyMQTT** 之类都可，只要可以发布订阅主题。

## **4. 订阅主题**

当前 MQTT 订阅的主题有三个，如下：

| Topic                 | 设备端 | APP 端 |
| --------------------- | --- | ----- |
| /"user string"/set    | 订阅方 | 发布方   |
| /"user string"/get    | 订阅方 | 发布方   |
| /"user string"/status | 发布方 | 订阅方   |

其中 `user string` 由配网时候 EspTouch 自定义数据传入，如果缺省则使用默认主题:

`/Slaven/set`

`/Slaven/get`

`/Slaven/status`

如果自定义 `user string`，则主题根据自定义相应修改

![](./docs/mqtt.client.jpg)

*图片以默认主题为例说明*

## **5. 消息负载**

- 对于 `/"user string"/set` 主题：

| Payload                         | 功能    |
| ------------------------------- | ----- |
| {"socket":"main","onoff":"on"}  | 主继电器开 |
| {"socket":"main","onoff":"off"} | 主继电器关 |
| {"socket":"sub","onoff":"on"}   | 子继电器开 |
| {"socket":"sub","onoff":"off"}  | 子继电器关 |

- 对于 `/"user string"/get` 主题：

| Payload           | 功能         |
| ----------------- | ---------- |
| {"socket":"main"} | 获取主继电器开关状态 |
| {"socket":"sub"}  | 获取子继电器开关状态 |

- 对于 `/"user string"/status` 主题：

设备收到 `/"user string"/set` 或 `/"user string"/get` 主题消息后，通过 `/"user string"/status` 主题将设备状态上报给 APP 端。

## **6. LED说明**

| LED    | 状态  | 功能                     |
| ------ | --- | ---------------------- |
| 蓝色     | 闪烁  | 设备未配网                  |
| 蓝色     | 常亮  | WiFi 已连接，但 Broker 还未连接 |
| 蓝色     | 灭   | Broker 连接成功            |
| 红色     | 常亮  | WiFi 断连                |
| 红色     | 灭   | WiFi 回连成功              |
| 白色     | 常亮  | 子继电器开                  |
| 白色     | 灭   | 子继电器关                  |
| 按键 LED | 常亮  | 主继电器开                  |
| 按键 LED | 灭   | 主继电器关                  |

`蓝色 LED 只在配网环节指示，后续只有红色 LED 指示网络情况`

## **7. 按键说明**

`短按`按键同时`开/关`主/子继电器

## **8. 重置说明**

`长按`按键直至红色 LED 亮起松手则重置设备

## **9. 计划开发**

 - MQTT-TLS
 - 计量功能
 - 本地 OTA
 - 定制手机端 APP 
 
# 第二部分

## **1. 如何编译**

参考[官方指导](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#introduction)

注意: **ESP8266_RTOS_SDK** 使用 `release/v3.4`。

## **2. 如何烧录**

可自行编译或者使用 release 的固件 `xxx_factory.bin`

 - 下载[官方工具](https://www.espressif.com/zh-hans/support/download/other-tools)
 - 按图配置烧录
 
![](./docs/download_1.png)
![](./docs/download_2.png)
![](./docs/console.png)