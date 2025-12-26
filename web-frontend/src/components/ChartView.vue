<template>
  <div class="chart-wrapper">
    <div class="chart-container" ref="chartRef"></div>
    <div v-if="!timeData.length" class="chart-empty-hint">等待数据实时同步中...</div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch } from 'vue'
import * as echarts from 'echarts'

const props = defineProps({
  temp: Number,
  humidity: Number,
  soil: Number,
  light: Number
})

const chartRef = ref(null)
let chartInstance = null

const timeData = ref([])
const tempData = ref([])
const humidityData = ref([])
const soilData = ref([]) 
const lightData = ref([])

const STORAGE_KEY = 'agri_chart_data'

function loadHistory() {
  const saved = localStorage.getItem(STORAGE_KEY)
  if (saved) {
    try {
      const parsed = JSON.parse(saved)
      if(parsed.time && parsed.temp) {
         timeData.value = parsed.time.slice(-500)
         tempData.value = parsed.temp.slice(-500)
         humidityData.value = (parsed.humidity || []).slice(-500)
         soilData.value = (parsed.soil || []).slice(-500)
         lightData.value = (parsed.light || []).slice(-500)
      }
    } catch (e) { console.error('History load error', e) }
  }
}

function saveHistory() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify({
    time: timeData.value,
    temp: tempData.value,
    humidity: humidityData.value,
    soil: soilData.value,
    light: lightData.value
  }))
}

function clearHistory() {
  timeData.value = []
  tempData.value = []
  humidityData.value = []
  soilData.value = []
  lightData.value = []
  localStorage.removeItem(STORAGE_KEY)
  if (chartInstance) {
    chartInstance.clear()
    initChart()
  }
}

defineExpose({ clearHistory })

function initChart() {
  if (!chartRef.value) return
  if (!chartInstance) chartInstance = echarts.init(chartRef.value)
  
  if (timeData.value.length === 0) loadHistory()

  const option = {
    animation: false,
    tooltip: { trigger: 'axis' },
    legend: { data: ['温度', '空气湿度', '土壤湿度', '光照强度'], top: 10, right: 30 },
    grid: { 
      left: 55, 
      right: 25, 
      top: 30, 
      bottom: 25, // Absolute minimum gap
      containLabel: true 
    },
    xAxis: { 
      type: 'category', 
      data: timeData.value,
      axisLine: { show: true, lineStyle: { color: '#ccc' } },
      axisLabel: { show: true, fontSize: 10, margin: 4 }
    },
    yAxis: { 
      type: 'value', 
      axisLine: { show: true, lineStyle: { color: '#ccc' } },
      splitLine: { show: true, lineStyle: { type: 'dashed', color: '#f0f0f0' } },
      axisLabel: { fontSize: 10, margin: 4 }
    },
    dataZoom: [
      { type: 'inside', xAxisIndex: 0 },
      { 
        type: 'slider', 
        show: true, 
        xAxisIndex: 0, 
        bottom: 0, 
        height: 6, // Extremely thin 6px bar
        backgroundColor: 'transparent',
        fillerColor: 'rgba(76,175,80,0.1)',
        borderColor: 'transparent',
        handleSize: '0%', 
        showDetail: false
      }
    ],
    series: [
      { name: '温度', type: 'line', data: tempData.value, smooth: true, itemStyle: { color: '#ff4d4f' } },
      { name: '空气湿度', type: 'line', data: humidityData.value, smooth: true, itemStyle: { color: '#1890ff' } },
      { name: '土壤湿度', type: 'line', data: soilData.value, smooth: true, itemStyle: { color: '#52c41a' } },
      { name: '光照强度', type: 'line', data: lightData.value, smooth: true, itemStyle: { color: '#fa8c16' } }
    ]
  }
  chartInstance.setOption(option, true)
}


// Local cache for latest values to decouple render frequency from data frequency
const latestData = ref({ temp: 0, humidity: 0, soil: 0, light: 0 })
let updateTimer = null
let saveTimer = null

// Lightweight watcher: just update local cache, NO RENDER
watch(() => [props.temp, props.humidity, props.soil, props.light], ([t, h, s, l]) => {
  latestData.value = { t: Number(t), h: Number(h), s: Number(s), l: Number(l) }
}, { deep: true, immediate: true })

function handleVisibilityChange() {
  if (document.visibilityState === 'visible') {
    if (chartInstance) {
       chartInstance.resize()
       updateChart() // Force one update immediately on wake
    }
  }
}

onMounted(() => {
  setTimeout(() => {
    initChart()
    if (chartInstance) chartInstance.resize()
  }, 300)
  window.addEventListener('resize', () => chartInstance?.resize())
  document.addEventListener('visibilitychange', handleVisibilityChange)
  
  // FIXED RENDER LOOP: Update chart every 1.5s (matches firmware cycle roughly)
  // This prevents render-queue buildup during data bursts
  updateTimer = setInterval(updateChart, 1500)
  
  // Save history less frequently (every 5s) to reduce IO blocking
  saveTimer = setInterval(saveHistory, 5000)
})

onUnmounted(() => {
  if (updateTimer) clearInterval(updateTimer)
  if (saveTimer) clearInterval(saveTimer)
  window.removeEventListener('resize', () => chartInstance?.resize())
  document.removeEventListener('visibilitychange', handleVisibilityChange)
  if (chartInstance) chartInstance.dispose()
})

// Update function reads from latestData cache
function updateChart() {
  if (!chartInstance) return
  const now = new Date().toLocaleTimeString('zh-CN', { hour12: false })
  
  // Use cached latest values
  const { t, h, s, l } = latestData.value || { t:0, h:0, s:0, l:0 }

  if (timeData.value.length > 0 && timeData.value[timeData.value.length - 1] === now) {
    // If same second, overwrite (last update wins)
    tempData.value[tempData.value.length - 1] = t
    humidityData.value[humidityData.value.length - 1] = h
    soilData.value[soilData.value.length - 1] = s
    lightData.value[lightData.value.length - 1] = l
  } else {
    // New second, push new point
    timeData.value.push(now)
    tempData.value.push(t)
    humidityData.value.push(h)
    soilData.value.push(s)
    lightData.value.push(l)
    
    // Strict cap to prevent memory leak
    if (timeData.value.length > 1000) {
      timeData.value.shift()
      tempData.value.shift()
      humidityData.value.shift()
      soilData.value.shift()
      lightData.value.shift()
    }
  }
  
  // Note: saveHistory is now handled by separate timer

  let zoomStart = 0
  if (timeData.value.length > 25) {
    zoomStart = Math.max(0, Math.floor((timeData.value.length - 25) / timeData.value.length * 100))
  }

  chartInstance.setOption({
    xAxis: { data: timeData.value },
    series: [
      { data: tempData.value },
      { data: humidityData.value },
      { data: soilData.value },
      { data: lightData.value }
    ],
    dataZoom: [
      { type: 'inside' },
      { type: 'slider', height: 6, start: zoomStart, end: 100 }
    ]
  })
}
</script>

<style scoped>
.chart-wrapper {
  position: relative;
  width: 100%;
  height: 100%;
  min-height: 0; /* Important: allow shrinking below intrinsic size */
  flex: 1;
}
.chart-container {
  width: 100%;
  height: 100%;
  background: #fff;
}
.chart-empty-hint {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  color: #bfbfbf;
  font-size: 0.9rem;
  pointer-events: none;
}
</style>
