#include "nao_proxychar.h"


#define NAO_STAT_SRV_UUID_UINT128 {{0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0x08,0x7b,0x2a,0x6e,0x86,0x68}}
#define NAO_UUID_68_UINT16 0x6e2a

#define NAO_STAT_SRV_UUID_UINT16 0x6e2a

#define BLE_UUID_NAO_STAT_TX_CHARACTERISTIC 0x7230
#define BLE_UUID_NAO_STAT_RX_CHARACTERISTIC 0x738E

#define GATT_MTU_SIZE_DEFAULT BLE_GATT_ATT_MTU_DEFAULT

#define BLE_NAO_STAT_MAX_DATA_LEN           (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


/* NAO+ message 2003 */

typedef struct {
    uint16_t signature;
    uint32_t units_left;
    uint16_t voltage;
    uint16_t PWM_width;
 } ble_nao_stat_message_2003_t;


/**@brief NAO_STAT Client event type. */
typedef enum
{
    BLE_NAO_STAT_C_EVT_DISCOVERY_COMPLETE = 20, /**< Event indicating that the NAO_STAT service and its characteristics was found. */
    BLE_NAO_STAT_C_EVT_NAO_STAT_RX_EVT,             /**< Event indicating that the central has received something from a peer. */
    BLE_NAO_STAT_C_EVT_DISCONNECTED            /**< Event indicating that the NAO_STAT server has disconnected. */
} ble_nao_stat_c_evt_type_t;


/**@brief Handles on the connected peer device needed to interact with it.
*/
typedef struct {
    uint16_t                nao_stat_rx_handle;      /**< Handle of the NAO_STAT RX characteristic as provided by a discovery. */
    uint16_t                nao_stat_rx_cccd_handle; /**< Handle of the CCCD of the NAO_STAT RX characteristic as provided by a discovery. */
    uint16_t                nao_stat_tx_handle;      /**< Handle of the NAO_STAT TX characteristic as provided by a discovery. */
} ble_nao_stat_c_handles_t;


/**@brief Structure containing the NAO_STAT event data received from the peer. */
typedef struct {
    ble_nao_stat_c_evt_type_t evt_type;
    uint16_t             conn_handle;
    uint8_t             *p_data;
    uint8_t              data_len;
    ble_nao_stat_c_handles_t  handles;     /**< Handles on which the Nordic Uart service characteristics was discovered on the peer device. This will be filled if the evt_type is @ref BLE_NAO_STAT_C_EVT_DISCOVERY_COMPLETE.*/
} ble_nao_stat_c_evt_t;


// Forward declaration of the ble_nao_stat_t type.
typedef struct ble_nao_stat_c_s ble_nao_stat_c_t;

/**@brief   Event handler type.
 *
 * @details This is the type of the event handler that should be provided by the application
 *          of this module to receive events.
 */
typedef void (* ble_nao_stat_c_evt_handler_t)(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_nao_stat_c_evt_t * p_evt);


/**@brief NAO_STAT Client structure.
 */
struct ble_nao_stat_c_s
{
    uint8_t                 uuid_type;          /**< UUID type. */
    uint16_t                conn_handle;        /**< Handle of the current connection. Set with @ref ble_nao_stat_c_handles_assign when connected. */
    ble_nao_stat_c_handles_t     handles;            /**< Handles on the connected peer device needed to interact with it. */
    ble_nao_stat_c_evt_handler_t evt_handler;        /**< Application event handler to be called when there is an event related to the NAO_STAT. */
};

/**@brief NAO_STAT Client initialization structure.
 */
typedef struct {
    ble_nao_stat_c_evt_handler_t evt_handler;
} ble_nao_stat_c_init_t;

static bool notif_68_subbed;

void ble_nao_stat_c_on_db_disc_evt(ble_nao_stat_c_t * p_ble_nao_stat_c, ble_db_discovery_evt_t * p_evt);
uint32_t ble_nao_stat_c_init(ble_nao_stat_c_t * p_ble_nao_stat_c, ble_nao_stat_c_init_t * p_ble_nao_stat_c_init);
void ble_nao_stat_c_on_ble_evt(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_evt_t * p_ble_evt);
uint32_t ble_nao_stat_c_rx_notif_enable(ble_nao_stat_c_t * p_ble_nao_stat_c);
uint32_t ble_nao_stat_c_handles_assign(ble_nao_stat_c_t * p_ble_nao_stat, const uint16_t conn_handle, const ble_nao_stat_c_handles_t * p_peer_handles);
uint32_t ble_nao_stat_c_send_auth(ble_nao_stat_c_t * p_ble_nao_stat_c);
uint32_t ble_nao_stat_notif_forward(nao_proxy_t * p_proxy, uint8_t *data, uint16_t data_len);

