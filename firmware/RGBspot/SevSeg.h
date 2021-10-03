#define SS_CATHODE 0
#define SS_ANODE 1
#define SS_PRD 1000

static const uint8_t _digs[] = {0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b};

template <uint8_t _DIG_AM, bool _TYPE = SS_CATHODE>
class SevSeg {
  public:
    SevSeg(const uint8_t* dig, const uint8_t* seg) : _dig(dig), _seg(seg) {
      for (int i = 0; i < _DIG_AM; i++) pinMode(_dig[i], OUTPUT);
      for (int i = 0; i < 7; i++) pinMode(_seg[i], OUTPUT);
    }

    void tick() {
      if (micros() - tmr >= SS_PRD) {
        tmr += SS_PRD;
        digitalWrite(_dig[_count], !_TYPE);
        if (++_count >= _DIG_AM) _count = 0;
        display(_buf[_count]);
        digitalWrite(_dig[_count], _TYPE);
      }
    }

    void setInt(int data) {
      int count = 0;
      bool minus = data < 0;
      data = abs(data);
      do {
        _buf[count] = _digs[data % 10];
        data /= 10;
        count++;
        if (count == _DIG_AM) return;
      } while (data);
      if (minus) _buf[count] = 0x01;
    }

    void clear() {
      for (int i = 0; i < _DIG_AM; i++) _buf[i] = 0;
    }

    void setOneByte(uint8_t pos, uint8_t data) {
      _buf[pos] = data;
    }

  private:
    void display(uint8_t data) {
      if (_TYPE) data = ~data;
      int i = 7;
      while (i--) {
        digitalWrite(_seg[i], data & 1);
        data >>= 1;
      }
    }

    const uint8_t *_dig, *_seg;
    uint8_t _buf[_DIG_AM];
    uint8_t _count = 0;
    uint32_t tmr = 0;
};
