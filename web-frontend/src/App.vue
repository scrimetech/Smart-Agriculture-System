<template>
  <div class="dashboard-root">
    <!-- Header -->
    <header class="app-header">
      <div class="logo-area">
        <span class="logo-icon">🌿</span>
        <h1>智慧农业后台管理系统</h1>
        <div class="conn-status-indicator" :class="{ online: mqttState.connected }">
          <span class="breathing-dot"></span>
          {{ mqttState.connected ? '系统在线' : '离线' }}
        </div>
      </div>
      <div class="header-right">
        <button 
          class="connection-btn" 
          :class="{ online: mqttState.connected, connecting: mqttState.connecting }"
          @click="handleConnection"
        >
          <span class="btn-text">
            {{ mqttState.connected ? '断开服务器' : (mqttState.connecting ? '正在连接...' : '连接服务器') }}
          </span>
        </button>
      </div>
    </header>

    <!-- Main Content -->
    <main class="main-content">
      <!-- C1: Sensors (Left 20%) -->
      <aside class="grid-sensors">
        <div class="full-width-header-box">
          <span class="header-icon">📊</span>
          <span class="header-text">实时环境监测数据</span>
        </div>
        <div class="sensor-stack">
          <SensorCard title="土壤湿度" :value="mqttState.soil" unit="%" type="soil" :isWarning="Number(mqttState.soil) < mqttState.soilMin || Number(mqttState.soil) > mqttState.soilMax" />
          <SensorCard title="环境温度" :value="mqttState.temp" unit="°C" type="temp" :isWarning="Number(mqttState.temp) < mqttState.tempMin || Number(mqttState.temp) > mqttState.tempMax" />
          <SensorCard title="空气湿度" :value="mqttState.humidity" unit="%" type="humidity" />
          <SensorCard title="光照强度" :value="mqttState.light" unit="%" type="light" :isWarning="Number(mqttState.light) < mqttState.lightMin || Number(mqttState.light) > mqttState.lightMax" />
        </div>
      </aside>

      <!-- Middle & Right Columns mapped via Grid -->
      <section class="panel panel-trends grid-trends">
        <div class="panel-header-inner left-align">
          <div class="header-left">
            <span class="p-icon">📈</span>
            <span class="p-title">环境数据趋势图</span>
          </div>
          <button class="clear-chart-btn" @click="clearChart">清空数据</button>
        </div>
        <div class="panel-body">
          <ChartView 
            ref="chartViewRef"
            :temp="Number(mqttState.temp)" 
            :humidity="Number(mqttState.humidity)"
            :soil="Number(mqttState.soil)" 
            :light="Number(mqttState.light)"
          />
        </div>
      </section>
      
      <div class="panel-row-split-bottom grid-controls">
        <section class="panel panel-half">
          <div class="panel-body scroll-auto">
            <ControlPanel 
              :mode="mqttState.mode"
              :pump="mqttState.pump"
              :fan="mqttState.fan"
              :led="mqttState.led"
              @set-mode="setMode"
              @toggle-device="handleToggle"
            />
          </div>
        </section>

        <section class="panel panel-half">
          <div class="panel-body scroll-auto">
            <ThresholdSettings 
              :tempMin="mqttState.tempMin"
              :tempMax="mqttState.tempMax"
              :soilMin="mqttState.soilMin"
              :soilMax="mqttState.soilMax"
              :lightMin="mqttState.lightMin"
              :lightMax="mqttState.lightMax"
              @set-threshold="handleThreshold"
            />
          </div>
        </section>
      </div>

      <section class="panel panel-alerts grid-alerts">
        <div class="panel-header-inner left-align">
          <span class="p-icon">🔔</span>
          <span class="p-title">系统状态提醒</span>
        </div>
        <div class="panel-body scroll-auto">
          <div v-if="mqttState.alert" class="msg-box hardware-alert">
            <span class="msg-icon">🚨</span>
            <div class="msg-cnt">
              <strong>硬件异常警报</strong>
              <p>{{ mqttState.alert }}</p>
            </div>
          </div>
          
          <div v-if="mqttState.reminders.length > 0" class="reminder-list">
            <div v-for="(rem, idx) in mqttState.reminders" :key="idx" class="msg-box suggest-alert">
              <span class="msg-icon">💡</span>
              <div class="msg-cnt">
                <p>{{ rem }}</p>
              </div>
            </div>
          </div>
          <div v-else-if="!mqttState.alert" class="no-data">
            <p>系统运行环境平稳，暂无建议</p>
          </div>
        </div>
      </section>

      <section class="panel panel-logs grid-logs">
        <div class="panel-body no-padding">
          <SystemLog :mqttState="mqttState" />
        </div>
      </section>
    </main>
  </div>
</template>

<script setup>
import { onMounted, watch, ref } from 'vue'
import { mqttState, connectMqtt, disconnectMqtt, setMode, toggleDevice, setThreshold } from './services/mqtt'
import SensorCard from './components/SensorCard.vue'
import ControlPanel from './components/ControlPanel.vue'
import ThresholdSettings from './components/ThresholdSettings.vue'
import ChartView from './components/ChartView.vue'
import SystemLog from './components/SystemLog.vue' 

const chartViewRef = ref(null)

// 提醒逻辑维护 - 确保始终显示 3 个项目的状态，以保持 UI 均衡
watch(
  () => [mqttState.mode, mqttState.temp, mqttState.soil, mqttState.light, 
         mqttState.tempMin, mqttState.tempMax, 
         mqttState.soilMin, mqttState.soilMax, 
         mqttState.lightMin, mqttState.lightMax],
  () => {
    const list = []
    
    // 1. 土壤湿度范围判断
    const s = Number(mqttState.soil)
    if (s < mqttState.soilMin) {
      list.push(`土壤水分 (${s}%) 低于阈值 ${mqttState.soilMin}%，建议开启灌溉。`)
    } else if (s > mqttState.soilMax) {
      list.push(`土壤水分 (${s}%) 已达上限 ${mqttState.soilMax}%，请停止灌溉。`)
    } else {
      list.push(`土壤水分 (${s}%) 充足，处于理想状态。`)
    }

    // 2. 温度范围判断
    const t = Number(mqttState.temp)
    if (t > mqttState.tempMax) {
      list.push(`当前温度 ${t}°C 已超出上限 ${mqttState.tempMax}°C，建议开启风扇。`)
    } else if (t < mqttState.tempMin) {
      list.push(`当前温度 ${t}°C 低于下限 ${mqttState.tempMin}°C，请注意环境防寒。`)
    } else {
      list.push(`当前温度 ${t}°C 处于最佳区间 [${mqttState.tempMin}, ${mqttState.tempMax}]。`)
    }

    // 3. 光照强度范围判断
    const l = Number(mqttState.light)
    if (l < mqttState.lightMin) {
      list.push(`光照强度 (${l}%) 不足，低于 ${mqttState.lightMin}%，建议开启补光。`)
    } else if (l > mqttState.lightMax) {
      list.push(`光照强度 (${l}%) 过强，超出 ${mqttState.lightMax}%，建议遮阴。`)
    } else {
      list.push(`当前光照强度 (${l}%) 适宜作物生长。`)
    }

    mqttState.reminders = list
  },
  { immediate: true }
)

function handleConnection() {
  mqttState.connecting ? disconnectMqtt() : connectMqtt()
}

function handleToggle({ device, state }) { toggleDevice(device, state) }
function handleThreshold({ type, target, value }) { setThreshold(type, target, value) }

function clearChart() {
  if (chartViewRef.value?.clearHistory) {
    chartViewRef.value.clearHistory()
  }
}
</script>

<style>
:root {
  --primary-green: #4caf50;
  --bg-color: #f0f2f5;
  --panel-bg: #fff;
  --border-color: #f0f0f0;
  --text-main: #262626;
}

body, html, #app {
  margin: 0; padding: 0;
  height: 100vh; width: 100vw;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
  background-color: var(--bg-color);
  color: var(--text-main);
}

.dashboard-root {
  display: flex;
  flex-direction: column;
  height: 100vh;
}

/* Header Styles */
.app-header {
  height: 60px;
  background: #fff;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.06);
  z-index: 100;
}

.logo-area { display: flex; align-items: center; gap: 12px; }
.logo-icon { font-size: 1.6rem; }
.app-header h1 { font-size: 1.2rem; margin: 0; font-weight: 700; color: #135200; }

.conn-status-indicator {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 0.8rem;
  font-weight: 600;
  padding: 4px 12px;
  border-radius: 20px;
  background: #f5f5f5;
  color: #8c8c8c;
  margin-left: 16px;
}
.conn-status-indicator.online { background: #f6ffed; color: #389e0d; border: 1px solid #b7eb8f; }

.breathing-dot {
  width: 10px;
  height: 10px;
  background: #bfbfbf;
  border-radius: 50%;
}
.online .breathing-dot {
  background: #52c41a;
  box-shadow: 0 0 10px #52c41a;
  animation: breathe 2s infinite ease-in-out;
}

@keyframes breathe {
  0%, 100% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.5; transform: scale(1.3); }
}

.connection-btn {
  padding: 8px 20px;
  border-radius: 6px;
  border: 1px solid #d9d9d9;
  font-size: 0.85rem;
  font-weight: 600;
  cursor: pointer;
  background: white;
  transition: all 0.3s;
}
.connection-btn.online { color: #f5222d; border-color: #ffa39e; }

/* Main Layout Styles */
.main-content {
  flex: 1;
  display: grid;
  grid-template-columns: 20fr 50fr 30fr; /* Strict 20:50:30 ratio */
  grid-template-rows: 1fr 380px; /* Absolute horizontal alignment line */
  gap: 16px;
  padding: 16px;
  width: 100%;
  box-sizing: border-box;
  overflow: hidden;
}

/* Structural Mapping (Locking the Horizon) */
.grid-sensors { grid-column: 1; grid-row: 1 / 3; display: flex; flex-direction: column; gap: 16px; }
.grid-trends { grid-column: 2; grid-row: 1; overflow: hidden; }
.grid-controls { grid-column: 2; grid-row: 2; }
.grid-alerts { grid-column: 3; grid-row: 1; overflow: hidden; }
.grid-logs { grid-column: 3; grid-row: 2; }

.full-width-header-box {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 10px 16px;
  background: #fff;
  border: 1px solid #e8e8e8;
  border-radius: 8px;
  width: 100%; /* Occupation full row as requested */
  box-sizing: border-box;
  color: #595959;
  font-weight: 800;
  font-size: 0.95rem;
  box-shadow: 0 2px 4px rgba(0,0,0,0.02);
}

/* Panel Styles */
.panel {
  background: white;
  border-radius: 12px;
  display: flex;
  flex-direction: column;
  box-shadow: 0 4px 16px rgba(0,0,0,0.04);
  overflow: hidden;
  border: 1px solid #f0f0f0;
}
.panel-body { flex: 1; min-height: 0; padding: 16px; }
.panel-body.no-padding { padding: 0; }
.scroll-auto { overflow-y: auto; }

.panel-header-inner {
  padding: 12px 16px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-bottom: 1px solid #f5f5f5;
}
.panel-header-inner.left-align { justify-content: flex-start; gap: 12px; }
.header-left { display: flex; align-items: center; gap: 10px; }
.p-icon { font-size: 1.1rem; }
.p-title { font-weight: 700; font-size: 0.95rem; color: #262626; }

.clear-chart-btn {
  margin-left: auto;
  background: #f0f0f0;
  border: none;
  padding: 3px 10px;
  border-radius: 4px;
  font-size: 0.75rem;
  color: #8c8c8c;
  cursor: pointer;
}
.clear-chart-btn:hover { background: #e8e8e8; }

.panel-row-split-bottom {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16px;
  height: 100%; /* Fill the 380px grid row */
}

.sensor-stack { 
  display: flex; 
  flex-direction: column; 
  gap: 10px; 
  flex: 1; 
  justify-content: space-between; 
}

.panel-trends { 
  height: 100%; 
  overflow: hidden; 
}
.panel-alerts { 
  height: 100%; 
  min-height: 0;
}
.panel-alerts .panel-body {
  display: flex;
  flex-direction: column;
}
.reminder-list {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
.panel-alerts .msg-box {
  flex: 1;
  margin-bottom: 0;
  display: flex;
  align-items: center;
}
.panel-alerts .msg-cnt {
  display: flex;
  align-items: center;
}

.panel-logs { 
  height: 100%; /* Fill the 380px grid row */
}

.panel-half { flex: 1; height: 100%; }

/* Message Styles */
.msg-box {
  display: flex;
  gap: 12px;
  padding: 12px;
  border-radius: 10px;
  margin-bottom: 10px;
  font-size: 0.9rem;
  border-left: 4px solid #ccc;
  line-height: 1.5;
  transition: transform 0.2s;
}
.msg-box:hover { transform: translateX(5px); }

.msg-cnt p { margin: 0; color: #595959; font-weight: 500; }

.hardware-alert { background: #fff1f0; border-color: #f5222d; margin-bottom: 12px !important; }
.suggest-alert { background: #fffbe6; border-color: #faad14; }

.no-data { text-align: center; color: #bfbfbf; padding: 40px 0; font-size: 0.85rem; }

/* Global component overrides */
:deep(.control-panel), :deep(.threshold-settings) { height: 100%; border: none !important; }
:deep(.system-log) { height: 100%; border: none !important; box-shadow: none !important; }
</style>
