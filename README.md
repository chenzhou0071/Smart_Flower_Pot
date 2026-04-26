# 智能灌溉系统 (Intelligent Irrigation System)

基于 STM32F103 + uni-app 的智能灌溉系统，支持蓝牙连接与手机远程控制。

## 项目结构

```
Intelligent_irrigation_system/
├── Application/          # uni-app Android 控制端 APP
├── STMicroelectronics/    # STM32F103 单片机程序
├── backup/               # 历史版本备份
└── README.md             # 项目总览
```

## 系统组成

### 下位机 (STMicroelectronics)
- **芯片**: STM32F103C8T6
- **传感器**: DHT11 (温湿度)、土壤湿度传感器、光敏电阻
- **通信**: HC-08 蓝牙模块 (BLE 4.0)
- **执行器**: 水泵
- **显示**: OLED 屏幕

### 上位机 (Application)
- **框架**: uni-app + HBuilderX
- **平台**: Android
- **功能**: 蓝牙连接、实时数据展示、历史折线图、远程浇水

## 通信协议

**文本格式** (STM32 → 手机): `T25H60S45L80\r\n`

| 字段 | 含义 | 示例 |
|------|------|------|
| T | 温度 (°C) | T25 |
| H | 空气湿度 (%) | H60 |
| S | 土壤湿度 (%) | S45 |
| L | 光照强度 (%) | L80 |

**指令格式** (手机 → STM32):

| 指令 | 说明 |
|------|------|
| `STATUS\r\n` | 请求立即发送传感器数据 |
| `WATER\r\n` | 立即浇水 |

## 快速开始

### 单片机程序
1. 使用 Keil MDK 打开 `STMicroelectronics` 目录下的工程
2. 编译并烧录到 STM32F103C8T6
3. 连接 HC-08 蓝牙模块 (TX→RX, RX→TX, 波特率 9600)

### 手机 APP
1. 使用 HBuilderX 打开 `Application` 目录
2. 菜单 → 发行 → 原生App-云打包 → Android
3. 安装生成的 APK 到 Android 手机
4. 开启蓝牙，扫描连接 HC-08 模块

## 智能灌溉算法

系统根据多因素自动判断是否需要浇水:
- **土壤湿度**: 基础阈值 40%
- **空气湿度**: 低于 30% 加权 +15%，50% 以下加权 +8%，70% 以上加权 -8%
- **温度**: 高于 35°C 加权 +12%，30°C 以上加权 +8%，低于 10°C 加权 -8%
- **光照**: 强光 (>70%) 加权 +8%，弱光 (<20%) 加权 -5%

阈值范围: 25% ~ 55%

## 硬件连接

| 模块 | 引脚 |
|------|------|
| HC-08 TX | PA9 |
| HC-08 RX | PA10 |
| DHT11 | PA7 |
| 土壤湿度 ADC | PA0 |
| 光敏电阻 ADC | PA1 |
| 水泵控制 | PA6 |
| OLED SCL | PB8 |
| OLED SDA | PB9 |
