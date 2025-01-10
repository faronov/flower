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

// Глобальные переменные для хранения данных Bind/Unbind
uint16 dynamicDstAddr;
uint8 dynamicDstEndpoint;
uint16 dynamicClusterId;

// Функция для обновления значений кластеров
static void updateClusterValue(uint8 endpoint, uint16 clusterId, uint16 attrId, int16 value) {
    bdb_RepChangedAttrValue(endpoint, clusterId, attrId);
    LREP("Cluster 0x%X Attribute 0x%X Updated: %d\r\n", clusterId, attrId, value);
}

// Функция для создания Bind через Zigbee API
void zclApp_ZigbeeBind(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId) {
    ZDO_BindReq_t bindReq;
    osal_memset(&bindReq, 0, sizeof(bindReq));

    // Настройка источника
    bindReq.srcAddr = NLME_GetShortAddr();  // Короткий адрес источника
    osal_cpyExtAddr(bindReq.srcExtAddr, NLME_GetExtAddr());  // IEEE адрес источника
    bindReq.srcEP = zclApp_FirstEP.EndPoint;  // Endpoint источника
    bindReq.clusterId = clusterId;

    // Настройка назначения
    bindReq.dstAddr.addrMode = Addr16Bit;  // Используем 16-битный адрес
    bindReq.dstAddr.addr.shortAddr = dstAddr;  // Адрес назначения
    bindReq.dstEP = dstEndpoint;  // Endpoint назначения

    // Отправка команды Bind
    if (ZDO_BindReq(&bindReq) == ZSuccess) {
        LREP("Zigbee Bind successful: Cluster 0x%X to device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
    } else {
        LREP("Zigbee Bind failed: Cluster 0x%X to device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
    }
}

// Функция для удаления Bind через Zigbee API
void zclApp_ZigbeeUnbind(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId) {
    ZDO_BindReq_t unbindReq;
    osal_memset(&unbindReq, 0, sizeof(unbindReq));

    // Настройка источника
    unbindReq.srcAddr = NLME_GetShortAddr();  // Короткий адрес источника
    osal_cpyExtAddr(unbindReq.srcExtAddr, NLME_GetExtAddr());  // IEEE адрес источника
    unbindReq.srcEP = zclApp_FirstEP.EndPoint;  // Endpoint источника
    unbindReq.clusterId = clusterId;

    // Настройка назначения
    unbindReq.dstAddr.addrMode = Addr16Bit;  // Используем 16-битный адрес
    unbindReq.dstAddr.addr.shortAddr = dstAddr;  // Адрес назначения
    unbindReq.dstEP = dstEndpoint;  // Endpoint назначения

    // Отправка команды Unbind
    if (ZDO_UnbindReq(&unbindReq) == ZSuccess) {
        LREP("Zigbee Unbind successful: Cluster 0x%X from device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
    } else {
        LREP("Zigbee Unbind failed: Cluster 0x%X from device 0x%X, endpoint %d\r\n", clusterId, dstAddr, dstEndpoint);
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

    if (events & APP_BIND_EVT) {
        zclApp_ZigbeeBind(dynamicDstAddr, dynamicDstEndpoint, dynamicClusterId);
        return (events ^ APP_BIND_EVT);
    }

    if (events & APP_UNBIND_EVT) {
        zclApp_ZigbeeUnbind(dynamicDstAddr, dynamicDstEndpoint, dynamicClusterId);
        return (events ^ APP_UNBIND_EVT);
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

// Установка динамических параметров для Bind/Unbind
void setDynamicBindParameters(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId) {
    dynamicDstAddr = dstAddr;
    dynamicDstEndpoint = dstEndpoint;
    dynamicClusterId = clusterId;
}
