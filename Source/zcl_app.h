#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************
 * INCLUDES
 **************************************************************/
#include "version.h"
#include "zcl.h"
#include "bdb.h"
#include "bdb_interface.h"
#include "ZDObject.h"
/**************************************************************
 * CONSTANTS
 **************************************************************/
// Application Events
#define APP_REPORT_EVT                  0x0001
#define APP_READ_SENSORS_EVT            0x0002
#define APP_IDENTIFY_EVT  0x0001
#define APP_BIND_EVT      0x0002
#define APP_UNBIND_EVT    0x0004

#define AIR_COMPENSATION_FORMULA(ADC)   ((0.179 * (double)ADC + 3926.0))
#define WATER_COMPENSATION_FORMULA(ADC) ((0.146 * (double)ADC + 2020.0))

#define APP_REPORT_DELAY ((uint32) 1800000) // 30 minutes

/**************************************************************
 * MACROS
 **************************************************************/
#define R           ACCESS_CONTROL_READ
#define RR          (R | ACCESS_REPORTABLE)

#define BASIC       ZCL_CLUSTER_ID_GEN_BASIC
#define POWER_CFG   ZCL_CLUSTER_ID_GEN_POWER_CFG
#define TEMP        ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY    ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
#define SOIL_HUMIDITY                  0x0408
#define PRESSURE    ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT
#define ILLUMINANCE ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT

#define ZCL_UINT8   ZCL_DATATYPE_UINT8
#define ZCL_UINT16  ZCL_DATATYPE_UINT16
#define ZCL_UINT32  ZCL_DATATYPE_UINT32
#define ZCL_INT16   ZCL_DATATYPE_INT16
#define ZCL_INT8    ZCL_DATATYPE_INT8
#define W ACCESS_CONTROL_WRITE

#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_RAW_ADC              0x0200
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE_BATTERY_RAW_ADC      0x0201

#define ATTRID_MS_TEMPERATURE_MEASURED_VALUE           0x0000
#define ATTRID_MS_RELATIVE_HUMIDITY_MEASURED_VALUE     0x0000
#define ATTRID_MS_PRESSURE_MEASUREMENT_MEASURED_VALUE  0x0000

/**************************************************************
 * VARIABLES
 **************************************************************/
extern SimpleDescriptionFormat_t zclApp_FirstEP;
extern SimpleDescriptionFormat_t zclApp_SecondEP;

extern uint8 zclApp_BatteryVoltage;
extern uint8 zclApp_BatteryPercentageRemainig;
extern uint16 zclApp_BatteryVoltageRawAdc;
extern int16 zclApp_Temperature_Sensor_MeasuredValue;
extern int16 zclApp_PressureSensor_MeasuredValue;
extern int16 zclApp_PressureSensor_ScaledValue;
extern int8 zclApp_PressureSensor_Scale;
extern uint16 zclApp_HumiditySensor_MeasuredValue;
extern int16 zclApp_DS18B20_MeasuredValue;
extern uint16 zclApp_SoilHumiditySensor_MeasuredValue;
extern uint16 zclApp_SoilHumiditySensor_MeasuredValueRawAdc;
extern uint16 zclApp_IlluminanceSensor_MeasuredValue;
extern uint16 zclApp_IlluminanceSensor_MeasuredValueRawAdc;

// attribute list
extern CONST zclAttrRec_t zclApp_AttrsFirstEP[];
extern CONST zclAttrRec_t zclApp_AttrsSecondEP[];
extern CONST uint8 zclApp_AttrsSecondEPCount;
extern CONST uint8 zclApp_AttrsFirstEPCount;

extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;

extern bool hasDS18B20;
extern bool hasBME280;

/**************************************************************
 * FUNCTIONS
 **************************************************************/
extern void zclApp_Report(void);
extern void zclApp_ReadSensors(void);
extern void detectSensors(void);
extern void configureSensorAttributes(void);
extern void configureBatteryAttributes(void);
extern void configureIdentifyAttributes(void);
extern void registerReporting(void);
extern void configureDynamicAttributes(void);
extern int DS18B20_Init(uint8_t pin); // Пример прототипа для DS18B20
extern void zcl_registerReportableAttribute(uint8 endpoint, uint16 clusterId, uint16 attrId, uint16 minReportInt, uint16 maxReportInt, uint8 reportableChange);
extern void registerBatteryReporting(void);
extern void setDynamicBindParameters(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId);

void zclApp_InitClusters(void);
void zclApp_BindClusters(void);

/*
 * Initialization for the task
 */
extern void zclApp_Init(byte task_id);

/*
 *  Event Process for the task
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

void user_delay_ms(uint32_t period);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
