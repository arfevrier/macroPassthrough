
// SPI Configuration with PINs
#define SPI_HID_SENDER      SPI2_HOST
#define GPIO_MOSI           39
#define GPIO_MISO           40
#define GPIO_SCLK           41
#define GPIO_CS             42

#define SPI_PC_RECEIVER     SPI3_HOST
#define GPIO_MOSI2          38
#define GPIO_MISO2          37
#define GPIO_SCLK2          36
#define GPIO_CS2            35

typedef struct {
    uint8_t header;
    char led;
    uint16_t crc;
} spi_pc_transmit_t;

typedef struct {
    uint8_t header;
    hid_report_t event;
} hid_transmit_t;
typedef struct {
    hid_transmit_t hid;
    uint16_t crc;
} spi_hid_transmit_t;

void spi_task_slave_pc_receiver(void *pvParameters);
void spi_init_master_hid_sender();
void spi_init_slave_pc_receiver();
void spi_uninstall();
void spi_send_master_hid_sender(uint8_t type, hid_report_t* report);
