#define NAO_AUTH_SRV_UUID_UINT128 {{0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0xb6,0x8a,0xf6,0x2b,0x17,0x13}}
#define NAO_AUTH_SRV_UUID_UINT16 0x2bf6

#define BLE_UUID_NAO_AUTH_TX_CHARACTERISTIC 0x31D2
#define BLE_UUID_NAO_AUTH_RX_CHARACTERISTIC 0x304C

#define GATT_MTU_SIZE_DEFAULT BLE_GATT_ATT_MTU_DEFAULT

#define BLE_NAO_AUTH_MAX_DATA_LEN           (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


/**@brief NAO_AUTH Client event type. */
typedef enum
{
    BLE_NAO_AUTH_C_EVT_DISCOVERY_COMPLETE = 10, /**< Event indicating that the NAO_AUTH service and its characteristics was found. */
    BLE_NAO_AUTH_C_EVT_NAO_AUTH_RX_EVT,             /**< Event indicating that the central has received something from a peer. */
    BLE_NAO_AUTH_C_EVT_DISCONNECTED            /**< Event indicating that the NAO_AUTH server has disconnected. */
} ble_nao_auth_c_evt_type_t;


/**@brief Handles on the connected peer device needed to interact with it.
*/
typedef struct {
    uint16_t                nao_auth_rx_handle;      /**< Handle of the NAO_AUTH RX characteristic as provided by a discovery. */
    uint16_t                nao_auth_rx_cccd_handle; /**< Handle of the CCCD of the NAO_AUTH RX characteristic as provided by a discovery. */
    uint16_t                nao_auth_tx_handle;      /**< Handle of the NAO_AUTH TX characteristic as provided by a discovery. */
} ble_nao_auth_c_handles_t;


/**@brief Structure containing the NAO_AUTH event data received from the peer. */
typedef struct {
    ble_nao_auth_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint8_t             *p_data;
    uint8_t              data_len;
    ble_nao_auth_c_handles_t  handles;     /**< Handles on which the Nordic Uart service characteristics was discovered on the peer device. This will be filled if the evt_type is @ref BLE_NAO_AUTH_C_EVT_DISCOVERY_COMPLETE.*/
} ble_nao_auth_c_evt_t;


// Forward declaration of the ble_nao_auth_t type.
typedef struct ble_nao_auth_c_s ble_nao_auth_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_nao_auth_c_evt_handler_t)(ble_nao_auth_c_t * p_ble_nao_auth_c, const ble_nao_auth_c_evt_t * p_evt);


/**@brief NAO_AUTH Client structure.
 */
struct ble_nao_auth_c_s
{
    uint8_t                 uuid_type;          /**< UUID type. */
    uint16_t                conn_handle;        /**< Handle of the current connection. Set with @ref ble_nao_auth_c_handles_assign when connected. */
    ble_nao_auth_c_handles_t     handles;            /**< Handles on the connected peer device needed to interact with it. */
    ble_nao_auth_c_evt_handler_t evt_handler;        /**< Application event handler to be called when there is an event related to the NAO_AUTH. */
};

/**@brief NAO_AUTH Client initialization structure.
 */
typedef struct {
    ble_nao_auth_c_evt_handler_t evt_handler;
} ble_nao_auth_c_init_t;


static bool notif_13_subbed;

void ble_nao_auth_c_on_db_disc_evt(ble_nao_auth_c_t * p_ble_nao_auth_c, ble_db_discovery_evt_t * p_evt);
uint32_t ble_nao_auth_c_init(ble_nao_auth_c_t * p_ble_nao_auth_c, ble_nao_auth_c_init_t * p_ble_nao_auth_c_init);
void ble_nao_auth_c_on_ble_evt(ble_nao_auth_c_t * p_ble_nao_auth_c, const ble_evt_t * p_ble_evt);
uint32_t ble_nao_auth_c_rx_notif_enable(ble_nao_auth_c_t * p_ble_nao_auth_c);
uint32_t ble_nao_auth_c_handles_assign(ble_nao_auth_c_t * p_ble_nao_auth, const uint16_t conn_handle, const ble_nao_auth_c_handles_t * p_peer_handles);
uint32_t ble_nao_auth_c_send_auth(ble_nao_auth_c_t * p_ble_nao_auth_c);
