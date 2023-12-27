# Healthcare_System_Based_on_Stm32
    以STM32F103ZET6为主控板，通过ESP8266wifi模块，采用MQTT协议与云服务器上运行的EMQX服务端进行通信，以及由TCP通信将数据发送到手机App端，基于QT开发服务器和安卓APP，实现家庭成员健康数据的远程共享。具体来说，由三部分构成:
(1)生理信息采集与传输部分， 以 STM32F103ZET6 为主控板负责与传感器通信， 包括 AS608 指纹识别模块、 MLX0614 红外测温传感器、 MAX30102 心率血氧传感器等，并通过 ESP8266WiFi 模块将数据通过互联网传输到云服务器上的 EMQX 服务端。
(2)云服务器系统管理部分， EMQ X 服务端负责接收健康数据，并运行基于QT 开发的面向系统管理员使用的应用程序，负责将数据存入数据库、与手机APP 客户端通信、控制系统服务端运行、数据库数据可视化等。
(3) 手机 APP 家庭用户终端部分， 基于QT的Qt for Android组件开发的手机APP，为家庭用户提供每个家庭成员的历史健康数据查询并可视化，以及对家庭用户的管理和用户验证等功能。

主控板和各模块的接线可看main代码注释，云服务器使用购买的短期的阿里云ESC服务器，采用Windows系统个人觉得好调试些，依靠EMQX官方文档在云服务器上搭建EMQX服务端，然后再下载MQTT X客户端用来调试。EMQX官方文档地址https://www.emqx.io/docs/zh/latest/deploy/install-windows.html 。在stm32部分参考过https://www.bilibili.com/video/BV1ae411W7yD/?spm_id_from=333.1007.top_right_bar_window_custom_collection.content.click&vd_source=6a482a56fb6ff204c6b983935594fe2b 的一些模块的连接和开发。Qt部分的开发也参考过许多博客文章，用Qt开发MQTT要注意Qt的版本，5.12和5.14是支持比较好的版本，这篇博客或许能帮助你https://blog.csdn.net/weixin_44618297/article/details/129029376 。用Qt开发Android App也要注意机型的适配问题，安装开发环境可参考https://blog.csdn.net/qq153471503/article/details/128063210 ，https://blog.csdn.net/qq_57780685/article/details/129984744 能让App看起来还不错。
本项目程序的运行，需要你购买一个服务器，并安装好EMQX服务端、sqlite3数据库等，然后代码中修改服务端ip和端口，再编译后打包程序到云服务器上和手机上运行。
