#define TX_BUFFER_MASK         0x07

static uint32_t      m_tx_index = 0;

void tx_buffer_process(void);
uint32_t cccd_configure(uint16_t conn_handle, uint16_t cccd_handle, bool enable);

