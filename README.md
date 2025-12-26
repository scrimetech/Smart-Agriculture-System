# 🌿 智能农业物联网管理系统 (Smart-Agriculture-System)

## 📖 项目简介

本项目是一套完整的端到端物联网（IoT）解决方案，旨在实现温室大棚环境的自动化管理。系统通过 **STM32** 采集传感器数据，利用 **MQTT 协议** 实现低延迟通信，并在基于 **Vue 3** 的 Web 端进行实时可视化展示与远程控制。

### ✨ 核心功能

- **实时监控**：采集温度、空气湿度、土壤湿度及光照强度。
- **双模式切换**：支持 **自动 (AUTO)** 与 **手动 (MANUAL)** 模式切换。
- **远程控制**：手动模式下，可通过 Web 界面控制水泵、风扇、补光灯。
- **自动策略**：自动模式下，系统根据环境阈值自动触发执行器（如：湿度低则自动浇水）。
- **实时告警**：当环境参数超出设定安全阈值时，Web 端实时弹出报警提醒。
- **数据可视化**：使用 ECharts 实现环境参数的历史曲线展示。

------

## 🛠️ 技术栈

### 硬件端 (Embedded)

- **主控芯片**: STM32F103C8T6
- **操作系统**: FreeRTOS
- **通信模块**: ESP8266 (使用 AT 指令集集成 MQTT 协议)
- **传感器**: DHT11 (温湿度)、土壤湿度传感器、光敏电阻
- **执行器**: LED灯珠 (模拟水泵/风扇)

### 云端与通信 (Cloud & Communication)

- **消息服务器**: Mosquitto (MQTT Broker)
- **Web 服务器**: Nginx (托管静态资源)
- **协议**: MQTT over WebSocket (用于 Web 端通信)

### 前端 (Web)

- **框架**: Vue 3 + Vite
- **图表**: ECharts
- **通信库**: Paho MQTT JS

------

## 📂 目录结构说明

Plaintext

```
.
├── stm32-firmware/          # STM32 嵌入式固件源代码 (Keil 项目)
│   ├── CORE/                # STM32内核文件
│   ├── FreeRTOS/            # FreeRTOS 内核源码
│   ├── HARDWARE/            # 传感器及外设驱动 (DHT11, ADC, ESP8266等)
│   ├── STM32F10x_FWLib/     # 系统固件库
│   ├── USER/                # 应用逻辑、工程文件
│   ├── keilkill.bat         # 清除工程编译文件脚本
├── web-frontend/            # Vue.js 前端源代码
│   ├── dist/                # build打包后的文件
│   ├── node_modules/        # 需要的模块文件
│   ├── src/                 # 源代码目录
│   ├── index.html           # 网页文件占位
│   ├── package.json         # 项目依赖配置
│   ├── package-lock.json    # 锁定文件，确保在不同电脑上安装的插件版本完全一致
│   ├── start_dashboard.bat  # 本地运行测试脚本
│   └── vite.config.js       # Vite 的配置文件
├── README.md                # 项目说明文档
└── .gitignore               # Git 忽略文件配置
```

------

## 🚀 快速开始

### 1. 硬件连接

1. **DHT11**: 连接至 PA5 (注意时序需匹配 72MHz 频率)。
2. **ESP8266**: 连接至 USART2 接口 (通常为 PA2/PA3)。
3. **传感器**: 土壤湿度/光敏电阻连接至相应 ADC 通道 (PA1, PA4)。
4. **设备**：将三个LED灯珠连接到PA12、PA13、PA14，模拟水泵、风扇、照明灯

### 2. 服务器配置

- 确保服务器已安装 **Mosquitto** 并开启 1883 (MQTT) 和 8080 (WebSocket) 端口。
- 配置MQTT服务器的用户名密码（建议）
- 在MQTT服务器的配置文件中配置MQTT监听1883、8080端口
- 安装并配置 Nginx 代理，将 Web 根目录指向前端打包后的 `dist` 文件夹。

### 3. 前端部署

Bash

```
cd web-frontend
npm install
# 修改 src/config/mqtt.js 中的 IP 地址为您的服务器 IP
npm run build
# 将 dist 文件夹内容上传至 Nginx 目录
```

------

## 🔧 常见问题排查 (FAQ)

- **DHT11 读取失败**: 检查 FreeRTOS 任务调度是否干扰了微秒级延时，建议在读取时进入 `taskENTER_CRITICAL()` 临界区。
- **MQTT 频繁断连**: 检查 `ClientID` 是否在多端重复，以及服务器 8080 端口是否已在安全组放行。
- **Web 界面 404**: 检查 Nginx 配置中是否添加了 `try_files $uri $uri/ /index.html;`。