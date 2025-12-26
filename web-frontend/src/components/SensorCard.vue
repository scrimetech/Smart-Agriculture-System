<template>
  <div class="sensor-card" :class="{ warning: isWarning }">
    <div class="card-left">
      <div class="icon-circle" :class="type">
        <span class="icon">{{ getIcon(type) }}</span>
      </div>
      <div class="label-group">
        <span class="title">{{ title }}</span>
        <div class="status-indicator">
          <span class="status-dot" :class="isWarning ? 'warn' : 'normal'"></span>
          <span class="status-text" :class="isWarning ? 'warn' : 'normal'">
            {{ isWarning ? '数值异常' : '运行正常' }}
          </span>
        </div>
      </div>
    </div>
    <div class="card-right">
      <div class="value-box">
        <span class="value">{{ value }}</span>
        <span class="unit">{{ unit }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
defineProps({
  title: String,
  value: [String, Number],
  unit: String,
  type: String, 
  isWarning: Boolean
})

function getIcon(type) {
  const icons = {
    temp: '🌡️',
    humidity: '💧',
    soil: '🌱',
    light: '☀️'
  }
  return icons[type] || '📊'
}
</script>

<style scoped>
.sensor-card {
  background: white;
  border-radius: 12px;
  padding: 20px 24px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  box-shadow: 0 4px 12px rgba(0,0,0,0.03);
  border: 1px solid #f0f0f0;
  transition: all 0.3s ease;
  min-height: 105px; /* Increased from 90 */
  flex: 1; /* Allow stretching */
}

.card-left {
  display: flex;
  align-items: center;
  gap: 16px;
}

.icon-circle {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 1.5rem;
}

.icon-circle.temp { background: #fff1f0; }
.icon-circle.humidity { background: #e6f7ff; }
.icon-circle.soil { background: #f6ffed; }
.icon-circle.light { background: #fffbe6; }

.label-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.title {
  font-size: 1.1rem;
  font-weight: 700;
  color: #262626;
}

.status-indicator {
  display: flex;
  align-items: center;
  gap: 6px;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}
.status-dot.normal { background: #52c41a; }
.status-dot.warn { background: #f5222d; }

.status-text {
  font-size: 0.8rem;
  font-weight: 600;
}
.status-text.normal { color: #52c41a; }
.status-text.warn { color: #f5222d; }

.card-right {
  display: flex;
  align-items: center;
}

.value-box {
  background: #fbfbfb;
  padding: 10px 16px;
  border-radius: 8px;
  border: 1px solid #f0f0f0;
  min-width: 90px;
  text-align: right;
}

.value {
  font-size: 2rem;
  font-weight: 800;
  color: #1a1a1a;
  line-height: 1;
}

.unit {
  font-size: 0.95rem;
  color: #8c8c8c;
  margin-left: 3px;
  font-weight: 600;
}

.sensor-card.warning {
  border-color: #ffa39e;
  background: #fffcfc;
  animation: subtle-blink 2s infinite ease-in-out;
}

@keyframes subtle-blink {
  0%, 100% { box-shadow: 0 4px 12px rgba(0,0,0,0.03); }
  50% { box-shadow: 0 0 10px rgba(255, 77, 79, 0.1); border-color: #ffccc7; }
}

.warning .value { color: #f5222d; }
</style>
