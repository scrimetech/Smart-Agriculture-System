import { reactive } from 'vue'
import Paho from 'paho-mqtt'

// Configuration
const MQTT_CONFIG = {
    //host: 'mqtt.runner8.dpdns.org',
    host: '106.53.211.251',
    port: 8080,
    clientId: 'vue_client_fixed_id', // 使用固定 ID 避免被服务器踢掉
    path: '/mqtt', // Mosquitto 默认路径
}

// Reactive State
export const mqttState = reactive({
    connected: false,
    connecting: false, // 新增：表示“正在连接或保持连接意图”的状态
    error: null,
    // Sensor Data
    temp: 0,
    humidity: 0,
    soil: 0,
    light: 0, // NEW
    // System Status
    mode: 'MANUAL',
    pump: 'OFF',
    fan: 'OFF',
    led: 'OFF',
    // Thresholds
    tempMin: 20.0,
    tempMax: 30.0,
    soilMin: 30,
    soilMax: 80,
    lightMin: 20,
    lightMax: 90,
    // Alerts
    alert: null,
    reminders: [], // 新增：Web 端计算的提醒信息
    // Logs
    lastLog: null // 新增：用于透传日志到 UI
})

let client = null
let shouldAutoReconnect = false

// 辅助日志函数
function pushLog(msg, type = 'info') {
    mqttState.lastLog = { msg, type, ts: Date.now() }
}

export function disconnectMqtt() {
    shouldAutoReconnect = false // 禁止自动重连
    mqttState.connecting = false // 清除连接意图

    if (client) {
        pushLog('用户手动断开连接', 'info')
        console.log('Disconnecting MQTT...')
        try {
            if (client.isConnected()) client.disconnect()
        } catch (e) {
            console.error("Disconnect error:", e)
        }
    }
    mqttState.connected = false
}

export function connectMqtt() {
    // 如果已经在连接流程中（无论是已连接还是正在重试），则忽略
    if (mqttState.connecting) {
        console.log('Already in connecting state...')
        return
    }

    mqttState.connecting = true
    shouldAutoReconnect = true
    pushLog('开始尝试连接服务器...', 'info')

    _doConnect()
}

function _doConnect() {
    console.log('Connecting to MQTT via Paho...')

    // 如果之前有 client 实例，先清理，防止多重实例
    if (client) {
        try { client.disconnect() } catch (e) { }
        client = null
    }

    // 初始化 Paho 客户端
    // 添加时间戳到 ClientID 确保唯一性，避免被 Server 踢下线
    const uniqueId = MQTT_CONFIG.clientId + '_' + new Date().getTime().toString().substr(8)
    client = new Paho.Client(MQTT_CONFIG.host, MQTT_CONFIG.port, MQTT_CONFIG.path, uniqueId)

    // 重置错误状态，以便下一次失败能触发 UI 更新
    mqttState.error = null

    client.onConnectionLost = onConnectionLost
    client.onMessageArrived = onMessageArrived

    const connectOptions = {
        useSSL: false,
        userName: 'agri_user',
        password: '32768',
        keepAliveInterval: 60,
        mqttVersion: 4,
        onSuccess: onConnect,
        onFailure: onFailure
    }

    try {
        client.connect(connectOptions)
    } catch (e) {
        console.error("Connect exception:", e)
        onFailure({ errorMessage: e.message })
    }
}

function onConnect() {
    console.log('MQTT Connected')
    mqttState.connected = true
    mqttState.error = null
    // 注意：这里保持 mqttState.connecting = true

    pushLog('服务器连接成功!', 'success')

    // Subscribe
    client.subscribe('agri/#')
}

function onFailure(responseObject) {
    console.error('MQTT Connection Failed:', responseObject.errorMessage)
    mqttState.connected = false
    // 注意：这里不设 connecting = false，因为还要自动重连

    if (shouldAutoReconnect) {
        mqttState.error = `连接失败: ${responseObject.errorMessage}`
        pushLog(`连接失败，2秒后重试...`, 'error')

        setTimeout(() => {
            if (shouldAutoReconnect) _doConnect()
        }, 2000)
    } else {
        mqttState.connecting = false
    }
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log('Connection Lost:', responseObject.errorMessage)
        mqttState.connected = false
        pushLog(`连接异常中断: ${responseObject.errorMessage}`, 'error')

        if (shouldAutoReconnect) {
            // mqttState.error = `连接中断: ${responseObject.errorMessage}. 正在重连...` // 错误信息由 onFailure 处理
            setTimeout(() => {
                if (shouldAutoReconnect) _doConnect()
            }, 2000)
        }
    }
}

function onMessageArrived(message) {
    const topic = message.destinationName
    const payload = message.payloadString
    console.log('Msg:', topic, payload)

    switch (topic) {
        case 'agri/data/temp':
            mqttState.temp = parseFloat(payload).toFixed(1)
            break
        case 'agri/data/humi':
            mqttState.humidity = parseFloat(payload).toFixed(1)
            break
        case 'agri/data/soil':
            mqttState.soil = parseInt(payload)
            break
        case 'agri/status/mode':
            mqttState.mode = payload
            break
        case 'agri/status/pump':
            mqttState.pump = (payload === '1' || payload === 'ON') ? 'ON' : 'OFF'
            break
        case 'agri/status/fan':
            mqttState.fan = (payload === '1' || payload === 'ON') ? 'ON' : 'OFF'
            break
        case 'agri/status/led':
            mqttState.led = (payload === '1' || payload === 'ON') ? 'ON' : 'OFF'
            break
        case 'agri/status/threshold/temp/min':
            mqttState.tempMin = parseFloat(payload)
            break
        case 'agri/status/threshold/temp/max':
            mqttState.tempMax = parseFloat(payload)
            break
        case 'agri/status/threshold/soil/min':
            mqttState.soilMin = parseInt(payload)
            break
        case 'agri/status/threshold/soil/max':
            mqttState.soilMax = parseInt(payload)
            break
        case 'agri/status/threshold/light/min':
            mqttState.lightMin = parseInt(payload)
            break
        case 'agri/status/threshold/light/max':
            mqttState.lightMax = parseInt(payload)
            break
        case 'agri/data/light':
            mqttState.light = parseInt(payload)
            break
        case 'agri/alert':
            mqttState.alert = payload
            break
        case 'agri/cmd/mode':
            mqttState.mode = payload
            break
        case 'agri/cmd/pump':
            mqttState.pump = payload;
            break;
        case 'agri/cmd/fan':
            mqttState.fan = payload;
            break;
        case 'agri/cmd/led':
            mqttState.led = payload;
            break;
    }
}

// Actions
export function sendCommand(topic, payload) {
    if (!client || !mqttState.connected) return
    const message = new Paho.Message(payload)
    message.destinationName = topic
    client.send(message)
}

export function setMode(mode) {
    sendCommand('agri/cmd/mode', mode)
}

export function toggleDevice(device, state) {
    sendCommand(`agri/cmd/${device}`, state)
}

export function setThreshold(type, target, value) {
    // target should be 'min' or 'max'
    const topic = `agri/set/threshold/${type}/${target}`
    sendCommand(topic, value.toString())
}
