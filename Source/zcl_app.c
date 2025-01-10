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

// Функция для обработки Bind/Unbind через Zigbee API
static void handleBindUnbind(uint16 dstAddr, uint8 dstEndpoint, uint16 clusterId, bool isBind) {
    ZDO_BindReq_t req;
    osal_memset(&req, 0, sizeof(req));

    // Настройка источника
    req.srcAddr = NLME_GetShortAddr();  // Короткий адрес источника
    osal_cpyExtAddr(req.srcExtAddr, NLME_GetExtAddr());  // IEEE адрес источника
    req.srcEP = zclApp_FirstEP.EndPoint;  // Endpoint источника
    req.clusterId = clusterId;

    // Настройка назначения
    req.dstAddr.addrMode = Addr16Bit;  // Используем 16-битный адрес
    req.dstAddr.addr.shortAddr = dstAddr;  // Адрес назначения
    req.dstEP = dstEndpoint;  // Endpoint назначения

    // Вызов Bind или Unbind
    ZStatus_t status = isBind ? ZDO_BindReq(&req) : ZDO_UnbindReq(&req);

    if (status == ZSuccess) {
        LREP("%s successful: Cluster 0x%X to device 0x%X, endpoint %d\r\n",
             isBind ? "Bind" : "Unbind", clusterId, dstAddr, dstEndpoint);
    } else {
        LREP("%s failed: Cluster 0x%X to device 0x%X, endpoint %d\r\n",
             isBind ? "Bind" : "Unbind", clusterId, dstAddr, dstEndpoint);
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
        handleBindUnbind(dynamicDstAddr, dynamicDstEndpoint, dynamicClusterId, true);
        return (events ^ APP_BIND_EVT);
    }

    if (events & APP_UNBIND_EVT) {
        handleBindUnbind(dynamicDstAddr, dynamicDstEndpoint, dynamicClusterId, false);
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
    LREP("Dynamic Bind Parameters Set: Addr=0x%X, EP=%d, Cluster=0x%X\r\n", dstAddr, dstEndpoint, clusterId);
}
