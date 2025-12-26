#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp8266.h"
#include "sensor.h"
#include "control.h"

// MQTT全局定义
#define MQTT_BROKER   "106.53.211.251"
#define MQTT_PORT     1883
#define MQTT_USER     "agri_user"
#define MQTT_PASS     "32768"
#define MQTT_CLIENT_ID "stm32_firmware_001"

// 系统模式
#define MODE_MANUAL 0
#define MODE_AUTO   1

// Report Bits (Bitmask for MQTT reporting)
#define REPORT_MODE      (1 << 0)
#define REPORT_PUMP      (1 << 1)
#define REPORT_FAN       (1 << 2)
#define REPORT_LED       (1 << 3)
#define REPORT_T_MIN     (1 << 4)
#define REPORT_T_MAX     (1 << 5)
#define REPORT_S_MIN     (1 << 6)
#define REPORT_S_MAX     (1 << 7)
#define REPORT_L_MIN     (1 << 8)
#define REPORT_L_MAX     (1 << 9)
#define REPORT_ALL       (0x3FF)

// 系统变量、状态结构体
typedef struct 
{
    // 传感器数据 & 状态
    float temp;
    float humi;
    int soil;
    int light;         
    uint8_t pump_status; // 1 = ON, 0 = OFF
    uint8_t fan_status;
    uint8_t led_status;
	
    uint8_t mode;        // MODE_AUTO / MODE_MANUAL
    
    uint32_t report_mask; 

    // 阈值变量
    float temp_min;
    float temp_max;
    int soil_min;
    int soil_max;
    int light_min;
    int light_max;
} SystemState_t;

// 结构体变量赋默认值
SystemState_t sysState = 
{
    .mode = MODE_MANUAL,
    .temp_min = 20.0,
    .temp_max = 30.0,
    .soil_min = 30,
    .soil_max = 80,
    .light_min = 20,
    .light_max = 90,
    .report_mask = 0
};


// --- FreeRTOS 任务 ---
TaskHandle_t StartTask_Handler;
TaskHandle_t MQTTTask_Handler;
TaskHandle_t SensorTask_Handler;
TaskHandle_t ControlTask_Handler;

SemaphoreHandle_t xMutexState; // 互斥量
QueueHandle_t xQueueCmds;      // 队列

// 命令结构体
typedef struct 
{
    uint8_t type;   // 0 = 模式, 1 = 设备, 2 = 阈值
    uint8_t target; 
    uint8_t limit_type; // 0 = Min, 1 = Max
    float value;    
} CmdMsg_t;

// 任务函数
void Start_Task(void *pvParameters);
void MQTT_Task(void *pvParameters);
void Sensor_Task(void *pvParameters);
void Control_Task(void *pvParameters);

int main(void) 
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    
    // 创建开始任务
    xTaskCreate((TaskFunction_t)Start_Task,
                (const char*)"Start_Task",
                (uint16_t)128,
                (void*)NULL,
                (UBaseType_t)1,
                (TaskHandle_t*)&StartTask_Handler);
                
    vTaskStartScheduler();
    
    while(1);
}

void Start_Task(void *pvParameters) 
{
    taskENTER_CRITICAL();
    
    // 硬件初始化
    Control_Init();
    Sensor_Init();
    ESP8266_Init(); 
    
    xMutexState = xSemaphoreCreateMutex();
    xQueueCmds = xQueueCreate(10, sizeof(CmdMsg_t));
    
    // 创建任务
    xTaskCreate(MQTT_Task, "MQTT", 768, NULL, 2, &MQTTTask_Handler);
    xTaskCreate(Sensor_Task, "Sensor", 256, NULL, 1, &SensorTask_Handler);
    xTaskCreate(Control_Task, "Control", 256, NULL, 3, &ControlTask_Handler); 
    
    taskEXIT_CRITICAL(); // 退出循环之前删除当前任务
    vTaskDelete(StartTask_Handler); // 删除该任务
}

// --- MQTT Task ---
void MQTT_Task(void *pvParameters) 
{
    char rx_line[256];
    char tx_buf[256];
    uint8_t connected = 0;
	
	uint8_t pub_step = 0;
	TickType_t last_step_tick = 0;
    
    vTaskDelay(2000); 
    
    while(1) 
	{
        if (!connected) 
		{
			// 1、清除连接
			ESP8266_SendString("AT+MQTTCLEAN=0\r\n");
			vTaskDelay(2000);
			
            // 2、WIFI连接
            while (!ESP8266_SendCmd("AT+CWJAP=\"ddp\",\"12345678\"\r\n", "WIFI GOT IP", 10000)) 
			{
                vTaskDelay(2000);
            }
            
            // 3、MQTT配置
            sprintf(tx_buf, "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n", 
                           MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
            while (!ESP8266_SendCmd(tx_buf, "OK", 5000)) 
			{
                vTaskDelay(2000);
            }
            
            // 4、MQTT连接
            sprintf(tx_buf, "AT+MQTTCONN=0,\"%s\",%d,1\r\n", MQTT_BROKER, MQTT_PORT);
            if (ESP8266_SendCmd(tx_buf, "OK", 5000)) 
			{
                connected = 1;
                // 订阅主题
                ESP8266_SendCmd("AT+MQTTSUB=0,\"agri/cmd/#\",1\r\n", "OK", 4000);
				vTaskDelay(200);
                ESP8266_SendCmd("AT+MQTTSUB=0,\"agri/set/#\",1\r\n", "OK", 4000);
            } 
			else 
			{
                vTaskDelay(2000); // 重新开始连接
                continue;
            }
        }

        // 解析数据
        if (ESP8266_GetLine(rx_line, 256)) 
		{
            // 检查MQTT是否断开连接
            if (strstr(rx_line, "CLOSED") || strstr(rx_line, "DISCONNECT")) 
			{
                connected = 0;
                continue;
            }
            // 检查是否是该格式 +MQTTSUBRECV:0,"topic",length,payload
             if (strstr(rx_line, "+MQTTSUBRECV")) 
			{
                CmdMsg_t msg;
                msg.type = 255; // 默认值
               
                // 格式: +MQTTSUBRECV:<LinkID>,<"topic">,<data_len>,<data>
                char* p = strchr(rx_line, ':'); 
                if (p) 
				{
                    int commas = 0;
                    while (*p && commas < 3) 
					{
                        if (*p == ',') 
							commas++;
                        p++;
                    }
                    if (commas == 3) 
					{
                        while (*p && !isdigit((unsigned char)*p) && *p != '-' && *p != '.' && *p != 'O' && *p != 'A' && *p != 'M') 
						{
                            p++;
                        }
                        
                        if (strstr(rx_line, "agri/cmd/mode")) 
						{
                            msg.type = 0;
                            if (strstr(p, "AUTO")) 
								msg.value = (float)MODE_AUTO;
                            else 
								msg.value = (float)MODE_MANUAL;
                        }
                        else if (strstr(rx_line, "agri/cmd/pump")) 
						{
                            msg.type = 1; 
							msg.target = DEV_PUMP;
                            msg.value = strstr(p, "ON") ? 1.0f : 0.0f;
                        }
                        else if (strstr(rx_line, "agri/cmd/fan")) 
						{
                            msg.type = 1; 
							msg.target = DEV_FAN;
                            msg.value = strstr(p, "ON") ? 1.0f : 0.0f;
                        }
                        else if (strstr(rx_line, "agri/cmd/led")) 
						{
                            msg.type = 1; 
							msg.target = DEV_LAMP;
                            msg.value = strstr(p, "ON") ? 1.0f : 0.0f;
                        }
                        else if (strstr(rx_line, "agri/set/threshold/temp/min")) 
                        {
                            msg.type = 2; 
							msg.target = 0; 
							msg.limit_type = 0; 
							msg.value = atof(p);
                        }
                        else if (strstr(rx_line, "agri/set/threshold/temp/max")) 
                        {
                            msg.type = 2; 
							msg.target = 0; 
							msg.limit_type = 1; 
							msg.value = atof(p);
                        }
                        else if (strstr(rx_line, "agri/set/threshold/soil/min")) 
                        {
                            msg.type = 2; 
							msg.target = 1; 
							msg.limit_type = 0; 
							msg.value = atof(p);
                        }
                        else if (strstr(rx_line, "agri/set/threshold/soil/max")) 
                        {
                            msg.type = 2; 
							msg.target = 1; 
							msg.limit_type = 1; 
							msg.value = atof(p);
                        }
                        else if (strstr(rx_line, "agri/set/threshold/light/min")) 
                        {
                            msg.type = 2; 
							msg.target = 2; 
							msg.limit_type = 0; 
							msg.value = atof(p);
                        }
                        else if (strstr(rx_line, "agri/set/threshold/light/max")) 
                        {
                            msg.type = 2; 
							msg.target = 2; 
							msg.limit_type = 1; 
							msg.value = atof(p);
                        }
                    }
                }
                
                // 将解析出来的命令放入队列
                if (msg.type != 255) 
				{
                    xQueueSend(xQueueCmds, &msg, 0);
                }
            }
        }

		if(xTaskGetTickCount() - last_step_tick > 1000)
		{
			xSemaphoreTake(xMutexState, portMAX_DELAY);
            switch(pub_step) 
            {
                case 0: sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/data/temp\",\"%.1f\",0,0\r\n", sysState.temp); break;
                case 1: sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/data/humi\",\"%.1f\",0,0\r\n", sysState.humi); break;
                case 2: sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/data/soil\",\"%d\",0,0\r\n", sysState.soil); break;
                case 3: sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/data/light\",\"%d\",0,0\r\n", sysState.light); break;
            }
            xSemaphoreGive(xMutexState);
            
            ESP8266_SendCmd(tx_buf, "OK", 1000); 
            pub_step = (pub_step + 1) % 4;
            last_step_tick = xTaskGetTickCount();
		}

        if (sysState.report_mask != 0) 
		{
            xSemaphoreTake(xMutexState, portMAX_DELAY);
            uint32_t current_mask = sysState.report_mask;
            sysState.report_mask = 0; 
            
            if (current_mask & REPORT_MODE) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/mode\",\"%s\",0,0\r\n", 
                                (sysState.mode == MODE_AUTO) ? "AUTO" : "MANUAL");
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_PUMP) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/pump\",\"%d\",0,0\r\n", sysState.pump_status);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_FAN) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/fan\",\"%d\",0,0\r\n", sysState.fan_status);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_LED) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/led\",\"%d\",0,0\r\n", sysState.led_status);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_T_MIN) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/temp/min\",\"%.1f\",0,0\r\n", sysState.temp_min);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_T_MAX) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/temp/max\",\"%.1f\",0,0\r\n", sysState.temp_max);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_S_MIN) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/soil/min\",\"%d\",0,0\r\n", sysState.soil_min);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_S_MAX) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/soil/max\",\"%d\",0,0\r\n", sysState.soil_max);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_L_MIN) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/light/min\",\"%d\",0,0\r\n", sysState.light_min);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            if (current_mask & REPORT_L_MAX) 
			{
                sprintf(tx_buf, "AT+MQTTPUB=0,\"agri/status/threshold/light/max\",\"%d\",0,0\r\n", sysState.light_max);
                ESP8266_SendString(tx_buf); 
				vTaskDelay(50);
            }
            
            xSemaphoreGive(xMutexState);
        }
		vTaskDelay(20);
    }
}

// --- Sensor Task ---
void Sensor_Task(void *pvParameters) 
{
    uint8_t t, h;
    uint16_t s;
    
    while(1) 
	{
        if(Sensor_ReadDHT11(&t, &h)) 
		{
            xSemaphoreTake(xMutexState, portMAX_DELAY);
            sysState.temp = (float)t;
            sysState.humi = (float)h;
            xSemaphoreGive(xMutexState);
        }
        
        s = Sensor_ReadSoil();
        xSemaphoreTake(xMutexState, portMAX_DELAY);
        sysState.soil = s;
        xSemaphoreGive(xMutexState);
        
        s = Sensor_ReadLight();
        xSemaphoreTake(xMutexState, portMAX_DELAY);
        sysState.light = s;
        xSemaphoreGive(xMutexState);
        
        vTaskDelay(2000); 
    }
}

// --- Control Task ---
void Control_Task(void *pvParameters) 
{
    CmdMsg_t cmd;
    
    while(1) 
	{
        if (xQueueReceive(xQueueCmds, &cmd, 100)) 
		{
            xSemaphoreTake(xMutexState, portMAX_DELAY);
            if (cmd.type == 0) 
			{ 
                sysState.mode = (uint8_t)cmd.value;
                sysState.report_mask |= REPORT_MODE; 
            } 
            else if (cmd.type == 1) 
			{ 
                if (sysState.mode == MODE_MANUAL) 
				{
                     Control_SetDevice(cmd.target, (uint8_t)cmd.value);
                     if(cmd.target == DEV_PUMP) 
					 { 
						sysState.pump_status = (uint8_t)cmd.value; 
						sysState.report_mask |= REPORT_PUMP; 
						 
					 }
                     if(cmd.target == DEV_FAN) 
					 { 
						sysState.fan_status = (uint8_t)cmd.value; 
						sysState.report_mask |= REPORT_FAN; 
					 }
                     if(cmd.target == DEV_LAMP) 
					 { 
						sysState.led_status = (uint8_t)cmd.value; 
						sysState.report_mask |= REPORT_LED; 
					 }
                }
            }
            else if (cmd.type == 2) 
			{ 
                if (cmd.target == 0) 
				{ 
                    if (cmd.limit_type == 0) 
					{ 
						sysState.temp_min = cmd.value; 
					    sysState.report_mask |= REPORT_T_MIN; 
					}
                    else 
					{ 
					    sysState.temp_max = cmd.value; 
						sysState.report_mask |= REPORT_T_MAX; 
					}
                }
                if (cmd.target == 1) 
				{ 
                    if (cmd.limit_type == 0) 
					{ 
						sysState.soil_min = (int)cmd.value; 
						sysState.report_mask |= REPORT_S_MIN; 
					}
                    else 
					{ 
						sysState.soil_max = (int)cmd.value; 
						sysState.report_mask |= REPORT_S_MAX; 
					}
                }
                if (cmd.target == 2) 
				{ 
                    if (cmd.limit_type == 0) 
					{ 
						sysState.light_min = (int)cmd.value; 
						sysState.report_mask |= REPORT_L_MIN; 
					}
                    else 
					{ 
						sysState.light_max = (int)cmd.value; 
						sysState.report_mask |= REPORT_L_MAX; 
					}
                }
            }
            xSemaphoreGive(xMutexState);
        }
        
        xSemaphoreTake(xMutexState, portMAX_DELAY);
        if (sysState.mode == MODE_AUTO) 
		{
            if (sysState.temp > sysState.temp_max) 
			{
                if (!sysState.fan_status) 
				{
                    Control_SetDevice(DEV_FAN, 1);
                    sysState.fan_status = 1;
                    sysState.report_mask |= REPORT_FAN;
                }
            } 
			else if (sysState.temp < sysState.temp_min) 
			{
                if (sysState.fan_status) 
				{
                    Control_SetDevice(DEV_FAN, 0);
                    sysState.fan_status = 0;
                    sysState.report_mask |= REPORT_FAN;
                }
            }
            
            if (sysState.soil < sysState.soil_min) 
			{
                 if (!sysState.pump_status) 
				 {
                    Control_SetDevice(DEV_PUMP, 1);
                    sysState.pump_status = 1;
                    sysState.report_mask |= REPORT_PUMP;
                 }
            } 
			else if (sysState.soil > sysState.soil_max)
			{
                if (sysState.pump_status) 
				{
                    Control_SetDevice(DEV_PUMP, 0);
                    sysState.pump_status = 0;
                    sysState.report_mask |= REPORT_PUMP;
                }
            }
            
            if (sysState.light < sysState.light_min) 
			{
                 if (!sysState.led_status) 
				 {
                    Control_SetDevice(DEV_LAMP, 1);
                    sysState.led_status = 1;
                    sysState.report_mask |= REPORT_LED;
                 }
            } 
			else if (sysState.light > sysState.light_max)
			{
                if (sysState.led_status) 
				{
                    Control_SetDevice(DEV_LAMP, 0);
                    sysState.led_status = 0;
                    sysState.report_mask |= REPORT_LED;
                }
            }
        }
        xSemaphoreGive(xMutexState);
        
		vTaskDelay(50);
    }
}

