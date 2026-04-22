<template>
	<view class="container">
		<!-- 连接状态 -->
		<view class="status-bar">
			<text class="status-text">{{ connected ? '已连接 HC-08' : '未连接' }}</text>
			<view class="status-btns">
				<button class="btn-small" @click="scanDevices" :disabled="scanning">
					{{ scanning ? '扫描中...' : '扫描' }}
				</button>
				<button class="btn-small btn-mac" @click="showMacInput = true">
					手动
				</button>
				<button class="btn-small btn-disconnect-status" @click="disconnect" :disabled="!connected">
					断开
				</button>
			</view>
		</view>

		<!-- 手动输入MAC弹窗 -->
		<view v-if="showMacInput" class="device-modal">
			<view class="modal-content">
				<text class="modal-title">手动输入MAC地址</text>
				<input class="mac-input" v-model="manualMac" placeholder="例如: AA:BB:CC:DD:EE:FF" />
				<view class="modal-btns">
					<button class="btn-cancel" @click="showMacInput = false">取消</button>
					<button class="btn-confirm" @click="connectByMac">连接</button>
				</view>
			</view>
		</view>

		<!-- 设备列表弹窗 -->
		<view v-if="showDevices" class="device-modal">
			<view class="modal-content">
				<text class="modal-title">可用设备</text>
				<view v-for="device in devices" :key="device.deviceId" class="device-item" @click="connectDevice(device)">
					<text>{{ device.name || device.deviceId }}</text>
				</view>
				<button @click="showDevices = false">关闭</button>
			</view>
		</view>

		<!-- 传感器数据卡片 -->
		<view class="card-grid">
			<view class="card">
				<text class="card-label">温度</text>
				<text class="card-value">{{ sensorData.temperature }}°C</text>
			</view>
			<view class="card">
				<text class="card-label">湿度</text>
				<text class="card-value">{{ sensorData.humidity }}%</text>
			</view>
			<view class="card">
				<text class="card-label">土壤湿度</text>
				<text class="card-value">{{ sensorData.soil }}%</text>
			</view>
			<view class="card">
				<text class="card-label">光照</text>
				<text class="card-value">{{ sensorData.light }}lux</text>
			</view>
		</view>

		<!-- 温度和湿度柱状图 -->
		<view class="chart-card">
			<text class="chart-title">温度和湿度历史数据</text>
			<view class="bar-chart">
				<view v-for="(v, i) in tempHistory" :key="i" class="bar-item">
					<view class="bar bar-temp" :style="{ height: (v / 40 * 200) + 'rpx' }"></view>
					<text class="bar-value">{{ v }}°</text>
				</view>
				<view v-for="(v, i) in humidityHistory" :key="i" class="bar-item">
					<view class="bar bar-humidity" :style="{ height: (v / 100 * 200) + 'rpx' }"></view>
					<text class="bar-value">{{ v }}%</text>
				</view>
			</view>
		</view>

		<!-- 土壤和光照柱状��� -->
		<view class="chart-card">
			<text class="chart-title">土壤湿度和光照历史数据 (%)</text>
			<view class="bar-chart">
				<view v-for="(v, i) in soilHistory" :key="i" class="bar-item">
					<view class="bar bar-soil" :style="{ height: (v / 100 * 200) + 'rpx' }"></view>
					<text class="bar-value">{{ v }}%</text>
				</view>
				<view v-for="(v, i) in lightHistory" :key="i" class="bar-item">
					<view class="bar bar-light" :style="{ height: (v / 100 * 200) + 'rpx' }"></view>
					<text class="bar-value">{{ v }}%</text>
				</view>
			</view>
		</view>

		<!-- 操作按钮 -->
		<view class="btn-group">
			<button class="btn-primary" @click="sendStatus" :disabled="statusCooldown || !connected">
				{{ statusCooldown ? statusCooldownText : '立即刷新' }}
			</button>
			<button class="btn-water" @click="sendWater" :disabled="waterCooldown || !connected">
				{{ waterCooldown ? waterCooldownText : '浇水' }}
			</button>
		</view>

		<!-- 日志 -->
		<view class="log-card">
			<text class="log-title">通信日志</text>
			<scroll-view scroll-y class="log-content">
				<text v-for="(log, i) in logs" :key="i" class="log-item" :class="log.type">{{ log.msg }}</text>
			</scroll-view>
		</view>
	</view>
</template>

<script>
export default {
	data() {
		return {
			connected: false,
			scanning: false,
			showDevices: false,
			showMacInput: false,
			manualMac: '',
			devices: [],
			BluetoothDevice: null,
			characteristic: null,

			sensorData: {
				temperature: '--',
				humidity: '--',
				soil: '--',
				light: '--'
			},

			// 历史数据（最近5个点）
			tempHistory: [],
			humidityHistory: [],
			soilHistory: [],
			lightHistory: [],
			timeLabels: [],
			chartWidth: 280,
			pointSpacing: 28,
			chartYAxis: ['50', '25', '0'],

			statusCooldown: false,
			waterCooldown: false,
			statusCooldownText: '',
			waterCooldownText: '',
			logs: [],
			rxBuffer: ''  // 接收缓冲区，用于拼接分包数据
		}
	},
	onLoad() {
		this.initBluetooth()
	},
	methods: {
		// 初始化蓝牙
		initBluetooth() {
			if (!uni.canIUse('getLocation')) {
				this.addLog('info', '注意：低功耗蓝牙可能需要定位权限')
			}

			uni.openBluetoothAdapter({
				success: (res) => {
					this.addLog('success', '蓝牙适配器已打开')
					this.startBluetoothStateListener()
				},
				fail: (err) => {
					this.addLog('error', '蓝牙适配器打开失败: ' + err.errMsg)
				}
			})
		},

		// 监听蓝牙状态
		startBluetoothStateListener() {
			uni.onBluetoothAdapterStateChange((res) => {
				if (!res.discovering && !res.available) {
					this.addLog('warn', '蓝牙适配器不可用')
					this.connected = false
				}
			})
		},

		// 检查权限
		checkPermissions(callback) {
			uni.openBluetoothAdapter({
				success: () => {
					callback && callback()
				},
				fail: (err) => {
					uni.showModal({
						title: '提示',
						content: '请在手机设置中开启蓝牙和定位权限',
						showCancel: false
					})
				}
			})
		},

		// 扫描设备
		scanDevices() {
			this.checkPermissions(() => {
				this.devices = []
				this.showDevices = true
				this.scanning = true

				uni.startBluetoothDevicesDiscovery({
					allowDuplicatesKey: false,
					success: () => {
						this.addLog('info', '开始扫描...')
						setTimeout(() => {
							uni.getBluetoothDevices({
								success: (res) => {
									this.devices = res.devices.filter(d =>
										d.name && d.name.includes('HC') || d.name && d.name.includes('irrigation')
									)
									if (this.devices.length === 0) {
										this.devices = res.devices.slice(0, 5)
									}
									this.scanning = false
									this.addLog('info', '发现 ' + this.devices.length + ' 个设备')
								}
							})
						}, 3000)
					},
					fail: (err) => {
						this.addLog('error', '扫描失败: ' + err.errMsg)
						this.scanning = false
					}
				})
			})
		},

		// 手动输入MAC连接
		connectByMac() {
			if (!this.manualMac) {
				this.addLog('warn', '请输入MAC地址')
				return
			}
			// 格式化MAC地址（去掉冒号转小写）
			let mac = this.manualMac.replace(/:/g, '').toUpperCase()
			if (mac.length !== 12) {
				this.addLog('warn', 'MAC地址格式不正确')
				return
			}
			// 格式化为标准格式
			let formattedMac = mac.match(/.{2}/g).join(':')
			this.showMacInput = false

			this.checkPermissions(() => {
				this.addLog('info', '正在连接 ' + formattedMac + '...')

				uni.createBLEConnection({
					deviceId: formattedMac,
					timeout: 10000,
					success: (res) => {
						this.connected = true
						this.BluetoothDevice = formattedMac
						this.addLog('success', '连接成功')
						this.stopScan()
						setTimeout(() => this.getServices(formattedMac), 500)
					},
					fail: (err) => {
						this.addLog('error', '连接失败: ' + err.errMsg)
					}
				})
			})
		},

		// 连接设备
		connectDevice(device) {
			this.showDevices = false
			this.addLog('info', '正在连接 ' + (device.name || device.deviceId) + '...')

			uni.createBLEConnection({
				deviceId: device.deviceId,
				timeout: 10000,
				success: (res) => {
					this.connected = true
					this.BluetoothDevice = device.deviceId
					this.addLog('success', '连接成功')
					this.stopScan()
					// 连接后立即获取服务和特征值
					setTimeout(() => this.getServices(device.deviceId), 500)
				},
				fail: (err) => {
					this.addLog('error', '连接失败: ' + err.errMsg)
				}
			})
		},

		// 获取服务
		getServices(deviceId) {
			uni.getBLEDeviceServices({
				deviceId: deviceId,
				success: (res) => {
					this.addLog('info', '发现 ' + res.services.length + ' 个服务')
					// 遍历服务找串口服务
					for (let service of res.services) {
						this.getCharacteristics(deviceId, service.uuid)
					}
				}
			})
		},

		// 获取特征值
		getCharacteristics(deviceId, serviceId) {
			uni.getBLEDeviceCharacteristics({
				deviceId: deviceId,
				serviceId: serviceId,
				success: (res) => {
					for (let char of res.characteristics) {
						// 找可读+可通知的特征值
						if (char.properties.read && char.properties.notify) {
							this.characteristic = {
								serviceId: serviceId,
								characteristicId: char.uuid
							}
							this.addLog('info', '找到可用特征值: ' + char.uuid)
							// 启用通知
							this.enableNotification(deviceId, serviceId, char.uuid)
							return
						}
					}
				}
			})
		},

		// 启用通知
		enableNotification(deviceId, serviceId, charId) {
			uni.notifyBLECharacteristicValueChange({
				deviceId: deviceId,
				serviceId: serviceId,
				characteristicId: charId,
				state: true,
				success: () => {
					this.addLog('success', '数据监听已开启')
					this.startDataListener()
				}
			})
		},

		// 监听数据
		startDataListener() {
			uni.onBLECharacteristicValueChange((res) => {
				// 将字节数组转换为字符串（ASCII解码）
				const arr = new Uint8Array(res.value)
				let str = ''
				for (let i = 0; i < arr.length; i++) {
					str += String.fromCharCode(arr[i])
				}

				// 将接收到的数据添加到缓冲区
				this.rxBuffer += str

				// 检查是否收到完整的数据包（以\r\n结尾）
				while (this.rxBuffer.includes('\r\n')) {
					const endIndex = this.rxBuffer.indexOf('\r\n')
					const completeData = this.rxBuffer.substring(0, endIndex)
					this.rxBuffer = this.rxBuffer.substring(endIndex + 2)

					if (completeData.length > 0) {
						this.addLog('recv', '收到完整数据: ' + completeData)
						this.parseData(completeData)
					}
				}
			})
		},

		// 停止扫描
		stopScan() {
			uni.stopBluetoothDevicesDiscovery()
			this.scanning = false
		},

		// 发送 STATUS 命令
		sendStatus() {
			if (!this.connected) {
				this.addLog('warn', '请先连接设备')
				return
			}
			const cmd = this.stringToBytes('STATUS\r\n')
			this.writeData(cmd)
			this.addLog('send', '发送: STATUS')
			this.startCooldown('status')
		},

		// 发送 WATER_NOW 命令
		sendWater() {
			if (!this.connected) {
				this.addLog('warn', '请先连接设备')
				return
			}
			const cmd = this.stringToBytes('WATER\r\n')
			this.writeData(cmd)
			this.addLog('send', '发送: WATER')
			this.startCooldown('water')
		},

		// 断开连接
		disconnect() {
			if (!this.connected) {
				this.addLog('warn', '未连接设备')
				return
			}

			uni.closeBLEConnection({
				deviceId: this.BluetoothDevice,
				success: () => {
					this.connected = false
					this.BluetoothDevice = null
					this.characteristic = null
					this.addLog('success', '已断开连接')
				},
				fail: (err) => this.addLog('error', '断开失败: ' + err.errMsg)
			})
		},

		// 写入数据
		writeData(buffer) {
			uni.writeBLECharacteristicValue({
				deviceId: this.BluetoothDevice,
				serviceId: this.characteristic.serviceId,
				characteristicId: this.characteristic.characteristicId,
				value: buffer,
				success: () => {},
				fail: (err) => this.addLog('error', '发送失败: ' + err.errMsg)
			})
		},

		// 解析数据 (格式: T25H60S045L080)
		parseData(hex) {
			this.addLog('recv', '收到: ' + hex)
			try {
				// 格式: TxxHxxSxxLxx 例如 T25H60S45L80
				const temp = parseInt(hex.match(/T(\d+)/)?.[1] || '0')
				const hum = parseInt(hex.match(/H(\d+)/)?.[1] || '0')
				const soil = parseInt(hex.match(/S(\d+)/)?.[1] || '0')
				const light = parseInt(hex.match(/L(\d+)/)?.[1] || '0')

				// 更新传感器数据（允许显示0值）
				this.sensorData.temperature = temp
				this.sensorData.humidity = hum
				this.sensorData.soil = soil
				this.sensorData.light = light

				// 土壤湿度传感器故障检测
				if (soil === 0) {
					uni.showModal({
						title: '传感器警告',
						content: '土壤传感器接收不良！\n请检查传感器是否正确插入土壤中，确保接触良好。',
						showCancel: false,
						confirmText: '我知道了'
					})
					this.addLog('warn', '土壤湿度传感器异常: 0%')
				}

				// 更新历史数据
				this.tempHistory.push(temp)
				this.humidityHistory.push(hum)
				this.soilHistory.push(soil)
				this.lightHistory.push(light)
				this.addLog('info', '历史数据更新: T=' + temp + ' H=' + hum + ' S=' + soil + ' L=' + light)
				if (this.tempHistory.length > 5) {
					this.tempHistory.shift()
					this.humidityHistory.shift()
					this.soilHistory.shift()
					this.lightHistory.shift()
				}

				// 更新时间标签
				const now = new Date()
				this.timeLabels.push(now.getHours() + ':' + String(now.getMinutes()).padStart(2, '0'))
				if (this.timeLabels.length > 5) this.timeLabels.shift()

			} catch (e) {
				this.addLog('warn', '数据解析失败')
			}
		},

		// 冷却倒计时
		startCooldown(type) {
			let seconds = 5
			this[type + 'Cooldown'] = true
			this[type + 'CooldownText'] = seconds + 's'

			const timer = setInterval(() => {
				seconds--
				this[type + 'CooldownText'] = seconds + 's'
				if (seconds <= 0) {
					clearInterval(timer)
					this[type + 'Cooldown'] = false
					this[type + 'CooldownText'] = ''
				}
			}, 1000)
		},

		// 添加日志
		addLog(type, msg) {
			this.logs.push({ type, msg })
			if (this.logs.length > 50) this.logs.shift()
		},

		// 工具方法
		ab2hex(buffer) {
			const arr = new Uint8Array(buffer)
			return Array.from(arr).map(v => v.toString(16).padStart(2, '0').toUpperCase()).join('')
		},

		stringToBytes(str) {
			const buffer = new ArrayBuffer(str.length)
			const view = new Uint8Array(buffer)
			for (let i = 0; i < str.length; i++) {
				view[i] = str.charCodeAt(i)
			}
			return buffer
		},

		hexToBytes(hex) {
			const bytes = new Uint8Array(hex.length / 2)
			for (let i = 0; i < hex.length; i += 2) {
				bytes[i / 2] = parseInt(hex.substr(i, 2), 16)
			}
			return bytes.buffer
		}
	}
}
</script>

<style>
.container {
	padding: 20rpx;
	background: #f5f5f5;
	min-height: 100vh;
}
.status-bar {
	display: flex;
	justify-content: space-between;
	align-items: center;
	background: #fff;
	padding: 20rpx;
	border-radius: 16rpx;
	margin-bottom: 20rpx;
}
.status-text {
	font-size: 28rpx;
	color: #333;
}
.status-btns {
	display: flex;
	gap: 10rpx;
}
.btn-small {
	font-size: 24rpx;
	padding: 10rpx 20rpx;
	background: #4CAF50;
	color: #fff;
	border-radius: 8rpx;
}
.btn-mac {
	background: #FF9800;
}
.btn-disconnect-status {
	background: #F44336;
}
.btn-small[disabled] {
	background: #ccc;
}
.mac-input {
	border: 1rpx solid #ddd;
	border-radius: 8rpx;
	padding: 16rpx;
	margin: 20rpx 0;
	font-size: 28rpx;
}
.modal-btns {
	display: flex;
	gap: 20rpx;
	margin-top: 20rpx;
}
.btn-cancel {
	flex: 1;
	background: #ccc;
	color: #333;
}
.btn-confirm {
	flex: 1;
	background: #4CAF50;
	color: #fff;
}
.device-modal {
	position: fixed;
	top: 0;
	left: 0;
	right: 0;
	bottom: 0;
	background: rgba(0,0,0,0.5);
	display: flex;
	align-items: center;
	justify-content: center;
	z-index: 999;
}
.modal-content {
	background: #fff;
	padding: 40rpx;
	border-radius: 20rpx;
	width: 80%;
	max-height: 60vh;
}
.modal-title {
	font-size: 32rpx;
	font-weight: bold;
	margin-bottom: 20rpx;
}
.device-item {
	padding: 20rpx;
	border-bottom: 1rpx solid #eee;
}
.card-grid {
	display: grid;
	grid-template-columns: 1fr 1fr;
	gap: 20rpx;
	margin-bottom: 20rpx;
}
.card {
	background: #fff;
	padding: 30rpx;
	border-radius: 16rpx;
	text-align: center;
}
.card-label {
	font-size: 24rpx;
	color: #666;
	display: block;
}
.card-value {
	font-size: 40rpx;
	font-weight: bold;
	color: #2196F3;
	display: block;
	margin-top: 10rpx;
}
.chart-card {
	background: #fff;
	padding: 20rpx;
	border-radius: 16rpx;
	margin-bottom: 15rpx;
}
.chart-title {
	font-size: 28rpx;
	font-weight: bold;
	display: block;
	margin-bottom: 20rpx;
}
.chart-container {
	display: flex;
	height: 200rpx;
}
.chart-y-axis {
	width: 60rpx;
	display: flex;
	flex-direction: column;
	justify-content: space-between;
}
.y-label, .x-label {
	font-size: 20rpx;
	color: #999;
}
.chart-area {
	flex: 1;
	position: relative;
	border-left: 1rpx solid #eee;
	border-bottom: 1rpx solid #eee;
}
.line-label {
	position: absolute;
	right: 10rpx;
	font-size: 20rpx;
	color: #999;
}
.line {
	position: absolute;
	bottom: 0;
	height: 2rpx;
}
.line-temp {
	background: #FF5722;
}
.line-soil {
	background: #2196F3;
}
.line-light {
	background: #FFC107;
}
.dot {
	position: absolute;
	width: 12rpx;
	height: 12rpx;
	border-radius: 50%;
	transform: translate(-50%, -50%);
}
.line-temp .dot {
	background: #FF5722;
}
.line-soil .dot {
	background: #2196F3;
}
.line-light .dot {
	background: #FFC107;
}
.bar-chart {
	display: flex;
	justify-content: space-around;
	align-items: flex-end;
	height: 300rpx;
	padding: 20rpx 10rpx;
}
.bar-item {
	display: flex;
	flex-direction: column;
	align-items: center;
	flex: 1;
	margin: 0 4rpx;
}
.bar {
	width: 50rpx;
	background: #2196F3;
	border-radius: 8rpx 8rpx 0 0;
	min-height: 4rpx;
	position: relative;
}
.bar::before {
	content: '';
	position: absolute;
	bottom: 0;
	left: 0;
	right: 0;
	height: 100%;
	background: inherit;
	border-radius: inherit;
}
.bar-temp {
	background: #FF5722;
}
.bar-soil {
	background: #2196F3;
}
.bar-light {
	background: #FFC107;
}
.bar-humidity {
	background: #9C27B0;
}
.bar-value {
	font-size: 24rpx;
	font-weight: bold;
	color: #333;
	margin-bottom: 8rpx;
}
.chart-x-axis {
	display: flex;
	justify-content: space-between;
	padding-left: 60rpx;
	margin-top: 10rpx;
}
.btn-group {
	display: flex;
	gap: 20rpx;
	margin-bottom: 20rpx;
}
.btn-primary, .btn-water {
	flex: 1;
	height: 100rpx;
	line-height: 100rpx;
	color: #fff;
	font-size: 32rpx;
	border-radius: 16rpx;
	border: none;
}
.btn-primary {
	background: #2196F3;
}
.btn-primary[disabled], .btn-water[disabled], .btn-disconnect[disabled] {
	background: #ccc;
}
.btn-water {
	background: #4CAF50;
}
.btn-disconnect {
	flex: 0 0 150rpx;
	height: 100rpx;
	line-height: 100rpx;
	color: #fff;
	font-size: 28rpx;
	background: #F44336;
	border-radius: 16rpx;
	border: none;
}
.log-card {
	background: #fff;
	padding: 30rpx;
	border-radius: 16rpx;
}
.log-title {
	font-size: 28rpx;
	font-weight: bold;
	display: block;
	margin-bottom: 20rpx;
}
.log-content {
	height: 200rpx;
}
.log-item {
	font-size: 22rpx;
	display: block;
	padding: 4rpx 0;
}
.log-item.success { color: #4CAF50; }
.log-item.error { color: #F44336; }
.log-item.warn { color: #FF9800; }
.log-item.send { color: #2196F3; }
.log-item.recv { color: #9C27B0; }
</style>
