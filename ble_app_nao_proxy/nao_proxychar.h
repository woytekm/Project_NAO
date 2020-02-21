/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the license.txt file.
 */

#ifndef BLE_LBS_H__
#define BLE_LBS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#define NAO_PROXY_UUID_BASE {0x23, 0xD1, 0xBC, 0xEA, 0x66, 0x66, 0x66, 0x66, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}
#define NAO_PROXY_UUID_SERVICE 0x1523
#define NAO_PROXY_UUID_NAO_WRITE_CHAR 0x1525
#define NAO_PROXY_UUID_NAO_NOTIF_CHAR 0x1524

#define NAO_PACKET_SIZE 20

// Forward declaration of the nao_proxy_t type. 
typedef struct nao_proxy_s nao_proxy_t;

typedef void (*nao_proxy_nao_write_handler_t) (nao_proxy_t * p_nao_proxy, uint8_t *nao_write_data);

typedef struct
{
    nao_proxy_nao_write_handler_t nao_write_handler;                    /**< Event handler to be called when LED characteristic is written. */
} nao_proxy_init_t;

/**@brief LED Button Service structure. This contains various status information for the service. */
typedef struct nao_proxy_s
{
    uint16_t                    service_handle;
    ble_gatts_char_handles_t    nao_write_char_handles;
    ble_gatts_char_handles_t    nao_notif_char_handles;
    uint8_t                     uuid_type;
    uint16_t                    conn_handle;
    nao_proxy_nao_write_handler_t nao_write_handler;
} nao_proxy_t;

/**@brief Function for initializing the LED Button Service.
 *
 * @param[out]  p_lbs       LED Button Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_lbs_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t nao_proxy_init(nao_proxy_t * p_nao_proxy, const nao_proxy_init_t * p_nao_proxy_init);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the LED Button Service.
 *
 *
 * @param[in]   p_lbs      LED Button Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void nao_proxy_on_ble_evt(nao_proxy_t * p_nao_proxy, ble_evt_t * p_ble_evt);

/**@brief Function for sending a button state notification.
 */
uint32_t nao_proxy_on_nao_notif(nao_proxy_t * p_nao_proxy, uint8_t *nao_notif_data);

#endif // BLE_LBS_H__

/** @} */

