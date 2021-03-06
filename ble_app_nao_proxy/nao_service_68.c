#include <stdlib.h> // definition of NULL

#include "ble.h"
#include "ble_gattc.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "ble_db_discovery.h"
#include "ble_gatt.h"
#include "nao_service_68.h"
#include "nao_generic.h"
#include "nao_proxychar.h"
#include "nrf_log.h"



uint32_t ble_nao_stat_notif_forward(nao_proxy_t * p_proxy, uint8_t *data, uint16_t data_len)
{
    uint32_t err_code;
    uint16_t len;

    // Send value if connected and notifying
    if (p_proxy->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t hvx_params;

        len = data_len;

        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_proxy->nao_notif_char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &data_len;
        hvx_params.p_data = data;

        NRF_LOG_INFO("sending notification to peripheral: data len:%d",*hvx_params.p_len);

        err_code = sd_ble_gatts_hvx(p_proxy->conn_handle, &hvx_params);

        if ((err_code == NRF_SUCCESS) && (*hvx_params.p_len != len))
        {
            NRF_LOG_INFO("wrote %d bytes",*hvx_params.p_len);
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }

    return err_code;
}




void ble_nao_stat_c_on_db_disc_evt(ble_nao_stat_c_t * p_ble_nao_stat_c, ble_db_discovery_evt_t * p_evt)
{
    ble_nao_stat_c_evt_t nao_stat_c_evt;
    memset(&nao_stat_c_evt,0,sizeof(ble_nao_stat_c_evt_t));

    ble_gatt_db_char_t * p_chars = p_evt->params.discovered_db.charateristics;

    // Check if the NAO was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == NAO_STAT_SRV_UUID_UINT16 &&
        p_evt->params.discovered_db.srv_uuid.type == p_ble_nao_stat_c->uuid_type)
    {

        uint32_t i;

        for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            switch (p_chars[i].characteristic.uuid.uuid)
            {
                case BLE_UUID_NAO_STAT_TX_CHARACTERISTIC:
                    nao_stat_c_evt.handles.nao_stat_tx_handle = p_chars[i].characteristic.handle_value;
                    break;

                case BLE_UUID_NAO_STAT_RX_CHARACTERISTIC:
                    nao_stat_c_evt.handles.nao_stat_rx_handle = p_chars[i].characteristic.handle_value;
                    nao_stat_c_evt.handles.nao_stat_rx_cccd_handle = p_chars[i].cccd_handle;
                    break;

                default:
                    break;
            }
        }
        if (p_ble_nao_stat_c->evt_handler != NULL) 
        {
            nao_stat_c_evt.conn_handle = p_evt->conn_handle;
            nao_stat_c_evt.evt_type    = BLE_NAO_STAT_C_EVT_DISCOVERY_COMPLETE;
            p_ble_nao_stat_c->evt_handler(p_ble_nao_stat_c, &nao_stat_c_evt);
        }
    }
}

/**@brief     Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details   This function will uses the Handle Value Notification received from the SoftDevice
 *            and checks if it is a notification of the NAO_STAT RX characteristic from the peer. If
 *            it is, this function will decode the data and send it to the
 *            application.
 *
 * @param[in] p_ble_nao_stat_c Pointer to the NAO_STAT Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void ble_nao_stat_on_hvx(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_evt_t * p_ble_evt)
{
    // HVX can only occur from client sending.

    if ( (p_ble_nao_stat_c->handles.nao_stat_rx_handle != BLE_GATT_HANDLE_INVALID)
            && (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_nao_stat_c->handles.nao_stat_rx_handle)
            && (p_ble_nao_stat_c->evt_handler != NULL)
        )
    {
        ble_nao_stat_c_evt_t ble_nao_stat_c_evt;

        ble_nao_stat_c_evt.evt_type = BLE_NAO_STAT_C_EVT_NAO_STAT_RX_EVT;
        ble_nao_stat_c_evt.p_data   = (uint8_t *)p_ble_evt->evt.gattc_evt.params.hvx.data;
        ble_nao_stat_c_evt.data_len = p_ble_evt->evt.gattc_evt.params.hvx.len;

        p_ble_nao_stat_c->evt_handler(p_ble_nao_stat_c, &ble_nao_stat_c_evt);
    }
}

uint32_t ble_nao_stat_c_init(ble_nao_stat_c_t * p_ble_nao_stat_c, ble_nao_stat_c_init_t * p_ble_nao_stat_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    uart_uuid;
    ble_uuid128_t nao_stat_base_uuid = NAO_STAT_SRV_UUID_UINT128;
        
    VERIFY_PARAM_NOT_NULL(p_ble_nao_stat_c);
    VERIFY_PARAM_NOT_NULL(p_ble_nao_stat_c_init);
    
    err_code = sd_ble_uuid_vs_add(&nao_stat_base_uuid, &p_ble_nao_stat_c->uuid_type);
    VERIFY_SUCCESS(err_code);
    
    uart_uuid.type = p_ble_nao_stat_c->uuid_type;
    uart_uuid.uuid = NAO_STAT_SRV_UUID_UINT16;
    
    p_ble_nao_stat_c->conn_handle           = BLE_CONN_HANDLE_INVALID;
    p_ble_nao_stat_c->evt_handler           = p_ble_nao_stat_c_init->evt_handler;
    p_ble_nao_stat_c->handles.nao_stat_rx_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_nao_stat_c->handles.nao_stat_tx_handle = BLE_GATT_HANDLE_INVALID;
    
    return ble_db_discovery_evt_register(&uart_uuid);
}


static void ble_nao_stat_on_write_rsp(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_evt_t * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_ble_nao_stat_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
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
         if(p_ble_nao_stat_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
         {
          NRF_LOG_INFO("Received write response to write request (0x68)\r\n");
          notif_68_subbed = true;
         }
    }

    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


void ble_nao_stat_c_on_ble_evt(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_evt_t * p_ble_evt)
{
    if ((p_ble_nao_stat_c == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    if ( (p_ble_nao_stat_c->conn_handle != BLE_CONN_HANDLE_INVALID) 
       &&(p_ble_nao_stat_c->conn_handle != p_ble_evt->evt.gap_evt.conn_handle)
       )
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            ble_nao_stat_on_hvx(p_ble_nao_stat_c, p_ble_evt);
            break;
                
        case BLE_GAP_EVT_DISCONNECTED:
            if (p_ble_evt->evt.gap_evt.conn_handle == p_ble_nao_stat_c->conn_handle
                    && p_ble_nao_stat_c->evt_handler != NULL)
            {
                ble_nao_stat_c_evt_t nao_stat_c_evt;
                
                nao_stat_c_evt.evt_type = BLE_NAO_STAT_C_EVT_DISCONNECTED;
                
                p_ble_nao_stat_c->conn_handle = BLE_CONN_HANDLE_INVALID;
                p_ble_nao_stat_c->evt_handler(p_ble_nao_stat_c, &nao_stat_c_evt);
            }
            break;

         case BLE_GATTC_EVT_WRITE_RSP:
            //ble_nao_stat_on_write_rsp(p_ble_nao_stat_c, p_ble_evt);
            break;

    }
}

uint32_t ble_nao_stat_c_rx_notif_enable(ble_nao_stat_c_t * p_ble_nao_stat_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_stat_c);

    if ( (p_ble_nao_stat_c->conn_handle == BLE_CONN_HANDLE_INVALID)
       ||(p_ble_nao_stat_c->handles.nao_stat_rx_cccd_handle == BLE_GATT_HANDLE_INVALID)
       )
    {
        return NRF_ERROR_INVALID_STATE;
    }
    return cccd_configure(p_ble_nao_stat_c->conn_handle,p_ble_nao_stat_c->handles.nao_stat_rx_cccd_handle, true);
}


uint32_t ble_nao_stat_c_string_send(ble_nao_stat_c_t * p_ble_nao_stat_c, uint8_t * p_string, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_stat_c);
    
    if (length > BLE_NAO_STAT_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
    if ( p_ble_nao_stat_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    const ble_gattc_write_params_t write_params = {
        .write_op = BLE_GATT_OP_WRITE_CMD,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_nao_stat_c->handles.nao_stat_tx_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = p_string
    };

    return sd_ble_gattc_write(p_ble_nao_stat_c->conn_handle, &write_params);
}


uint32_t ble_nao_stat_c_handles_assign(ble_nao_stat_c_t * p_ble_nao_stat,
                                  const uint16_t conn_handle,
                                  const ble_nao_stat_c_handles_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_nao_stat);

    p_ble_nao_stat->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_nao_stat->handles.nao_stat_rx_cccd_handle = p_peer_handles->nao_stat_rx_cccd_handle;
        p_ble_nao_stat->handles.nao_stat_rx_handle      = p_peer_handles->nao_stat_rx_handle;
        p_ble_nao_stat->handles.nao_stat_tx_handle      = p_peer_handles->nao_stat_tx_handle;    
    }
    return NRF_SUCCESS;
}


