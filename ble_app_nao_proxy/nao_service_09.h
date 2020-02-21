#define NAO_CONF_SRV_UUID_UINT128 {{ 0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0x08,0x7b,0x82,0xb3,0xb6,0x09 }}
#define NAO_UUID_09_UINT16 0xb382


#define NAO_CONF_SRV_UUID_UINT16 0xb382

#define BLE_UUID_NAO_CONF_TX_CHARACTERISTIC 0xB5DA
#define BLE_UUID_NAO_CONF_RX_CHARACTERISTIC 0xB72E

#define GATT_MTU_SIZE_DEFAULT BLE_GATT_ATT_MTU_DEFAULT

#define BLE_NAO_CONF_MAX_DATA_LEN           (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


/**@brief NAO_CONF Client event type. */
typedef enum
{
    BLE_NAO_CONF_C_EVT_DISCOVERY_COMPLETE = 30, /**< Event indicating that the NAO_CONF service and its characteristics was found. */
    BLE_NAO_CONF_C_EVT_NAO_CONF_RX_EVT,             /**< Event indicating that the central has received something from a peer. */
    BLE_NAO_CONF_C_EVT_DISCONNECTED            /**< Event indicating that the NAO_CONF server has disconnected. */
} ble_nao_conf_c_evt_type_t;


/**@brief Handles on the connected peer device needed to interact with it.
*/
typedef struct {
    uint16_t                nao_conf_rx_handle;      /**< Handle of the NAO_CONF RX characteristic as provided by a discovery. */
    uint16_t                nao_conf_rx_cccd_handle; /**< Handle of the CCCD of the NAO_CONF RX characteristic as provided by a discovery. */
    uint16_t                nao_conf_tx_handle;      /**< Handle of the NAO_CONF TX characteristic as provided by a discovery. */
} ble_nao_conf_c_handles_t;


/**@brief Structure containing the NAO_CONF event data received from the peer. */
typedef struct {
    ble_nao_conf_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint8_t             *p_data;
    uint8_t              data_len;
    ble_nao_conf_c_handles_t  handles;     /**< Handles on which the Nordic Uart service characteristics was discovered on the peer device. This will be filled if the evt_type is @ref BLE_NAO_CONF_C_EVT_DISCOVERY_COMPLETE.*/
} ble_nao_conf_c_evt_t;


// Forward declaration of the ble_nao_conf_t type.
typedef struct ble_nao_conf_c_s ble_nao_conf_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_nao_conf_c_evt_handler_t)(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_nao_conf_c_evt_t * p_evt);


/**@brief NAO_CONF Client structure.
 */
struct ble_nao_conf_c_s
{
    uint8_t                 uuid_type;          /**< UUID type. */
    uint16_t                conn_handle;        /**< Handle of the current connection. Set with @ref ble_nao_conf_c_handles_assign when connected. */
    ble_nao_conf_c_handles_t     handles;            /**< Handles on the connected peer device needed to interact with it. */
    ble_nao_conf_c_evt_handler_t evt_handler;        /**< Application event handler to be called when there is an event related to the NAO_CONF. */
};

/**@brief NAO_CONF Client initialization structure.
 */
typedef struct {
    ble_nao_conf_c_evt_handler_t evt_handler;
} ble_nao_conf_c_init_t;

static bool notif_09_subbed;

void ble_nao_conf_c_on_db_disc_evt(ble_nao_conf_c_t * p_ble_nao_conf_c, ble_db_discovery_evt_t * p_evt);
uint32_t ble_nao_conf_c_init(ble_nao_conf_c_t * p_ble_nao_conf_c, ble_nao_conf_c_init_t * p_ble_nao_conf_c_init);
void ble_nao_conf_c_on_ble_evt(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_evt_t * p_ble_evt);
uint32_t ble_nao_conf_c_rx_notif_enable(ble_nao_conf_c_t * p_ble_nao_conf_c);
uint32_t ble_nao_conf_c_handles_assign(ble_nao_conf_c_t * p_ble_nao_conf, const uint16_t conn_handle, const ble_nao_conf_c_handles_t * p_peer_handles);


