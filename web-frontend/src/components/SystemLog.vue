<template>
  <div class="system-log">
    <div class="header">
      <h3>📜 系统运行日志</h3>
      <button @click="clearLogs" class="clear-btn">清空</button>
    </div>
    <div class="log-window" ref="logWindow">
      <div v-if="logs.length === 0" class="empty-tip">暂无日志...</div>
      <div v-for="(log, index) in logs" :key="index" class="log-item">
        <span class="time">[{{ log.time }}]</span>
        <span class="content" :class="log.type">{{ log.msg }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, watch } from 'vue'

const props = defineProps({
  mqttState: Object
})

const logs = ref([])
const logWindow = ref(null)

function addLog(msg, type = 'info') {
  const time = new Date().toLocaleTimeString()
  logs.value.push({ time, msg, type }) // 最新在最下 (push)
  if (logs.value.length > 50) logs.value.shift() // 移除最早的

  // 自动滚动到底部 (改为即时滚动以消除跳动感)
  setTimeout(() => {
    if (logWindow.value) {
      logWindow.value.scrollTop = logWindow.value.scrollHeight
    }
  }, 0)
}

function clearLogs() {
  logs.value = []
}

// 监听 MQTT 状态变化自动记录
watch(() => props.mqttState.connected, (newVal) => {
  addLog(newVal ? "MQTT 连接成功" : "MQTT 连接断开", newVal ? 'success' : 'error')
})
// ... existing watches

watch(() => props.mqttState.mode, (newVal) => {
  addLog(`模式切换为: ${newVal === 'AUTO' ? '自动' : '手动'}`, 'info')
})

watch(() => props.mqttState.pump, (newVal) => {
  addLog(`水泵状态更新: ${newVal === 'ON' ? '开启' : '关闭'}`, 'info')
})

watch(() => props.mqttState.fan, (newVal) => {
  addLog(`排风扇状态更新: ${newVal === 'ON' ? '开启' : '关闭'}`, 'info')
})

watch(() => props.mqttState.led, (newVal) => {
  addLog(`照明灯状态更新: ${newVal === 'ON' ? '开启' : '关闭'}`, 'info')
})

watch(() => props.mqttState.alert, (newVal) => {
  if (newVal) addLog(`警报: ${newVal}`, 'error')
})

watch(() => props.mqttState.lastLog, (newVal) => {
  if (newVal) addLog(newVal.msg, newVal.type)
})

onMounted(() => {
  addLog("系统日志模块就绪", 'info')
})
</script>

<style scoped>
.system-log {
  background: white;
  border-radius: 12px;
  padding: 15px;
  box-shadow: 0 2px 4px rgba(0,0,0,0.05);
  height: 100%; 
  display: flex;
  flex-direction: column;
  box-sizing: border-box;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  border-bottom: 1px solid #eee;
  padding-bottom: 5px;
}

.header h3 {
  margin: 0;
  font-size: 1.1rem;
}

.clear-btn {
  background: none;
  border: none;
  color: #999;
  cursor: pointer;
  font-size: 0.8rem;
}
.clear-btn:hover { color: #666; }

.log-window {
  flex: 1;
  overflow-y: auto;
  font-family: monospace;
  font-size: 0.9rem;
  padding-bottom: 20px; /* Added padding to ensure last log is visible */
}

.log-item {
  margin-bottom: 6px;
  border-bottom: 1px dashed #f0f0f0;
  padding-bottom: 4px;
}

.time {
  color: #999;
  margin-right: 8px;
}

.content.info { color: #555; }
.content.success { color: #16a34a; font-weight: bold; }
.content.error { color: #dc2626; font-weight: bold; }

.empty-tip {
  text-align: center;
  color: #ccc;
  margin-top: 50px;
}
</style>
