#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "zcl.h"
#include "zcl_app.h"
#include "bdb.h"
#include "battery.h"
#include "commissioning.h"
#include "factory_reset.h"
#include "led_control.h"
#include "hal_adc.h"
#include "bme280.h"
#include "ds18b20.h"

byte zclApp_TaskID;

uint8 zclBattery_Voltage = 0;                // Напряжение батареи (0.1 Вольт)
uint8 zclBattery_PercentageRemaining = 0;   // Остаток заряда в %

// Функция для обновления значений кластеров
static void updateClusterValue(uint8 endpoint, uint16 clusterId, uint16 attrId, int16 value) {
    bdb_RepChangedAttrValue(endpoint, clusterId, attrId);
    LREP("Cluster 0x%X Attribute 0x%X Updated: %d\r\n", clusterId, attrId, value);
}

// Чтение напряжения батареи
uint16 readBatteryVoltage(void) {
    uint16 adcValue = halAdcRead(HAL_ADC_CHANNEL_VDD, HAL_ADC_RESOLUTION_14);
    return (adcValue * 1250) / 4096; // Конвертация в милливольты
}

// Вычисление процента заряда батареи
uint8 calculateBatteryPercentage(uint16 voltage) {
    const uint16 MIN_VOLTAGE = 2000; // 2.0 В (разряженная батарея)
    const uint16 MAX_VOLTAGE = 3000; // 3.0 В (полностью заряженная батарея)

    if (voltage <= MIN_VOLTAGE) {
        return 0; // Полностью разряжено
    } else if (voltage >= MAX_VOLTAGE) {
        return 100; // Полностью заряжено
    } else {
        // Пропорциональный расчет
        return (uint8)((voltage - MIN_VOLTAGE) * 100 / (MAX_VOLTAGE - MIN_VOLTAGE));
    }
}

// Обновление данных о батарее
static void updateBatteryAttributes(void) {
    uint16 voltage = readBatteryVoltage(); // Напряжение в мВ
    zclBattery_Voltage = voltage / 100;   // Конвертация в 0.1 Вольта
    zclBattery_PercentageRemaining = calculateBatteryPercentage(voltage);

    bdb_RepChangedAttrValue(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE);
    bdb_RepChangedAttrValue(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING);
}

// Регистрация репортинга батареи
static void registerBatteryReporting(void) {
    zcl_registerReportableAttribute(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, 30, 3600, 1);
    zcl_registerReportableAttribute(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, 30, 3600, 1);
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

    return 0;
}

// Обработка команды Identify
static void zclApp_IdentifyCB(zclIdentify_t *pCmd) {
    LREP("Identify: Time=%d\r\n", pCmd->identifyTime);

    for (uint16 i = 0; i < pCmd->identifyTime; i++) {
        LED_On();
        osal_start_timerEx(zclApp_TaskID, APP_IDENTIFY_EVT, 500); // Таймер для выключения LED
    }
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
