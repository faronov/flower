#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "zcl_app.h"

#include "battery.h"
#include "version.h"

// Глобальные атрибуты
const uint16 zclApp_clusterRevision_all = 0x0001;

int16 zclApp_Temperature_Sensor_MeasuredValue = 0;
int16 zclApp_PressureSensor_MeasuredValue = 0;
int16 zclApp_PressureSensor_ScaledValue = 0;
int8 zclApp_PressureSensor_Scale = -1;

uint16 zclApp_HumiditySensor_MeasuredValue = 0;
uint16 zclApp_SoilHumiditySensor_MeasuredValue = 0;
uint16 zclApp_SoilHumiditySensor_MeasuredValueRawAdc = 0;

int16 zclApp_DS18B20_MeasuredValue = 0;

uint16 zclApp_IlluminanceSensor_MeasuredValue = 0;
uint16 zclApp_IlluminanceSensor_MeasuredValueRawAdc = 0;

uint16 zclApp_IdentifyTime = 0;

// Basic Cluster
const uint8 zclApp_HWRevision = 1;
const uint8 zclApp_ZCLVersion = 1;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;
const uint8 zclApp_ManufacturerName[] = {9, 'm', 'o', 'd', 'k', 'a', 'm', '.', 'r', 'u'};
const uint8 zclApp_ModelId[] = {13, 'D', 'I', 'Y', 'R', 'u', 'Z', '_', 'F', 'l', 'o', 'w', 'e', 'r'};
const uint8 zclApp_PowerSource = POWER_SOURCE_BATTERY;

// Атрибуты кластеров
CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPLICATION_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_DATATYPE_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    {ILLUMINANCE, {ATTRID_MS_ILLUMINANCE_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_IlluminanceSensor_MeasuredValue}},
    {TEMP, {ATTRID_MS_TEMPERATURE_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_Temperature_Sensor_MeasuredValue}},
    {PRESSURE, {ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE, ZCL_INT16, RR, (void *)&zclApp_PressureSensor_MeasuredValue}},
    {HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_HumiditySensor_MeasuredValue}},
    {SOIL_HUMIDITY, {ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE, ZCL_UINT16, RR, (void *)&zclApp_SoilHumiditySensor_MeasuredValue}},
    {ZCL_CLUSTER_ID_GEN_IDENTIFY, {ATTRID_IDENTIFY_TIME, ZCL_UINT16, RR, (void *)&zclApp_IdentifyTime}}, // Identify
};

// Конфигурация кластеров
const cId_t zclApp_InClusterListFirstEP[] = {
    ZCL_CLUSTER_ID_GEN_BASIC,
    ZCL_CLUSTER_ID_GEN_IDENTIFY, // Identify Cluster
    ILLUMINANCE,
    TEMP,
    PRESSURE,
    HUMIDITY,
    SOIL_HUMIDITY,
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
