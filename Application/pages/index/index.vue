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

		<!-- 折线图 -->
		<view class="chart-card">
			<text class="chart-title">历史数据</text>
			<view class="chart-container">
				<view class="chart-y-axis">
					<text v-for="(val, i) in chartYAxis" :key="i" class="y-label">{{ val }}</text>
				</view>
				<view class="chart-area">
					<!-- 温度曲线 -->
					<view class="line-label" style="top: 10%">温度</view>
					<view class="line line-temp" :style="{ width: chartWidth + 'px' }">
						<view v-for="(v, i) in tempHistory" :key="i" class="dot" :style="{ left: (i * pointSpacing) + 'px', bottom: (v / 100 * 80) + '%' }"></view>
					</view>
					<!-- 土壤湿度曲线 -->
					<view class="line-label" style="top: 60%">土壤</view>
					<view class="line line-soil" :style="{ width: chartWidth + 'px' }">
						<view v-for="(v, i) in soilHistory" :key="i" class="dot" :style="{ left: (i * pointSpacing) + 'px', bottom: (v / 100 * 80) + '%' }"></view>
					</view>
				</view>
			</view>
			<view class="chart-x-axis">
				<text v-for="(t, i) in timeLabels" :key="i" class="x-label">{{ t }}</text>
			</view>
		</view>

		<!-- 操作按钮 -->
		<view class="btn-group">
			<button class="btn-primary" @click="sendStatus" :disabled="statusCooldown || !connected">
				{{ statusCooldown ? cooldownText : '立即刷新' }}
			</button>
			<button class="btn-water" @click="sendWater" :disabled="waterCooldown || !connected">
				{{ waterCooldown ? cooldownText : '浇水' }}
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

			// 历史数据（最近10个点）
			tempHistory: [],
			soilHistory: [],
			timeLabels: [],
			chartWidth: 280,
			pointSpacing: 28,
			chartYAxis: ['100', '50', '0'],

			statusCooldown: false,
			waterCooldown: false,
			cooldownText: '',
			logs: []
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
				const hex = this.ab2hex(res.value)
				this.parseData(hex)
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

				if (temp) this.sensorData.temperature = temp
				if (hum) this.sensorData.humidity = hum
				if (soil) this.sensorData.soil = soil
				if (light) this.sensorData.light = light

				// 更新历史
				this.tempHistory.push(temp)
				this.soilHistory.push(soil)
				if (this.tempHistory.length > 10) {
					this.tempHistory.shift()
					this.soilHistory.shift()
				}

				// 更新时间标签
				const now = new Date()
				this.timeLabels.push(now.getHours() + ':' + String(now.getMinutes()).padStart(2, '0'))
				if (this.timeLabels.length > 10) this.timeLabels.shift()

			} catch (e) {
				this.addLog('warn', '数据解析失败')
			}
		},

		// 冷却倒计时
		startCooldown(type) {
			let seconds = 5
			this[type + 'Cooldown'] = true
			this.cooldownText = seconds + 's'

			const timer = setInterval(() => {
				seconds--
				this.cooldownText = seconds + 's'
				if (seconds <= 0) {
					clearInterval(timer)
					this[type + 'Cooldown'] = false
					this.cooldownText = ''
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
	padding: 30rpx;
	border-radius: 16rpx;
	margin-bottom: 20rpx;
}
.chart-title {
	font-size: 28rpx;
	font-weight: bold;
	display: block;
	margin-bottom: 20rpx;
}
.chart-container {
	display: flex;
	height: 300rpx;
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
.dot {
	position: absolute;
	width: 12rpx;
	height: 12rpx;
	border-radius: 50%;
	transform: translate(-50%, 50%);
}
.line-temp .dot {
	background: #FF5722;
}
.line-soil .dot {
	background: #2196F3;
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
.btn-primary[disabled], .btn-water[disabled] {
	background: #ccc;
}
.btn-water {
	background: #4CAF50;
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
