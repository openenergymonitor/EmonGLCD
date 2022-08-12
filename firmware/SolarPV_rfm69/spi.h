template< int N>
class SpiDev {
  static uint8_t spiTransferByte (uint8_t out) {
    SPDR = out;
    while ((SPSR & (1<<SPIF)) == 0)
      ;
    return SPDR;
  }

public:
  static void master (int div) {
    digitalWrite(N, 1);
    pinMode(N, OUTPUT);

    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    pinMode(12, INPUT);
    pinMode(13, OUTPUT);

    SPCR = _BV(SPE) | _BV(MSTR);
    SPSR |= _BV(SPI2X);
  }

  static uint8_t rwReg (uint8_t cmd, uint8_t val) {
    digitalWrite(N, 0);
    spiTransferByte(cmd);
    uint8_t in = spiTransferByte(val);
    digitalWrite(N, 1);
    return in;
  }
};

typedef SpiDev<10> SpiDev10;
