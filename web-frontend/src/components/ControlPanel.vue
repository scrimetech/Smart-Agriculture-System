<template>
  <div class="control-panel">
    <div class="panel-header-inner">
      <span class="p-icon">🎮</span>
      <span class="p-title">设备控制中心</span>
    </div>
    
    <div class="content-box">
      <!-- Mode Switch -->
      <div class="mode-section">
        <span class="label">运行模式：</span>
        <div class="btn-group">
          <button 
            :class="{ active: mode === 'AUTO' }" 
            @click="$emit('set-mode', 'AUTO')"
          >自动</button>
          <button 
            :class="{ active: mode === 'MANUAL' }" 
            @click="$emit('set-mode', 'MANUAL')"
          >手动</button>
        </div>
      </div>

      <!-- Device Controls -->
      <div class="device-grid" :class="{ disabled: mode === 'AUTO' }">
        <div class="device-row">
          <div class="row-left">
            <span class="row-icon">🚰</span>
            <span class="device-name">灌溉水泵</span>
          </div>
          <button 
            class="toggle-btn"
            :class="pump === 'ON' ? 'on' : 'off'"
            @click="toggle('pump', pump)"
            :disabled="mode === 'AUTO'"
          >{{ pump === 'ON' ? '已开启' : '已关闭' }}</button>
        </div>
        
        <div class="device-row">
          <div class="row-left">
            <span class="row-icon">🌀</span>
            <span class="device-name">排风风扇</span>
          </div>
          <button 
            class="toggle-btn"
            :class="fan === 'ON' ? 'on' : 'off'"
            @click="toggle('fan', fan)"
            :disabled="mode === 'AUTO'"
          >{{ fan === 'ON' ? '已开启' : '已关闭' }}</button>
        </div>

         <div class="device-row">
          <div class="row-left">
            <span class="row-icon">💡</span>
            <span class="device-name">补光灯具</span>
          </div>
          <button 
            class="toggle-btn"
            :class="led === 'ON' ? 'on' : 'off'"
            @click="toggle('led', led)"
            :disabled="mode === 'AUTO'"
          >{{ led === 'ON' ? '已开启' : '已关闭' }}</button>
        </div>
      </div>
      
      <div v-if="mode === 'AUTO'" class="mode-info">
        <i class="icon">ℹ️</i> 自动模式下，系统将根据环境阈值自动调节设备状态
      </div>
    </div>
  </div>
</template>

<script setup>
defineProps({
  mode: String,
  pump: String,
  fan: String,
  led: String
})

const emit = defineEmits(['set-mode', 'toggle-device'])

function toggle(device, currentState) {
  const newState = currentState === 'ON' ? 'OFF' : 'ON'
  emit('toggle-device', { device, state: newState })
}
</script>

<style scoped>
.control-panel { 
  display: flex; 
  flex-direction: column; 
  height: 100%;
}

.panel-header-inner {
  display: flex;
  align-items: center;
  justify-content: flex-start; /* Force left align */
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
  gap: 16px;
}

.mode-section {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  background: #fcfcfc;
  border-radius: 8px;
}
.mode-section .label { font-size: 0.95rem; font-weight: 600; color: #444; }

.btn-group { display: flex; background: #f0f0f0; border-radius: 6px; padding: 2px; }
.btn-group button {
  border: none;
  padding: 6px 16px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 0.85rem;
  background: transparent;
  color: #666;
  transition: all 0.2s;
}
.btn-group button.active {
  background: white;
  color: #2e7d32;
  font-weight: bold;
  box-shadow: 0 1px 4px rgba(0,0,0,0.1);
}

.device-grid { 
  display: flex; 
  flex-direction: column; 
  gap: 12px;
  flex: 1;
}
.device-grid.disabled { opacity: 0.6; }

.device-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  background: #f8faf9;
  border: 1px solid #eef2f0;
  border-radius: 10px;
  flex: 1;
}

.row-left { display: flex; align-items: center; gap: 12px; }
.row-icon { font-size: 1.1rem; }
.device-name { font-size: 0.95rem; font-weight: 600; color: #444; }

.toggle-btn {
  padding: 6px 18px;
  border-radius: 6px;
  border: none;
  font-size: 0.85rem;
  font-weight: bold;
  cursor: pointer;
  min-width: 80px;
  transition: all 0.2s;
}
.toggle-btn.on { background: #4caf50; color: white; box-shadow: 0 2px 6px rgba(76,175,80,0.2); }
.toggle-btn.off { background: #eeeeee; color: #777; }
.toggle-btn:hover:not(:disabled) { transform: translateY(-1px); }

.mode-info {
  font-size: 0.8rem;
  color: #666;
  background: #f5f5f5;
  padding: 10px;
  border-radius: 8px;
  line-height: 1.4;
}
</style>
