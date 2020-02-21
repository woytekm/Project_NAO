/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the license.txt file.
 */

#include "nao_proxychar.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_nao_proxy       LED Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(nao_proxy_t * p_nao_proxy, ble_evt_t * p_ble_evt)
{
    p_nao_proxy->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_nao_proxy       LED Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(nao_proxy_t * p_nao_proxy, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_nao_proxy->conn_handle = BLE_CONN_HANDLE_INVALID;
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_nao_proxy       LED Button Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(nao_proxy_t * p_nao_proxy, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    
    if ((p_evt_write->handle == p_nao_proxy->nao_write_char_handles.value_handle) &&
        (p_evt_write->len == 1) && // zmienic na długość pakietu pisanego do NAO
        (p_nao_proxy->nao_write_handler != NULL))
    {
        p_nao_proxy->nao_write_handler(p_nao_proxy, p_evt_write->data);
    }
}


void nao_proxy_on_ble_evt(nao_proxy_t * p_nao_proxy, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_nao_proxy, p_ble_evt);
            break;
            
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_nao_proxy, p_ble_evt);
            break;
            
        case BLE_GATTS_EVT_WRITE:
            on_write(p_nao_proxy, p_ble_evt);
            break;
            
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for adding the LED characteristic.
 *
 */
static uint32_t nao_write_char_add(nao_proxy_t * p_nao_proxy, const nao_proxy_init_t * p_nao_proxy_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type = p_nao_proxy->uuid_type;
    ble_uuid.uuid = NAO_PROXY_UUID_NAO_WRITE_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    
    return sd_ble_gatts_characteristic_add(p_nao_proxy->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_nao_proxy->nao_write_char_handles);
}

/**@brief Function for adding the Button characteristic.
 *
 */
static uint32_t nao_notif_char_add(nao_proxy_t * p_nao_proxy, const nao_proxy_init_t * p_nao_proxy_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;
    
    ble_uuid.type = p_nao_proxy->uuid_type;
    ble_uuid.uuid = NAO_PROXY_UUID_NAO_NOTIF_CHAR;
    
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    
    return sd_ble_gatts_characteristic_add(p_nao_proxy->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_nao_proxy->nao_notif_char_handles);
}

uint32_t nao_proxy_init(nao_proxy_t * p_nao_proxy, const nao_proxy_init_t * p_nao_proxy_init)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_nao_proxy->conn_handle       = BLE_CONN_HANDLE_INVALID;
    p_nao_proxy->nao_write_handler = p_nao_proxy_init->nao_write_handler;
    
    // Add service
    ble_uuid128_t base_uuid = {NAO_PROXY_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_nao_proxy->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_nao_proxy->uuid_type;
    ble_uuid.uuid = NAO_PROXY_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_nao_proxy->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    err_code = nao_notif_char_add(p_nao_proxy, p_nao_proxy_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    err_code = nao_write_char_add(p_nao_proxy, p_nao_proxy_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;
}

uint32_t nao_proxy_on_nao_notif(nao_proxy_t * p_nao_proxy, uint8_t *nao_notif_data)
{
    ble_gatts_hvx_params_t params;
    uint16_t len = NAO_PACKET_SIZE; // zmienic na długość pakietu notyfikacyjnego NAO
    
    memset(&params, 0, sizeof(params));
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_nao_proxy->nao_notif_char_handles.value_handle;
    params.p_data = nao_notif_data;
    params.p_len = &len;
    
    return sd_ble_gatts_hvx(p_nao_proxy->conn_handle, &params);
}


