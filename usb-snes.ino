#include <hidboot.h>
#include <usbhub.h>

#define DATA_PIN A0
#define CLOCK_PIN A4
#define LATCH_PIN A2
#define DATA_BIT 0  // A0 = PC0
#define CLOCK_BIT 4 // A4 = PC4
#define LATCH_BIT 2 // A2 = PC2

USB     Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> Keyboard(&Usb);

volatile uint16_t buttonState = 0;

class KbdRptParser : public KeyboardReportParser {
    void OnKeyDown(uint8_t mod, uint8_t key) override {
      switch (key) {
        case 0x04: buttonState |= (1 << 8); break;  // 'A'
        case 0x05: buttonState |= (1 << 0); break;  // 'B'
        case 0x1A: buttonState |= (1 << 9); break;  // 'X'
        case 0x1C: buttonState |= (1 << 1); break;  // 'Y'
        case 0x28: buttonState |= (1 << 3); break;  // Enter → Start
        case 0x2A: buttonState |= (1 << 2); break;  // Backspace → Select
        case 0x15: buttonState |= (1 << 10); break; // LShift → L
        case 0x0F: buttonState |= (1 << 11); break; // LGUI → R
        case 0x52: buttonState |= (1 << 4); break;  // ↑ Up
        case 0x51: buttonState |= (1 << 5); break;  // ↓ Down
        case 0x50: buttonState |= (1 << 6); break;  // ← Left
        case 0x4F: buttonState |= (1 << 7); break;  // → Right
      }
    }

    void OnKeyUp(uint8_t mod, uint8_t key) override {
      switch (key) {
        case 0x04: buttonState &= ~(1 << 8); break;
        case 0x05: buttonState &= ~(1 << 0); break;
        case 0x1A: buttonState &= ~(1 << 9); break;
        case 0x1C: buttonState &= ~(1 << 1); break;
        case 0x28: buttonState &= ~(1 << 3); break;
        case 0x2A: buttonState &= ~(1 << 2); break;
        case 0x15: buttonState &= ~(1 << 10); break;
        case 0x0F: buttonState &= ~(1 << 11); break;
        case 0x52: buttonState &= ~(1 << 4); break; // ↑ Up
        case 0x51: buttonState &= ~(1 << 5); break; // ↓ Down
        case 0x50: buttonState &= ~(1 << 6); break; // ← Left
        case 0x4F: buttonState &= ~(1 << 7); break; // → Right
      }
    }
};

KbdRptParser Prs;

#define readLatch() ((PINC & (1 << LATCH_BIT)) ? HIGH : LOW )
#define readClock() ((PINC & (1 << CLOCK_BIT)) ? HIGH : LOW )

// Active LOW: pressed = 1 → output LOW (0)
#define sendButtonBit(sendState) \
  PORTC = (PORTC & (~(1 << DATA_BIT))) | ((!(sendState & 1)) << DATA_BIT); \
  sendState = (sendState >> 1)

#define waitForClockCycle() while (readClock() == HIGH); while (readClock() == LOW)

void setup() {
  pinMode(LATCH_PIN, INPUT);
  pinMode(CLOCK_PIN, INPUT);
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, HIGH);  // Idle state is HIGH (not pressed)

  Usb.Init();
  Keyboard.SetReportParser(0, &Prs);
}

void loop() {
  Usb.Task();  // Always process USB

    uint16_t sendState = buttonState;

    // Wait for LATCH to go LOW
    while (readLatch() == HIGH);

    // Send 12 button bits
    for (int i = 0; i < 12; i++) {
      sendButtonBit(sendState);
      waitForClockCycle();
    }

    // Send remaining 4 bits as HIGH (not pressed)
      PORTC |= (1 << DATA_BIT); // Output HIGH
}
