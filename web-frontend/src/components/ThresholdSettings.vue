<template>
  <div class="threshold-settings">
    <div class="panel-header-inner">
      <span class="p-icon">⚙️</span>
      <span class="p-title">自动控制阈值</span>
    </div>

    <div class="content-box">
      <!-- Soil Humidity -->
      <div class="t-row">
        <div class="row-left">
          <span class="row-icon">💧</span>
          <span>土壤湿度阈值(%)</span>
        </div>
        <div class="row-right-inputs">
          <input type="number" :value="soilMin" @change="updateThreshold('soil', 'min', $event)" step="1" placeholder="下限" />
          <input type="number" :value="soilMax" @change="updateThreshold('soil', 'max', $event)" step="1" placeholder="上限" />
        </div>
      </div>

      <!-- Temperature -->
      <div class="t-row">
        <div class="row-left">
          <span class="row-icon">🌡️</span>
          <span>环境温度阈值(°C)</span>
        </div>
        <div class="row-right-inputs">
          <input type="number" :value="tempMin" @change="updateThreshold('temp', 'min', $event)" step="0.5" placeholder="下限" />
          <input type="number" :value="tempMax" @change="updateThreshold('temp', 'max', $event)" step="0.5" placeholder="上限" />
        </div>
      </div>
      
      <!-- Light Intensity -->
      <div class="t-row">
        <div class="row-left">
          <span class="row-icon">☀️</span>
          <span>光照强度阈值(%)</span>
        </div>
        <div class="row-right-inputs">
          <input type="number" :value="lightMin" @change="updateThreshold('light', 'min', $event)" step="1" placeholder="下限" />
          <input type="number" :value="lightMax" @change="updateThreshold('light', 'max', $event)" step="1" placeholder="上限" />
        </div>
      </div>

      <div class="threshold-tip">
        提示：当环境数值低于（或高于温度）设定的临界值时，系统将在自动模式下触发相应设备。
      </div>
    </div>
  </div>
</template>

<script setup>
defineProps({
  tempMin: Number,
  tempMax: Number,
  soilMin: Number,
  soilMax: Number,
  lightMin: Number,
  lightMax: Number
})

const emit = defineEmits(['set-threshold'])

function updateThreshold(type, target, e) {
  const val = parseFloat(e.target.value)
  if (!isNaN(val)) emit('set-threshold', { type, target, value: val })
}
</script>

<style scoped>
.threshold-settings {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.panel-header-inner {
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 12px;
  padding-bottom: 8px;
  border-bottom: 1px solid #f2f2f2;
  margin-bottom: 12px;
}
.p-icon { font-size: 1.1rem; }
.p-title { font-weight: 700; font-size: 0.95rem; color: #434343; }

.content-box {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 12px;
}

.t-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: #fffcf8;
  border: 1px solid #fff3e6;
  border-radius: 10px;
  flex: 1;
}

.row-left { display: flex; align-items: center; gap: 10px; flex: 1; }
.row-icon { font-size: 1.1rem; }
.row-left span:not(.row-icon) { font-size: 0.9rem; font-weight: 600; color: #5d4037; }

.row-right-inputs {
  display: flex;
  gap: 12px;
  align-items: center;
}

.row-right-inputs input {
  width: 60px;
  padding: 6px;
  border: 1px solid #e8e8e8;
  border-radius: 6px;
  text-align: center;
  font-weight: 700;
  font-size: 0.95rem;
  color: #262626;
  background: #fff;
}
.row-right-inputs input:focus { border-color: #ffb74d; outline: none; box-shadow: 0 0 5px rgba(255,183,77,0.15); }

.threshold-tip {
  font-size: 0.75rem;
  color: #a0a0a0;
  background: #f9f9f9;
  padding: 8px 12px;
  border-radius: 8px;
  font-style: italic;
  line-height: 1.4;
}
</style>
