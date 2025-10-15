
// SPI Configuration with PINs
#define SPI_HID_RECEIVER     SPI2_HOST
#define GPIO_MOSI            39
#define GPIO_MISO            40
#define GPIO_SCLK            41
#define GPIO_CS              42

#define SPI_PC_SENDER        SPI3_HOST
#define GPIO_MOSI2           38
#define GPIO_MISO2           37
#define GPIO_SCLK2           36
#define GPIO_CS2             35

typedef union {
    hid_keyboard_report_t keyboard;
    hid_mouse_report_t mouse;
} hid_report_t;
typedef struct {
    uint8_t header;
    hid_report_t event;
} hid_transmit_t;
typedef struct {
    hid_transmit_t hid;
    uint16_t crc;
} spi_hid_transmit_t;

typedef struct {
    uint8_t header;
    uint8_t led;
    uint16_t crc;
} spi_pc_transmit_t;

void spi_init_slave_hid_receiver();
void spi_init_master_pc_sender();
void spi_send_master_pc_sender(uint8_t const* buffer);
