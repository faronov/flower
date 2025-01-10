#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "zcl.h"
#include "zcl_app.h"
#include "bdb.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "led_control.h"
#include "hal_adc.h"
#include "bme280.h"
#include "ds18b20.h"

byte zclApp_TaskID;

// Глобальные переменные для хранения данных Bind
uint16 manualBindDstAddr;
uint8 manualBindDstEndpoint;
uint16 manualBindClusterId;

// Функция для обновления значений кластеров
static void updateClusterValue(uint8 endpoint, uint16 clusterId, uint16 attrId, int16 value) {
    bdb_RepChangedAttrValue(endpoint, clusterId, attrId);
    LREP("Cluster 0x%X Attribute 0x%X Updated: %d\r\n", clusterId, attrId, value);
}

// Функция для создания Bind с другим устройством
static void zclApp_ManualBind(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId) {
    bindReq_t bindReq;
    osal_memset(&bindReq, 0, sizeof(bindReq));

    // Настройка источника Bind
    bindReq.srcAddr = NLME_GetShortAddr();  // Короткий адрес источника
    osal_cpyExtAddr(bindReq.srcIEEE, NLME_GetExtAddr());  // IEEE адрес источника
    bindReq.srcEP = zclApp_FirstEP.EndPoint;  // Endpoint источника
    bindReq.clusterId = clusterId;

    // Настройка назначения Bind
    bindReq.dstAddr.addrMode = Addr16Bit;  // Используем 16-битный адрес
    bindReq.dstAddr.addr.shortAddr = dstAddr;  // Адрес назначения
    bindReq.dstEP = dstEndpoint;  // Endpoint назначения

    // Добавление Bind через Zigbee API
    if (bdb_AddBindEntry(&bindReq) == SUCCESS) {
        LREP("Bind successful: Cluster 0x%X bound to device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
    } else {
        LREP("Bind failed: Cluster 0x%X to device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
    }
}

// Обработка ручного Bind через событие
static void zclApp_HandleManualBindRequest(void) {
    zclApp_ManualBind(manualBindDstAddr, manualBindDstEndpoint, manualBindClusterId);
}

// Чтение данных с сенсоров
static void zclApp_ReadSensors(void) {
    zclApp_ReadDS18B20();
    zclApp_ReadBME280();
    zclApp_ReadIlluminance();
    zclApp_ReadSoilHumidity();

    // Обновление данных батареи
    updateBatteryAttributes();
}

// Чтение температуры с DS18B20
static void zclApp_ReadDS18B20(void) {
    int16 temp = readTemperature();
    if (temp != -1) {
        zclApp_DS18B20_MeasuredValue = temp;
        updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE, temp);
    } else {
        LREP("DS18B20 Read Error\r\n");
    }
}

// Чтение данных с BME280
static void zclApp_ReadBME280(void) {
    struct bme280_data data;
    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &data, &bme280_dev);
    if (rslt == BME280_OK) {
        zclApp_Temperature_Sensor_MeasuredValue = data.temperature;
        zclApp_HumiditySensor_MeasuredValue = data.humidity;
        zclApp_PressureSensor_MeasuredValue = data.pressure;
        updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE, data.temperature);
        updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, data.humidity);
        updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, data.pressure);
    } else {
        LREP("BME280 Read Error\r\n");
    }
}

// Чтение освещенности
static void zclApp_ReadIlluminance(void) {
    uint16 adcValue = halAdcRead(HAL_ADC_CHANNEL_6, HAL_ADC_RESOLUTION_12);
    zclApp_IlluminanceSensor_MeasuredValue = (uint16)(adcValue * 0.2);
    updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT, ATTRID_MS_ILLUMINANCE_MEASURED_VALUE, zclApp_IlluminanceSensor_MeasuredValue);
}

// Чтение влажности почвы
static void zclApp_ReadSoilHumidity(void) {
    uint16 adcValue = halAdcRead(HAL_ADC_CHANNEL_5, HAL_ADC_RESOLUTION_14);
    uint16 minAir = 300;
    uint16 maxWater = 700;
    zclApp_SoilHumiditySensor_MeasuredValue = (adcValue - minAir) * 10000 / (maxWater - minAir);
    updateClusterValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, zclApp_SoilHumiditySensor_MeasuredValue);
}

// Генерация отчётов
static void zclApp_Report(void) {
    bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE);
}

// Обработка команды Identify
static void zclApp_IdentifyCB(zclIdentify_t *pCmd) {
    LREP("Identify: Time=%d\r\n", pCmd->identifyTime);

    for (uint16 i = 0; i < pCmd->identifyTime; i++) {
        LED_On();
        osal_start_timerEx(zclApp_TaskID, APP_IDENTIFY_EVT, 500); // Таймер для выключения LED
    }
}

// Основной цикл обработки событий
uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    if (events & APP_REPORT_EVT) {
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_READ_SENSORS_EVT) {
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }

    if (events & APP_IDENTIFY_EVT) {
        LED_Off();
        return (events ^ APP_IDENTIFY_EVT);
    }

    if (events & APP_MANUAL_BIND_EVT) {
        zclApp_HandleManualBindRequest();
        return (events ^ APP_MANUAL_BIND_EVT);
    }

    if (events & BDB_BIND_NOTIFICATION_EVT) {
        // Обработайте успешный Bind
        LREP("Bind notification received\r\n");
        return (events ^ BDB_BIND_NOTIFICATION_EVT);
    }

    return 0;
}

// Инициализация задачи
void zclApp_Init(byte task_id) {
    zclApp_TaskID = task_id;

    // Регистрация атрибутов и кластеров
    zcl_registerAttrList(zclApp_FirstEP.EndPoint,
                         sizeof(zclApp_AttrsFirstEP) / sizeof(zclAttrRec_t),
                         zclApp_AttrsFirstEP);
    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);

    registerBatteryReporting();

    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
}

// Функция для триггера Bind (инициируется пользователем)
void triggerManualBind(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId) {
    manualBindDstAddr = dstAddr;
    manualBindDstEndpoint = dstEndpoint;
    manualBindClusterId = clusterId;

    // Устанавливаем событие для обработки Bind
    osal_set_event(zclApp_TaskID, APP_MANUAL_BIND_EVT);
}
