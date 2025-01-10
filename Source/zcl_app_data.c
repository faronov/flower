#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_app.h"

// Глобальные атрибуты для батареи
uint8 zclBattery_Voltage = 0;
uint8 zclBattery_PercentageRemaining = 0;

// Массив атрибутов
CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    // Атрибуты батареи
    {ZCL_CLUSTER_ID_GEN_POWER_CFG, {ATTRID_POWER_CFG_BATTERY_VOLTAGE, ZCL_UINT8, R, (void *)&zclBattery_Voltage}},
    {ZCL_CLUSTER_ID_GEN_POWER_CFG, {ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, ZCL_UINT8, R, (void *)&zclBattery_PercentageRemaining}},
    // Другие атрибуты (освещенность, температура и т.д.)
};

// Конфигурация кластеров
const cId_t zclApp_InClusterListFirstEP[] = {
    ZCL_CLUSTER_ID_GEN_BASIC,
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    ZCL_CLUSTER_ID_GEN_POWER_CFG, // Кластер батареи
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT,
    ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY,
    ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT,
    ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,
};

#define APP_MAX_INCLUSTERS_FIRST_EP (sizeof(zclApp_InClusterListFirstEP) / sizeof(zclApp_InClusterListFirstEP[0]))

SimpleDescriptionFormat_t zclApp_FirstEP = {
    1, // Endpoint
    ZCL_HA_PROFILE_ID, // Profile ID
    ZCL_HA_DEVICEID_SIMPLE_SENSOR, // Device ID
    1, // Device Version
    0, // Flags
    APP_MAX_INCLUSTERS_FIRST_EP, // Number of input clusters
    (cId_t *)zclApp_InClusterListFirstEP, // Input clusters
    0, // Number of output clusters
    (cId_t *)NULL // Output clusters
};

// Регистрация репортинга батареи
void registerBatteryReporting(void) {
    zcl_registerReportableAttribute(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_VOLTAGE, 30, 3600, 1);
    zcl_registerReportableAttribute(1, ZCL_CLUSTER_ID_GEN_POWER_CFG, ATTRID_POWER_CFG_BATTERY_PERCENTAGE_REMAINING, 30, 3600, 1);
}
