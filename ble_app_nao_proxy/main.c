/**
 *
 *  Monitoring and partial control of Petzl NAO+ head lamp from Garmin Fenix 6 watch. This code is for BLE proxy part of the project. 
 *  BLE decryptor/encryptor proxy translates transmissions between NAO+ (encrypted) and Garmin Fenix 6 (unencrypted).
 *
 *  The Story: when i've read about Garmin providing BLE Delegate library enabling certain Garmin watches to control BLE devices, i've thought that it would be great to 
 *  be able to control my NAO+ head lamp using BLE Delegate library from my Garmin Fenix 6. So i've fired up BLE sniffer and started to reverse engineer NAO+ BLE interface.
 *  When i've got some basic knowledge which allowed me to check battery status, turn rear red light on/off, and do few other things, i've decided to move further and investigate
 *  possibilty of connecting NAO+ and Fenix 6. Unfortunately this turned out impossible, cause BLE Delegate does not implement BLE encryption, and NAO+ does not allow 
 *  unencrypted communications. Bummer. So are we done here? Not quite - there is another way around this problem - and that is to implement BLE proxy which would encrypt/decrypt BLE
 *  traffic between two devices making connections possible. Code below provides this functionality.
 *
 *  Code adapted from ble_app_hrs_rscs_relay example from nRF52 dev kit.
 *  (Nordic disclaimer header is removed)
 *
 *   Components needed to test:
 * 
 *   - pretty much any dev board with nRF52840 running this code
 *   - Petzl NAO+ head lamp
 *   - Garmin Fenix 6 with NAO_Control application installed
 *
 *  By woytekm, April 2020
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "ble_conn_state.h"
#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_hrs.h"
#include "ble_rscs.h"
#include "ble_hrs_c.h"
#include "ble_rscs_c.h"
#include "ble_conn_state.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_fstorage_sd.h"
#include "fds.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "SEGGER_RTT.h"

#include "nao_service_13.h"
#include "nao_service_68.h"
#include "nao_service_09.h"
#include "nao_generic.h"
#include "nao_proxychar.h"


#define NAO_UUID_13 0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0xb6,0x8a,0xf6,0x2b,0x17,0x13
#define NAO_UUID_09 0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0x08,0x7b,0x82,0xb3,0xb6,0x09
#define NAO_UUID_68 0xba,0x5c,0xf7,0x93,0x3b,0x12,0x16,0xb1,0xe4,0x11,0x08,0x7b,0x2a,0x6e,0x86,0x68
#define NAO_UUID_13_UINT16 0x2bf6
#define NAO_UUID_09_UINT16 0xb382
#define NAO_UUID_68_UINT16 0x6e2a

#define MY_LED_1          22
#define MY_LED_2          23
#define MY_LED_3          24

#define PERIPHERAL_CONNECTED_LED            MY_LED_1
#define CENTRAL_CONNECTED_LED           MY_LED_2

#define DEVICE_NAME                     "NAO Proxy"                                 /**< Name of device used for advertising. */
#define MANUFACTURER_NAME               "woytekm"                       /**< Manufacturer. Passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                         /**< The advertising interval (in units of 0.625 ms). This value corresponds to 187.5 ms. */

#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define APP_BLE_CONN_CFG_TAG            1                                           /**< Tag that identifies the SoftDevice BLE configuration. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to the first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  1                                           /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                           /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size in octets. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size in octets. */

#define HART_RATE_SERVICE_UUID_IDX      0                                           /**< Hart Rate service UUID index in array. */
#define RSCS_SERVICE_UUID_IDX           1                                           /**< RSCS service UUID index in array. */

#define CONFIG_BUF_LEN                  256
#define USERBLOCK                      0x60000

/**@brief   Priority of the application BLE event handler.
 * @note    You shouldn't need to modify this value.
 */
#define APP_BLE_OBSERVER_PRIO           3

#define DB_DISCOVERY_INSTANCE_CNT       2  /**< Number of DB Discovery instances. */


APP_TIMER_DEF(m_scheduler_timer_id);                                // timer for housekeeping routine which is not event-driven

NRF_BLE_GQ_DEF(m_ble_gatt_queue,                                    /**< BLE GATT Queue instance. */
               NRF_SDH_BLE_CENTRAL_LINK_COUNT,
               NRF_BLE_GQ_QUEUE_SIZE);
NRF_BLE_GATT_DEF(m_gatt);                                              /**< GATT module instance. */
NRF_BLE_QWRS_DEF(m_qwr, NRF_SDH_BLE_TOTAL_LINK_COUNT);                 /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                    /**< Advertising module instance. */
BLE_DB_DISCOVERY_ARRAY_DEF(m_db_discovery, 2);                         /**< Database discovery module instances. */
NRF_BLE_SCAN_DEF(m_scan);                                           /**< Scanning module instance. */

static uint16_t m_conn_handle_nao_c  = BLE_CONN_HANDLE_INVALID;     /**< Connection handle for NAO+  */

static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection to central (Garmin watch or something else). */

static nao_proxy_t                      m_nao_proxy;  // data related to connection to Fenix 6 or other peripheral device


ble_uuid128_t NAO_uuid13_128 = {{ NAO_UUID_13 }};
ble_uuid128_t NAO_uuid09_128 = {{ NAO_UUID_09 }};
ble_uuid128_t NAO_uuid68_128 = {{ NAO_UUID_68 }};

uint8_t NAO_uuid13_type;  // NAO+ Auth
uint8_t NAO_uuid09_type;  // NAO+ Configuration
uint8_t NAO_uuid68_type;  // NAO+ Status

uint8_t NAO_found;
uint8_t NAO_paired;

static ble_nao_auth_c_t              m_ble_nao_auth_c;  // structure holding data related to "service 13" - NAO+ Auth 
static ble_nao_stat_c_t              m_ble_nao_stat_c;
static ble_nao_conf_c_t              m_ble_nao_conf_c;

/**@brief Names that the central application scans for, and that are advertised by the peripherals.
 *  If these are set to empty strings, the UUIDs defined below are used.
 */

#define MAXNAME 30

static char m_default_periph_name[MAXNAME] = "NAO+_0100";
static char m_target_periph_name[MAXNAME] = ""; // NAO+_0100 is the default

nrf_fstorage_api_t *m_fs_api;

/**@brief UUIDs that the central application scans for if the name above is set to an empty string,
 * and that are to be advertised by the peripherals.
 */
#define BLE_UUID_NAO 0x2BF6

static ble_uuid_t m_adv_uuids[] =
{
    {BLE_UUID_NAO,        BLE_UUID_TYPE_BLE}
};

static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];         /**< Buffer for storing an encoded scan data. */

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};

static ble_gap_scan_params_t m_scan_param =                 /**< Scan parameters requested for scanning and connection. */
{
    .active        = 0x01,
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .timeout       = NRF_BLE_SCAN_SCAN_DURATION,
    .scan_phys     = BLE_GAP_PHY_1MBPS,
    .extended      = true,
};


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);
static void idle_state_handle(void);

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = 0x000FC000,
    .end_addr   = 0x000FE000,
};

void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        idle_state_handle();
    }
}

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation (%d).",p_evt->result);
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

void fstorage_init()
 {

   uint8_t rc;
   m_fs_api = &nrf_fstorage_sd;
   rc = nrf_fstorage_init(&fstorage, m_fs_api, NULL);
   APP_ERROR_CHECK(rc);

 }


void build_config(char *config_buffer)
 {
   snprintf(config_buffer,CONFIG_BUF_LEN,"%s:",m_target_periph_name);
 }

void apply_config(char *config_buffer)
 {
  char *token;

  //config buffer is CONFIG_BUF_LEN long, config data is separated by ":", currently only one field is implemented - NAO name, so only one token is parsed
  token = strtok(config_buffer,":");
  if(token == NULL)
   {
    strcpy(m_target_periph_name,m_default_periph_name);
    NRF_LOG_INFO("setting NAO name to: %s",m_default_periph_name);
   }
  else
   {
    strcpy(m_target_periph_name,token);
    NRF_LOG_INFO("setting NAO name to: %s",token);
   }
 }

void read_cfg_from_flash()
 {

  uint16_t len;
  uint8_t rc;

  uint8_t fstorage_data[CONFIG_BUF_LEN] = {0};

  rc = nrf_fstorage_read(&fstorage, fstorage.start_addr, fstorage_data, CONFIG_BUF_LEN);

  if (rc != NRF_SUCCESS)
   {
     NRF_LOG_INFO("error, nrf_fstorage_read returned: %s\n",nrf_strerror_get(rc));
     return;
   }

  apply_config((char *)fstorage_data);
 

 }


void write_cfg_to_flash()
 {
   uint8_t rc;

   uint8_t fstorage_data[CONFIG_BUF_LEN] = {0};

   build_config((char *)fstorage_data);
   rc = nrf_fstorage_erase(&fstorage, fstorage.start_addr, 1, NULL); 
   APP_ERROR_CHECK(rc);
   rc = nrf_fstorage_write(&fstorage, fstorage.start_addr, &fstorage_data, CONFIG_BUF_LEN, NULL);
   APP_ERROR_CHECK(rc);
   wait_for_flash_ready(&fstorage);
 }



volatile bool NAO_pair_now;
volatile uint16_t NAO_pair_timer = 0;
volatile uint16_t NAO_conn_handle = BLE_CONN_HANDLE_INVALID;


#define NAO_PAIR_TIMEOUT 20

static void housekeeping_timer_handler(void * p_context)
 {

  ret_code_t err_code;

  if(NAO_pair_now)
   {
    nrf_gpio_pin_toggle(MY_LED_3);  // flash led on proxy to indicate pairing window (press NAO knob briefly when proxy LED is flashing)
    NAO_pair_timer++;
   }
  else
   nrf_gpio_pin_clear(MY_LED_3); 

  if(NAO_pair_timer>NAO_PAIR_TIMEOUT) // pairing unsuccessful - back to scanning
   {
    NAO_pair_now = false;
    NAO_pair_timer = 0;
    err_code = sd_ble_gap_disconnect(m_conn_handle_nao_c,BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    APP_ERROR_CHECK(err_code);
   }

 }

static void create_timers()
{
    ret_code_t err_code;

    // Create timers
    err_code = app_timer_create(&m_scheduler_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                housekeeping_timer_handler);
    APP_ERROR_CHECK(err_code);
}

void board_led_on(uint8_t led)
 {
  nrf_gpio_pin_set(led); 
 }


void board_led_off(uint8_t led)
 {
  nrf_gpio_pin_clear(led);
 }


// we are not forwarding AUTH messages to Garmin. Proxy does auth and encryption by itself.

static void ble_nao_auth_c_evt_handler(ble_nao_auth_c_t * p_ble_nao_auth_c, const ble_nao_auth_c_evt_t * p_ble_nao_auth_evt)
{
    uint32_t err_code;


    NRF_LOG_INFO("ble_nao_auth_c_evt_handler: event: %X\r\n",p_ble_nao_auth_evt->evt_type);

    switch (p_ble_nao_auth_evt->evt_type)
    {
        case BLE_NAO_AUTH_C_EVT_DISCOVERY_COMPLETE:

            nrf_delay_ms(1000);

            m_conn_handle_nao_c = p_ble_nao_auth_evt->conn_handle;

            err_code = ble_nao_auth_c_handles_assign(p_ble_nao_auth_c, p_ble_nao_auth_evt->conn_handle, &p_ble_nao_auth_evt->handles);
            APP_ERROR_CHECK(err_code);
            err_code = ble_nao_auth_c_rx_notif_enable(p_ble_nao_auth_c);
            APP_ERROR_CHECK(err_code);
            NRF_LOG_INFO("The device has the NAO+ service 0x13 (AUTH)\r\n");  
            nrf_delay_ms(1000);
            break;
        
        case BLE_NAO_AUTH_C_EVT_NAO_AUTH_RX_EVT:
            NRF_LOG_INFO("Received packet from NAO+ service 0x13 (AUTH), data len:%d\r\n",p_ble_nao_auth_evt->data_len);
             
            if(p_ble_nao_auth_evt->data_len == 1)
              {
                uint8_t *data = p_ble_nao_auth_evt->p_data;

                NRF_LOG_INFO("Received packet from NAO+ service 0x13 (AUTH), data[0]:%X\r\n",data[0]);

                if(data[0] == 0xA5)
                 {
                  NAO_pair_now = true;
                  NRF_LOG_INFO("NAO+ is waiting for pairing confirmation - press NAO+ knob briefly...\r\n");
                 }
                else if(data[0] == 0xAA)
                 {
                  NRF_LOG_INFO("This NAO+ is already paired with this proxy...\r\n");
                  NAO_paired = true;
                 }
              }
            break;
        
        case BLE_NAO_AUTH_C_EVT_DISCONNECTED:
            NRF_LOG_INFO("NAO+ AUTH disconnected\r\n");
            //scan_start();
            break;
    }
}


static void ble_nao_stat_c_evt_handler(ble_nao_stat_c_t * p_ble_nao_stat_c, const ble_nao_stat_c_evt_t * p_ble_nao_stat_evt)
{
   
    uint32_t err_code;
    uint8_t *data;

    NRF_LOG_INFO("ble_nao_stat_c_evt_handler: event: %X\r\n",p_ble_nao_stat_evt->evt_type);

    switch (p_ble_nao_stat_evt->evt_type)
    {
        case BLE_NAO_STAT_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_nao_stat_c_handles_assign(p_ble_nao_stat_c, p_ble_nao_stat_evt->conn_handle, &p_ble_nao_stat_evt->handles);
            APP_ERROR_CHECK(err_code);

            err_code = ble_nao_stat_c_rx_notif_enable(p_ble_nao_stat_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("The device has the NAO+ service 0x68 (STAT)\r\n");
            NRF_LOG_INFO("Sending AUTH password to service 0x13\r\n");
            err_code = ble_nao_auth_c_send_auth(&m_ble_nao_auth_c);
            APP_ERROR_CHECK(err_code);
           

            break;

        case BLE_NAO_STAT_C_EVT_NAO_STAT_RX_EVT:
            data = p_ble_nao_stat_evt->p_data;
            NRF_LOG_INFO("Received packet from NAO+ service 0x68 (STAT), data len:%d\r\n",p_ble_nao_stat_evt->data_len);
            NRF_LOG_INFO("bytes 0-5: %02x,%02x,%02x,%02x\r\n",data[0],data[1],data[2],data[3],data[4],data[5]);
            err_code = ble_nao_stat_notif_forward(&m_nao_proxy, data, p_ble_nao_stat_evt->data_len);
            NRF_LOG_INFO("forward notification returned: %d",err_code);
            break;

        case BLE_NAO_STAT_C_EVT_DISCONNECTED:
            NRF_LOG_INFO("NAO+ STAT disconnected\r\n");
            //scan_start();
            break;
    }
}


static void ble_nao_conf_c_evt_handler(ble_nao_conf_c_t * p_ble_nao_conf_c, const ble_nao_conf_c_evt_t * p_ble_nao_conf_evt)
{

    uint32_t err_code;
    uint8_t *data;

    NRF_LOG_INFO("ble_nao_conf_c_evt_handler: event: %X\r\n",p_ble_nao_conf_evt->evt_type);

    switch (p_ble_nao_conf_evt->evt_type)
    {
        case BLE_NAO_CONF_C_EVT_DISCOVERY_COMPLETE:
            err_code = ble_nao_conf_c_handles_assign(p_ble_nao_conf_c, p_ble_nao_conf_evt->conn_handle, &p_ble_nao_conf_evt->handles);
            APP_ERROR_CHECK(err_code);

            err_code = ble_nao_conf_c_rx_notif_enable(p_ble_nao_conf_c);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("The device has the NAO+ service 0x09 (CONF)\r\n");


            break;

        case BLE_NAO_CONF_C_EVT_NAO_CONF_RX_EVT:
            data = p_ble_nao_conf_evt->p_data;
            NRF_LOG_INFO("Received packet from NAO+ service 0x09 (CONF), data len:%d\r\n",p_ble_nao_conf_evt->data_len);
            NRF_LOG_INFO("bytes 0-5: %02x,%02x,%02x,%02x\r\n",data[0],data[1],data[2],data[3],data[4],data[5]);
            err_code = ble_nao_stat_notif_forward(&m_nao_proxy, data, p_ble_nao_conf_evt->data_len);
            NRF_LOG_INFO("forward notification returned: %d",err_code);
            break;

        case BLE_NAO_CONF_C_EVT_DISCONNECTED:
            NRF_LOG_INFO("NAO+ CONF disconnected\r\n");
            //scan_start();
            break;
    }
}



static void nao_auth_c_init(void)
{
    uint32_t         err_code;
    ble_nao_auth_c_init_t nao_auth_c_init_t;
    
    nao_auth_c_init_t.evt_handler = ble_nao_auth_c_evt_handler;
    
    err_code = ble_nao_auth_c_init(&m_ble_nao_auth_c, &nao_auth_c_init_t);
    APP_ERROR_CHECK(err_code);
}

static void nao_stat_c_init(void)
{
    uint32_t         err_code;
    ble_nao_stat_c_init_t nao_stat_c_init_t;

    nao_stat_c_init_t.evt_handler = ble_nao_stat_c_evt_handler;

    err_code = ble_nao_stat_c_init(&m_ble_nao_stat_c, &nao_stat_c_init_t);
    APP_ERROR_CHECK(err_code);
}

static void nao_conf_c_init(void)
{
    uint32_t         err_code;
    ble_nao_conf_c_init_t nao_conf_c_init_t;

    nao_conf_c_init_t.evt_handler = ble_nao_conf_c_evt_handler;

    err_code = ble_nao_conf_c_init(&m_ble_nao_conf_c, &nao_conf_c_init_t);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling asserts in the SoftDevice.
 *
 * @details This function is called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and is not meant for the final product. You need to analyze
 *          how your product is supposed to react in case of assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing assert call.
 * @param[in] p_file_name  File name of the failing assert call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for handling the Heart Rate Service Client
 *        Running Speed and Cadence Service Client.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void service_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    ret_code_t err_code;
    ble_gap_evt_adv_report_t const * p_adv = 
                   p_scan_evt->params.filter_match.p_adv_report;
    ble_gap_scan_params_t    const * p_scan_param = 
                   p_scan_evt->p_scan_params;

    // NRF_LOG_INFO("Scan event: %X",p_scan_evt->scan_evt_id);

    switch(p_scan_evt->scan_evt_id)
    {
        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
        {
            // Initiate connection.
            err_code = sd_ble_gap_connect(&p_adv->peer_addr,
                                          p_scan_param,
                                          &m_scan.conn_params,
                                          APP_BLE_CONN_CFG_TAG);
            APP_ERROR_CHECK(err_code);
        } break;

        default:
            break;
    }
}


/**@brief Function for initialization the scanning and setting the filters.
 */
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    memset(&init_scan, 0, sizeof(init_scan));

    init_scan.p_scan_param = &m_scan_param;

    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);

    if (strlen(m_target_periph_name) != 0)
    {
        err_code = nrf_ble_scan_filter_set(&m_scan, 
                                           SCAN_NAME_FILTER, 
                                           m_target_periph_name);
        APP_ERROR_CHECK(err_code);
    }

    err_code = nrf_ble_scan_filter_set(&m_scan, 
                                       SCAN_UUID_FILTER, 
                                       &m_adv_uuids[0]);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_scan_filters_enable(&m_scan, 
                                           NRF_BLE_SCAN_ALL_FILTER, 
                                           false);
    APP_ERROR_CHECK(err_code);

}


/**@brief Function for initializing the scanning.
 */
static void scan_start(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the advertising and the scanning.
 */
static void adv_scan_start(void)
{
    ret_code_t err_code;

    //check if there are no flash operations in progress
    if (!nrf_fstorage_is_busy(NULL))
    {
        // Start scanning for peripherals and initiate connection to devices which
        // advertise Heart Rate or Running speed and cadence UUIDs.
        scan_start();

        // Turn on the LED to signal scanning.

        // Start advertising.
        err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
        APP_ERROR_CHECK(err_code);
        //err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        //APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_disconnect_on_sec_failure(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            adv_scan_start();
            break;

        default:
            break;
    }
}


/**@brief Function for changing filter settings after establishing the connection.
 */
static void filter_settings_change(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_scan_all_filter_remove(&m_scan);
    APP_ERROR_CHECK(err_code);

    if (strlen(m_target_periph_name) != 0)
    {
        err_code = nrf_ble_scan_filter_set(&m_scan, 
                                           SCAN_NAME_FILTER, 
                                           m_target_periph_name);
        APP_ERROR_CHECK(err_code);
    }

    if ((m_conn_handle_nao_c != BLE_CONN_HANDLE_INVALID))
    {
        err_code = nrf_ble_scan_filter_set(&m_scan, 
                                           SCAN_UUID_FILTER, 
                                           &m_adv_uuids[0]);
    }

    APP_ERROR_CHECK(err_code);
}



/**@brief Function for assigning new connection handle to available instance of QWR module.
 *
 * @param[in] conn_handle New connection handle.
 */
static void multi_qwr_conn_handle_assign(uint16_t conn_handle)
{
    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        if (m_qwr[i].conn_handle == BLE_CONN_HANDLE_INVALID)
        {
            ret_code_t err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr[i], conn_handle);
            APP_ERROR_CHECK(err_code);
            break;
        }
    }
}


/**@brief   Function for handling BLE events from the central application.
 *
 * @details This function parses scanning reports and initiates a connection to peripherals when a
 *          target UUID is found. It updates the status of LEDs used to report the central application
 *          activity.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_central_evt(ble_evt_t const * p_ble_evt)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    // NRF_LOG_INFO("Central event: %X",p_ble_evt->header.evt_id);

    ble_nao_auth_c_on_ble_evt(&m_ble_nao_auth_c,p_ble_evt);
    ble_nao_stat_c_on_ble_evt(&m_ble_nao_stat_c,p_ble_evt);
    ble_nao_conf_c_on_ble_evt(&m_ble_nao_conf_c,p_ble_evt);

    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral is connected (HR or RSC), initiate DB
        // discovery, update LEDs status, and resume scanning, if necessary.
        case BLE_GAP_EVT_CONNECTED:
        {
            NRF_LOG_INFO("Central connected");

            err_code = pm_conn_secure(p_gap_evt->conn_handle, false);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Connection secured");

            // If no Heart Rate sensor or RSC sensor is currently connected, try to find them on this peripheral.
            if (m_conn_handle_nao_c  == BLE_CONN_HANDLE_INVALID)
            {
                NRF_LOG_INFO("Attempt to find NAO+ on conn_handle 0x%x", p_gap_evt->conn_handle);
                
                err_code = ble_db_discovery_start(&m_db_discovery[0], p_gap_evt->conn_handle);

                if (err_code == NRF_ERROR_BUSY)
                {
                    err_code = ble_db_discovery_start(&m_db_discovery[1], p_gap_evt->conn_handle);
                    APP_ERROR_CHECK(err_code);
                }
                else
                {
                    APP_ERROR_CHECK(err_code);
                }
            }

            // Assign connection handle to the QWR module.
            multi_qwr_conn_handle_assign(p_gap_evt->conn_handle);

            // Update LEDs status, and check whether to look for more peripherals to connect to.
            board_led_on(CENTRAL_CONNECTED_LED);
            if (ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT)
            {
            }
            else
            {
                // Resume scanning.
                scan_start();
            }
        } break; // BLE_GAP_EVT_CONNECTED

        // Upon disconnection, reset the connection handle of the peer that disconnected,
        // update the LEDs status and start scanning again.
        case BLE_GAP_EVT_DISCONNECTED:
        {
            if (p_gap_evt->conn_handle == m_conn_handle_nao_c)
            {
                NRF_LOG_INFO("NAO central disconnected (reason: %d)",
                             p_gap_evt->params.disconnected.reason);

                m_conn_handle_nao_c = BLE_CONN_HANDLE_INVALID;
                
                err_code = nrf_ble_scan_filter_set(&m_scan, 
                                                   SCAN_UUID_FILTER, 
                                                   &m_adv_uuids[HART_RATE_SERVICE_UUID_IDX]);
                APP_ERROR_CHECK(err_code);
            }

            if (m_conn_handle_nao_c == BLE_CONN_HANDLE_INVALID)
            {
                // Start scanning.
                scan_start();

                // Update LEDs status.
            }

            if (ble_conn_state_central_conn_count() == 0)
            {
                board_led_off(CENTRAL_CONNECTED_LED);
            }
        } break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_TIMEOUT:
        {
            // No timeout for scanning is specified, so only connection attemps can timeout.
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                NRF_LOG_INFO("Connection Request timed out.");
            }
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // Accept parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}



/**@brief Function for checking whether a bluetooth stack event is an advertising timeout.
 *
 * @param[in] p_ble_evt Bluetooth stack event.
 */
static bool ble_evt_is_advertising_timeout(ble_evt_t const * p_ble_evt)
{
    return (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_SET_TERMINATED);
}


/**@brief   Function for handling BLE events from peripheral applications.
 * @details Updates the status LEDs used to report the activity of the peripheral applications.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_peripheral_evt(ble_evt_t const * p_ble_evt)
{
    ret_code_t            err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;


    nao_proxy_on_ble_evt(&m_nao_proxy,p_ble_evt);

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Peripheral connected");
            board_led_on(PERIPHERAL_CONNECTED_LED);

            // Assign connection handle to the QWR module.
            multi_qwr_conn_handle_assign(p_ble_evt->evt.gap_evt.conn_handle);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Peripheral disconnected. conn_handle: 0x%x, reason: 0x%x",
                         p_gap_evt->conn_handle,
                         p_gap_evt->params.disconnected.reason);

            board_led_off(PERIPHERAL_CONNECTED_LED);

            err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
            APP_ERROR_CHECK(err_code);

            break;

        case 38:
            NRF_LOG_INFO("Central advertising timeout. Restart.");
            err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
            APP_ERROR_CHECK(err_code);

            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint16_t conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    uint16_t role        = ble_conn_state_role(conn_handle);

    // Based on the role this device plays in the connection, dispatch to the right handler.
    if (role == BLE_GAP_ROLE_PERIPH || ble_evt_is_advertising_timeout(p_ble_evt))
    {
     on_ble_peripheral_evt(p_ble_evt);
    }
    else if ((role == BLE_GAP_ROLE_CENTRAL) || (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT))
    {
     on_ble_central_evt(p_ble_evt);
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for initializing the Peer Manager.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing buttons and LEDs.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to
 *                            wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the GAP.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device, including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = m_scan.conn_params.min_conn_interval;
    gap_conn_params.max_conn_interval = m_scan.conn_params.max_conn_interval;
    gap_conn_params.slave_latency     = m_scan.conn_params.slave_latency;
    gap_conn_params.conn_sup_timeout  = m_scan.conn_params.conn_sup_timeout;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_CONN_HANDLE_INVALID; // Start upon connection.
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;  // Ignore events.
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is a callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function forwards the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{

    ble_db_discovery_t const * p_db = (ble_db_discovery_t *)p_evt->params.p_db_instance;

    ble_nao_auth_c_on_db_disc_evt(&m_ble_nao_auth_c, p_evt);
    ble_nao_stat_c_on_db_disc_evt(&m_ble_nao_stat_c, p_evt);
    ble_nao_conf_c_on_db_disc_evt(&m_ble_nao_conf_c, p_evt);

    if (p_evt->evt_type == BLE_DB_DISCOVERY_AVAILABLE) {

        NRF_LOG_INFO("DB Discovery instance %p available on conn handle: %d",
                     p_db,
                     p_evt->conn_handle);

        NRF_LOG_INFO("Found %d services on conn_handle: %d",
                     p_db->srv_count,
                     p_evt->conn_handle);

        if(p_db->srv_count == 1)  // Found NAO+ torch specific service UUID on discovered device 
         {
          NAO_found = 1;
          NAO_conn_handle = p_evt->conn_handle;
         }

    }
}

/* NAO proxy: register NAO UUID's*/

static void NAO_register_UUIDs(void)
 {

    ret_code_t err = sd_ble_uuid_vs_add(&NAO_uuid13_128, &NAO_uuid13_type);
    APP_ERROR_CHECK(err);
    err = sd_ble_uuid_vs_add(&NAO_uuid09_128, &NAO_uuid09_type);
    APP_ERROR_CHECK(err);
    err = sd_ble_uuid_vs_add(&NAO_uuid68_128, &NAO_uuid68_type);
    APP_ERROR_CHECK(err);

 }

/**
 * @brief Database discovery initialization.
 */
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_init;

    memset(&db_init, 0, sizeof(ble_db_discovery_init_t));

    db_init.evt_handler  = db_disc_handler;
    db_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_init);
    APP_ERROR_CHECK(err_code);

    //ble_uuid_t nao_uuid_1800;
    //ble_uuid_t nao_uuid_1801;
    ble_uuid_t nao_uuid_13;
    ble_uuid_t nao_uuid_09;
    ble_uuid_t nao_uuid_68;
    
    //nao_uuid_1801.type = BLE_UUID_TYPE_BLE;
    //nao_uuid_1801.uuid = 0x1801;

    //nao_uuid_1800.type = BLE_UUID_TYPE_BLE;
    //nao_uuid_1800.uuid = 0x1800;

    nao_uuid_13.type = NAO_uuid13_type;
    nao_uuid_13.uuid = NAO_UUID_13_UINT16;  

    nao_uuid_09.type = NAO_uuid09_type;
    nao_uuid_09.uuid = NAO_UUID_09_UINT16;

    nao_uuid_68.type = NAO_uuid68_type;
    nao_uuid_68.uuid = NAO_UUID_68_UINT16; 

   // ble_db_discovery_evt_register(&nao_uuid_1801);
   // ble_db_discovery_evt_register(&nao_uuid_1800);
    ble_db_discovery_evt_register(&nao_uuid_13);
    ble_db_discovery_evt_register(&nao_uuid_09);
    ble_db_discovery_evt_register(&nao_uuid_68);

}


/**@brief Function for handling Queued Write module errors.
 *
 * @details A pointer to this function is passed to each service that may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code that contains information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}



/**@brief Function for initializing logging.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop). If there is no pending log operation,
          then sleeps until the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


/**@brief Function for initializing the timer.
 */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}


void pm_peer_delete_all(void)
{
    pm_peer_id_t current_peer_id = PM_PEER_ID_INVALID;
    while (pm_next_peer_id_get(PM_PEER_ID_INVALID) != PM_PEER_ID_INVALID)
    {
        current_peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
        pm_peer_delete(current_peer_id);
    }
} 


uint32_t proxy_local_cmd(uint8_t const *nao_write_data, uint16_t nao_write_data_len)
 {
   // CMD 0x11: erase bonds, restart
   // CMD 0x22: set NAO name (write setting to flash, erase bonds, restart)
   // CMD 0x33: get NAO name 
   switch(nao_write_data[1])
    {
     case 0x11:
       NRF_LOG_INFO("Local command 0x11 - delete all peers and restart");
       pm_peer_delete_all();
       NVIC_SystemReset();
       break;
     case 0x22:
       NRF_LOG_INFO("Local command 0x22 - set NAO name and restart (got: %s)",nao_write_data+2);
       if((nao_write_data_len > 2) && (nao_write_data_len < MAXNAME + 2))
        {
         const uint8_t *ptr;
         uint8_t terminator = 0;
         ptr = nao_write_data+2;
         memcpy(m_target_periph_name,ptr,nao_write_data_len-2);
         memcpy(m_target_periph_name+nao_write_data_len-2,&terminator,1);
         write_cfg_to_flash();
         pm_peer_delete_all();
         NVIC_SystemReset();     
        }
       break;
     case 0x33:
       NRF_LOG_INFO("Local command 0x33 - get NAO name");
       uint8_t notif_buffer[20];
       bzero(notif_buffer,20);
       uint8_t err_code;
       notif_buffer[0] = 0x77;
       notif_buffer[1] = 0x77;
       strcpy((char *)notif_buffer+2, (char *)m_target_periph_name);
       NRF_LOG_INFO("sending NAO name: %s",notif_buffer+2);
       err_code = ble_nao_stat_notif_forward(&m_nao_proxy,notif_buffer,20);
       NRF_LOG_INFO("forward notification returned: %d",err_code);
       free(notif_buffer);
       break;
    }
   return NRF_SUCCESS;
 }


static void nao_write_handler(nao_proxy_t * p_lbs, uint8_t const *nao_write_data, uint16_t nao_write_data_len)
{
  uint8_t const *nao_characteristic_addr;
  uint8_t const *data_buffer = nao_write_data + 1; // increment buffer address by 1 to skip first byte
  ret_code_t err_code;

  nao_characteristic_addr = nao_write_data; // first byte in data array is NAO service address: 0x09, 0x13 or 0x68. 0x69 will be a local command to the proxy.
 
  NRF_LOG_INFO("Received write from peripheral: %X, %X, %X, %X, ... (len: %d)",nao_write_data[0], nao_write_data[1], nao_write_data[2], nao_write_data[3],nao_write_data_len);

  switch(*nao_characteristic_addr)
    {
      case 0x09:
       NRF_LOG_INFO("write to service 09");
       if(m_conn_handle_nao_c != BLE_CONN_HANDLE_INVALID)
        {
         err_code =  ble_nao_characteristic_write(m_ble_nao_conf_c.conn_handle, m_ble_nao_conf_c.handles.nao_conf_tx_handle, data_buffer, nao_write_data_len - 1);
         APP_ERROR_CHECK(err_code);
        }
       else
        NRF_LOG_INFO("cannot write - NAO not connected");
       break;
      case 0x13:
       NRF_LOG_INFO("write to service 13");
       if(m_conn_handle_nao_c != BLE_CONN_HANDLE_INVALID)
        {
         err_code =  ble_nao_characteristic_write(m_ble_nao_auth_c.conn_handle, m_ble_nao_auth_c.handles.nao_auth_tx_handle, data_buffer, nao_write_data_len - 1);
         APP_ERROR_CHECK(err_code);
        }
       else
        NRF_LOG_INFO("cannot write - NAO not connected");       
       break;
      case 0x68:
       NRF_LOG_INFO("write to service 68");
       if(m_conn_handle_nao_c != BLE_CONN_HANDLE_INVALID)
        {
         err_code =  ble_nao_characteristic_write(m_ble_nao_stat_c.conn_handle, m_ble_nao_stat_c.handles.nao_stat_tx_handle, data_buffer, nao_write_data_len - 1);
         APP_ERROR_CHECK(err_code);
        }
       else
        NRF_LOG_INFO("cannot write - NAO not connected");
       break;
      case 0x69:
       NRF_LOG_INFO("local proxy command (69)");
       if(nao_write_data_len>1)
        proxy_local_cmd(nao_write_data, nao_write_data_len);
    }
  
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t err_code;
    nao_proxy_init_t init;
    nrf_ble_qwr_init_t qwr_init = {0};

    qwr_init.error_handler = nrf_qwr_error_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; i++)
    {
        err_code = nrf_ble_qwr_init(&m_qwr[i], &qwr_init);
        APP_ERROR_CHECK(err_code);
    }

    init.nao_write_handler = nao_write_handler;

    err_code = nao_proxy_init(&m_nao_proxy, &init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    ret_code_t    err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;

    ble_uuid_t adv_uuids[] = {{NAO_PROXY_UUID_SERVICE, m_nao_proxy.uuid_type}};

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;


    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    srdata.uuids_complete.p_uuids  = adv_uuids;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

    ble_gap_adv_params_t adv_params;

    // Set advertising parameters.
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
    adv_params.duration        = APP_ADV_DURATION;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr     = NULL;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = APP_ADV_INTERVAL;

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the application main entry.
 */
int main(void)
{
    bool erase_bonds = 0;
    ret_code_t      err_code;

    NAO_pair_now = false;
    NAO_paired = false;

    nrf_gpio_cfg_output(MY_LED_1);
    nrf_gpio_cfg_output(MY_LED_2);
    nrf_gpio_cfg_output(MY_LED_3);

    board_led_off(MY_LED_2);
    board_led_off(MY_LED_3);
    board_led_off(MY_LED_1);

    NAO_found = 0;

    // Initialize.
    log_init();

    NRF_LOG_INFO("Starting program...");    

    timer_init();
    create_timers();

    err_code = app_timer_start(m_scheduler_timer_id, APP_TIMER_TICKS(200), NULL);
    APP_ERROR_CHECK(err_code);

    power_management_init();
    ble_stack_init();
    fstorage_init();
    read_cfg_from_flash();
    scan_init();
    gap_params_init();
    gatt_init();
    conn_params_init();
    NAO_register_UUIDs();
    db_discovery_init();
    peer_manager_init();
    // pm_peer_delete_all(); 
 
    nao_auth_c_init();   
    nao_stat_c_init();
    nao_conf_c_init();

    services_init();
    advertising_init();


    // Start execution.
    NRF_LOG_INFO("NAO Proxy start.");

    if (erase_bonds == true)
    {
        // Scanning and advertising is done upon PM_EVT_PEERS_DELETE_SUCCEEDED event.
        delete_bonds();
    }
    else
    {
        adv_scan_start();
    }

    // Enter main loop.
    // Application is entirely event-driven, except timer-triggered housekeeping routine
    for (;;)
    {
        idle_state_handle();
    }
}
