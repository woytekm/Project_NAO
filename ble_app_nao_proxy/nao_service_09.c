#include <stdlib.h> // definition of NULL

#include "ble.h"
#include "ble_gattc.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "ble_db_discovery.h"
#include "ble_gatt.h"
#include "nao_service_09.h"
#include "nao_generic.h"
#include "nrf_log.h"


void ble_nao_conf_c_on_db_disc_evt(ble_nao_conf_c_t * p_ble_nao_conf_c, ble_db_discovery_evt_t * p_evt)
{
    ble_nao_conf_c_evt_t nao_conf_c_evt;
    memset(&nao_conf_c_evt,0,sizeof(ble_nao_conf_c_evt_t));

    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the NAO was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == NAO_CONF_SRV_UUID_UINT16 &&
        p_evt->params.discovered_db.srv_uuid.type == p_ble_nao_conf_c->uuid_type)
    {

        uint32_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_NAO_CONF_TX_CHARACTERISTIC:
                    nao_conf_c_evt.handles.nao_conf_tx_handle = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_NAO_CONF_RX_CHARACTERISTIC:
                    nao_conf_c_evt.handles.nao_conf_rx_handle = p_chars[i].characteristic.handle_value;
                    nao_conf_c_evt.handles.nao_conf_rx_cccd_handle = p_chars[i].cccd_handle;
                    break;

                default:
                    break;
            }
        }
        if (p_ble_nao_conf_c->evt_handler != NULL) 
        {
            nao_conf_c_evt.conn_handle = p_evt->conn_handle;
            nao_conf_c_evt.evt_type    = BLE_NAO_CONF_C_EVT_DISCOVERY_COMPLETE;
            p_ble_nao_conf_c->evt_handler(p_ble_nao_conf_c, &nao_conf_c_evt);
        }
    }
}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function will uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the NAO_CONF RX characteristic from the peer. If
 *            it is, this function will decode the data and send it to the
 *            application.
 *
 * @param[in] p_ble_nao_conf_c Pointer to the NAO_CONF Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void ble_nao_conf_on_hvx(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_evt_t * p_ble_evt)
{
    // HVX can only occur from client sending.

    if ( (p_ble_nao_conf_c->handles.nao_conf_rx_handle != BLE_GATT_HANDLE_INVALID)
            && (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_nao_conf_c->handles.nao_conf_rx_handle)
            && (p_ble_nao_conf_c->evt_handler != NULL)
        )
    {
        ble_nao_conf_c_evt_t ble_nao_conf_c_evt;

        ble_nao_conf_c_evt.evt_type = BLE_NAO_CONF_C_EVT_NAO_CONF_RX_EVT;
        ble_nao_conf_c_evt.p_data   = (uint8_t *)p_ble_evt->evt.gattc_evt.params.hvx.data;
        ble_nao_conf_c_evt.data_len = p_ble_evt->evt.gattc_evt.params.hvx.len;

        p_ble_nao_conf_c->evt_handler(p_ble_nao_conf_c, &ble_nao_conf_c_evt);
    }
}

uint32_t ble_nao_conf_c_init(ble_nao_conf_c_t * p_ble_nao_conf_c, ble_nao_conf_c_init_t * p_ble_nao_conf_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    uart_uuid;
    ble_uuid128_t nao_conf_base_uuid = NAO_CONF_SRV_UUID_UINT128;
        
    VERIFY_PARAM_NOT_NULL(p_ble_nao_conf_c);
    VERIFY_PARAM_NOT_NULL(p_ble_nao_conf_c_init);
    
    err_code = sd_ble_uuid_vs_add(&nao_conf_base_uuid, &p_ble_nao_conf_c->uuid_type);
    VERIFY_SUCCESS(err_code);
    
    uart_uuid.type = p_ble_nao_conf_c->uuid_type;
    uart_uuid.uuid = NAO_CONF_SRV_UUID_UINT16;
    
    p_ble_nao_conf_c->conn_handle           = BLE_CONN_HANDLE_INVALID;
    p_ble_nao_conf_c->evt_handler           = p_ble_nao_conf_c_init->evt_handler;
    p_ble_nao_conf_c->handles.nao_conf_rx_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_nao_conf_c->handles.nao_conf_tx_handle = BLE_GATT_HANDLE_INVALID;
    
    return ble_db_discovery_evt_register(&uart_uuid);
}


static void ble_nao_conf_on_write_rsp(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_ble_nao_conf_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }

    if ((p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_AUTHENTICATION) ||
        (p_ble_evt->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_INSUF_ENCRYPTION))
    {
        // Do nothing to reattempt write.
    }
    else
    {
        if(p_ble_nao_conf_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
         {
          NRF_LOG_INFO("Received write response to write request (0x09)\r\n");
          notif_09_subbed = true;
         }
    }

    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


void ble_nao_conf_c_on_ble_evt(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_evt_t * p_ble_evt)
{
    if ((p_ble_nao_conf_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_nao_conf_c->conn_handle != BLE_CONN_HANDLE_INVALID) 
       &&(p_ble_nao_conf_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            ble_nao_conf_on_hvx(p_ble_nao_conf_c, p_ble_evt);
            break;
                
        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == p_ble_nao_conf_c->conn_handle
                    && p_ble_nao_conf_c->evt_handler != NULL)
            {
                ble_nao_conf_c_evt_t nao_conf_c_evt;
                
                nao_conf_c_evt.evt_type = BLE_NAO_CONF_C_EVT_DISCONNECTED;
                
                p_ble_nao_conf_c->conn_handle = BLE_CONN_HANDLE_INVALID;
                p_ble_nao_conf_c->evt_handler(p_ble_nao_conf_c, &nao_conf_c_evt);
            }
            break;

         case BLE_GATTC_EVT_WRITE_RSP:
            //ble_nao_conf_on_write_rsp(p_ble_nao_conf_c, p_ble_evt);
            break;

    }
}

uint32_t ble_nao_conf_c_rx_notif_enable(ble_nao_conf_c_t * p_ble_nao_conf_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_conf_c);

    if ( (p_ble_nao_conf_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_nao_conf_c->handles.nao_conf_rx_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_ble_nao_conf_c->conn_handle,p_ble_nao_conf_c->handles.nao_conf_rx_cccd_handle, true);
}


uint32_t ble_nao_conf_c_string_send(ble_nao_conf_c_t * p_ble_nao_conf_c, uint8_t * p_string, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_conf_c);
    
    if (length > BLE_NAO_CONF_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    if ( p_ble_nao_conf_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    const ble_gattc_write_params_t write_params = {
        .write_op = BLE_GATT_OP_WRITE_CMD,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_nao_conf_c->handles.nao_conf_tx_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = p_string
    };

    return sd_ble_gattc_write(p_ble_nao_conf_c->conn_handle, &write_params);
}


uint32_t ble_nao_conf_c_handles_assign(ble_nao_conf_c_t * p_ble_nao_conf,
                                  const uint16_t conn_handle,
                                  const ble_nao_conf_c_handles_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_conf);

    p_ble_nao_conf->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_nao_conf->handles.nao_conf_rx_cccd_handle = p_peer_handles->nao_conf_rx_cccd_handle;
        p_ble_nao_conf->handles.nao_conf_rx_handle      = p_peer_handles->nao_conf_rx_handle;
        p_ble_nao_conf->handles.nao_conf_tx_handle      = p_peer_handles->nao_conf_tx_handle;    
    }
    return NRF_SUCCESS;
}


