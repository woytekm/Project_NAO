#define TX_BUFFER_MASK         0x07

static uint32_t      m_tx_index = 0;

void tx_buffer_process(void);
uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable);
uint32_t ble_nao_characteristic_write(uint16_t conn_handle, uint16_t char_tx_handle, uint8_t const *buffer, uint16_t buffer_len);


