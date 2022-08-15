/* 
 * Native mode RF69 driver.
 *
 * Derived from the JeeLabs file rf69.h, with some small changes (RSSI)
 *
 * OpenEnergyMonitor version V1.0.0 
 *
 * Note 1: Default transmit power reduced to +7 dBm to avoid damage to RFM if no effective antenna is present.
 * Note 2: This is not related to the JeeLib file RF69.h [RF in capital letters]
 */


#define RFM69_MAXDATA 62

#ifndef chThdYield
#define chThdYield() // FIXME should be renamed, ChibiOS leftover
#endif

template< typename SPI >
class RF69 {
  public:
    void init (uint8_t id, uint8_t group, int freq);
    void encrypt (const char* key);
    void txPower (uint8_t level);   // 0 - 31 min -18 dBm, steps of 1 dBm, max = +13 dBm

    int receive (void* ptr, int len);
    void send (uint8_t header, const void* ptr, int len);
    void sleep ();

    int16_t afc;
    uint8_t rssi;
    uint8_t lna;
    uint8_t myId;
    uint8_t parity;

    uint8_t readReg (uint8_t addr) {
      return spi.rwReg(addr, 0);
    }
    void writeReg (uint8_t addr, uint8_t val) {
      spi.rwReg(addr | 0x80, val);
    }

  protected:
    enum {
      REG_FIFO          = 0x00,
      REG_OPMODE        = 0x01,
      REG_FRFMSB        = 0x07,
      REG_PALEVEL       = 0x11,
      REG_LNAVALUE      = 0x18,
      REG_AFCMSB        = 0x1F,
      REG_AFCLSB        = 0x20,
      REG_FEIMSB        = 0x21,
      REG_FEILSB        = 0x22,
      REG_RSSIVALUE     = 0x24,
      REG_IRQFLAGS1     = 0x27,
      REG_IRQFLAGS2     = 0x28,
      REG_SYNCVALUE1    = 0x2F,
      REG_SYNCVALUE2    = 0x30,
      REG_NODEADDR      = 0x39,
      REG_BCASTADDR     = 0x3A,
      REG_FIFOTHRESH    = 0x3C,
      REG_PKTCONFIG2    = 0x3D,
      REG_AESKEYMSB     = 0x3E,

      MODE_SLEEP        = 0<<2,
      MODE_STANDBY      = 1<<2,
      MODE_TRANSMIT     = 3<<2,
      MODE_RECEIVE      = 4<<2,

      START_TX          = 0xC2,
      STOP_TX           = 0x42,

      RCCALSTART        = 0x80,
      IRQ1_MODEREADY    = 1<<7,
      IRQ1_RXREADY      = 1<<6,
      IRQ1_SYNADDRMATCH = 1<<0,

      IRQ2_FIFONOTEMPTY = 1<<6,
      IRQ2_PACKETSENT   = 1<<3,
      IRQ2_PAYLOADREADY = 1<<2,
    };

    void setMode (uint8_t newMode);
    void configure (const uint8_t* p);
    void setFrequency (uint32_t freq);

    SPI spi;
    volatile uint8_t mode;
};

// driver implementation

template< typename SPI >
void RF69<SPI>::setMode (uint8_t newMode) {
  mode = newMode;
  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | newMode);
  while ((readReg(REG_IRQFLAGS1) & IRQ1_MODEREADY) == 0)
    ;
}

template< typename SPI >
void RF69<SPI>::setFrequency (uint32_t hz) {
  // accept any frequency scale as input, including kHz and MHz
  // multiply by 10 until freq >= 100 MHz (don't specify 0 as input!)
  while (hz < 100000000)
    hz *= 10;

  // Frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
  // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
  // due to this, the lower 6 bits of the calculated factor will always be 0
  // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
  // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000
  // 434.0 MHz = 0x6C8000.
  uint32_t frf = (hz << 2) / (32000000L >> 11);
  writeReg(REG_FRFMSB, frf >> 10);
  writeReg(REG_FRFMSB+1, frf >> 2);
  writeReg(REG_FRFMSB+2, frf << 6);
}

template< typename SPI >
void RF69<SPI>::configure (const uint8_t* p) {
  while (true) {
    uint8_t cmd = p[0];
    if (cmd == 0)
      break;
    writeReg(cmd, p[1]);
    p += 2;
  }
}

static const uint8_t configRegs [] = {
// POR value is better for first rf_sleep  0x01, 0x00, // OpMode = sleep
  0x02, 0x00, // DataModul = packet mode, fsk
  0x03, 0x02, // BitRateMsb, data rate = 49,261 bits/s
  0x04, 0x8A, // BitRateLsb, divider = 32 MHz / 650
  0x05, 0x02, // FdevMsb = 45 kHz
  0x06, 0xE1, // FdevLsb = 45 kHz
  0x0B, 0x20, // Low M
  0x11, 0x99, // OutputPower = +7 dBm - was default = max = +13 dBm
  0x19, 0x4A, // RxBw 100 kHz
  0x1A, 0x42, // AfcBw 125 kHz
  0x1E, 0x0C, // AfcAutoclearOn, AfcAutoOn
  //0x25, 0x40, //0x80, // DioMapping1 = SyncAddress (Rx)
  0x26, 0x07, // disable clkout
  0x29, 0xA0, // RssiThresh -80 dB
  0x2D, 0x05, // PreambleSize = 5
  0x2E, 0x88, // SyncConfig = sync on, sync size = 2
  0x2F, 0x2D, // SyncValue1 = 0x2D
  0x37, 0xD0, // PacketConfig1 = variable, white, no filtering
  0x38, 0x42, // PayloadLength = 0, unlimited
  0x3C, 0x8F, // FifoThresh, not empty, level 15
  0x3D, 0x12, // 0x10, // PacketConfig2, interpkt = 1, autorxrestart off
  0x6F, 0x20, // TestDagc ...
  0x71, 0x02, // RegTestAfc
  0
};

template< typename SPI >
void RF69<SPI>::init (uint8_t id, uint8_t group, int freq) {
  myId = id;

  // b7 = group b7^b5^b3^b1, b6 = group b6^b4^b2^b0
  parity = group ^ (group << 4);
  parity = (parity ^ (parity << 2)) & 0xC0;

  // 10 MHz, i.e. 30 MHz / 3 (or 4 MHz if clock is still at 12 MHz)
  spi.master(3);
  do
    writeReg(REG_SYNCVALUE1, 0xAA);
  while (readReg(REG_SYNCVALUE1) != 0xAA);
  do
    writeReg(REG_SYNCVALUE1, 0x55);
  while (readReg(REG_SYNCVALUE1) != 0x55);

  configure(configRegs);
  configure(configRegs); // TODO why is this needed ???
  setFrequency(freq);

  writeReg(REG_SYNCVALUE2, group);
}

template< typename SPI >
void RF69<SPI>::encrypt (const char* key) {
  uint8_t cfg = readReg(REG_PKTCONFIG2) & ~0x01;
  if (key) {
    for (int i = 0; i < 16; ++i) {
      writeReg(REG_AESKEYMSB + i, *key);
      if (*key != 0)
        ++key;
    }
    cfg |= 0x01;
  }
  writeReg(REG_PKTCONFIG2, cfg);
}

template< typename SPI >
void RF69<SPI>::txPower (uint8_t level) {
  writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & ~0x1F) | level);
}

template< typename SPI >
void RF69<SPI>::sleep () {
  setMode(MODE_SLEEP);
}

template< typename SPI >
int RF69<SPI>::receive (void* ptr, int len) {
  if (mode != MODE_RECEIVE)
    setMode(MODE_RECEIVE);
  else {
    static uint8_t lastFlag;
    if ((readReg(REG_IRQFLAGS1) & IRQ1_RXREADY) != lastFlag) {
      lastFlag ^= IRQ1_RXREADY;
      if (lastFlag) { // flag just went from 0 to 1
        lna = (readReg(REG_LNAVALUE) >> 3) & 0x7;
        rssi = readReg(REG_RSSIVALUE);  // It appears this can report RSSI of the previous packet.
#if RF69_SPI_BULK
        spi.enable();
        spi.transfer(REG_AFCMSB);
        afc = spi.transfer(0) << 8;
        afc |= spi.transfer(0);
        spi.disable();
#else
        afc = readReg(REG_AFCMSB) << 8;
        afc |= readReg(REG_AFCLSB);
#endif
      }
    }

    if (readReg(REG_IRQFLAGS2) & IRQ2_PAYLOADREADY) {

#if RF69_SPI_BULK
      spi.enable();
      spi.transfer(REG_FIFO);
      int count = spi.transfer(0);
      for (int i = 0; i < count; ++i) {
        uint8_t v = spi.transfer(0);
        if (i < len)
          ((uint8_t*) ptr)[i] = v;
      }
      spi.disable();
#else
      int count = readReg(REG_FIFO);
      for (int i = 0; i < count; ++i) {
        uint8_t v = readReg(REG_FIFO);
        if (i < len)
          ((uint8_t*) ptr)[i] = v;
      }
#endif

      // only accept packets intended for us, or broadcasts
      // ... or any packet if we're the special catch-all node
      rssi = readReg(REG_RSSIVALUE);   // Duplicated here - RW
      uint8_t dest = *(uint8_t*) ptr;
      if ((dest & 0xC0) == parity) {
        uint8_t destId = dest & 0x3F;
        if (destId == myId || destId == 0 || myId == 63)
          return count;
      }
    }
  }
  return -1;
}

template< typename SPI >
void RF69<SPI>::send (uint8_t header, const void* ptr, int len) {
  setMode(MODE_SLEEP);
#if RF69_SPI_BULK
  spi.enable();
  spi.transfer(REG_FIFO | 0x80);
  spi.transfer(len + 2);
  spi.transfer((header & 0x3F) | parity);
  spi.transfer((header & 0xC0) | myId);
  for (int i = 0; i < len; ++i)
    spi.transfer(((const uint8_t*) ptr)[i]);
  spi.disable();
#else
  writeReg(REG_FIFO, len + 2);
  writeReg(REG_FIFO, (header & 0x3F) | parity);
  writeReg(REG_FIFO, (header & 0xC0) | myId);
  for (int i = 0; i < len; ++i)
    writeReg(REG_FIFO, ((const uint8_t*) ptr)[i]);
#endif

  setMode(MODE_TRANSMIT);
  while ((readReg(REG_IRQFLAGS2) & IRQ2_PACKETSENT) == 0)
    chThdYield();

  setMode(MODE_STANDBY);
}
