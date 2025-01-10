#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_app.h"
#include "hal_adc.h"
#include "bme280.h"
#include "ds18b20.h"
#include "Debug.h"

// Глобальные переменные для динамических атрибутов
zclAttrRec_t dynamicAttrList[20];
uint8 dynamicAttrListSize = 0;

// Флаги наличия сенсоров
bool hasDS18B20 = false;
bool hasBME280 = false;

// Глобальные переменные для батареи
uint8 zclBattery_Voltage = 0;                // Напряжение батареи (в десятых долях вольта)
uint8 zclBattery_PercentageRemaining = 0;   // Процент заряда батареи

// Глобальная переменная для времени идентификации
uint16 zclIdentifyTime = 0;

// Обнаружение подключенных сенсоров
void detectSensors(void) {
    // Проверяем наличие DS18B20
    if (ds18b20_isPresent() == SUCCESS) {
        hasDS18B20 = true;
        LREP("DS18B20 detected\r\n");
    } else {
        hasDS18B20 = false;
        LREP("DS18B20 not detected\r\n");
    }

    // Проверяем наличие BME280
    if (bme280_init() == BME280_OK) {
        hasBME280 = true;
        LREP("BME280 detected\r\n");
    } else {
        hasBME280 = false;
        LREP("BME280 not detected\r\n");
    }
}

// Конфигурация атрибутов для сенсоров
void configureSensorAttributes(void) {
    // Добавляем атрибуты температуры (DS18B20)
    if (hasDS18B20) {
        dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
            ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
            {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, R, (void *)&zclApp_DS18B20_MeasuredValue}
        };
    }

    // Добавляем атрибуты температуры, влажности и давления (BME280)
    if (hasBME280) {
        dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
            ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
            {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, R, (void *)&zclApp_Temperature_Sensor_MeasuredValue}
        };
        dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
            ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,
            {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, R, (void *)&zclApp_HumiditySensor_MeasuredValue}
        };
        dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
            ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT,
            {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_INT16, R, (void *)&zclApp_PressureSensor_MeasuredValue}
        };
    }

    LREP("Sensor attributes configured. Count: %d\r\n", dynamicAttrListSize);
}

// Конфигурация атрибутов батареи
void configureBatteryAttributes(void) {
    dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
        ZCL_CLUSTER_ID_GEN_POWER_CFG,
        {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, R, (void *)&zclBattery_Voltage}
    };
    dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
        ZCL_CLUSTER_ID_GEN_POWER_CFG,
        {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, R, (void *)&zclBattery_PercentageRemaining}
    };
    LREP("Battery attributes added to dynamicAttrList\r\n");
}

// Конфигурация атрибутов Identify
void configureIdentifyAttributes(void) {
    dynamicAttrList[dynamicAttrListSize++] = (zclAttrRec_t){
        ZCL_CLUSTER_ID_GEN_IDENTIFY,
        {ATTRID_IDENTIFY_TIME, ZCL_UINT16, R | W, (void *)&zclIdentifyTime}
    };
    LREP("Identify attributes added to dynamicAttrList\r\n");
}

// Регистрация репортинга для всех кластеров
void registerReporting(void) {
    // Репортинг для батареи
    zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, 30, 3600, 1);
    zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, 30, 3600, 1);

    // Репортинг для сенсоров
    if (hasDS18B20) {
        zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE, 30, 3600, 50);
    }
    if (hasBME280) {
        zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, ATTRID_MS_TEMPERATURE_MEASURED_VALUE, 30, 3600, 50);
        zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY, ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, 30, 3600, 200);
        zcl_registerReportableAttribute(zclApp_FirstEP.EndPoint, ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT, ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, 30, 3600, 50);
    }

    LREP("Reporting configured for all clusters\r\n");
}

// Основная конфигурация динамических атрибутов
void configureDynamicAttributes(void) {
    dynamicAttrListSize = 0;

    configureSensorAttributes();
    configureBatteryAttributes();
    configureIdentifyAttributes();

    LREP("Dynamic Attributes Configured. Total Count: %d\r\n", dynamicAttrListSize);
}
