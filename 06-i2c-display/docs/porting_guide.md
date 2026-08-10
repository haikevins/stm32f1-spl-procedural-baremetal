# Porting Guide — 06-i2c-display

## 1. Changing the OLED Address

Change:

```c
BOARD_DISPLAY_I2C_ADDRESS_7BIT
```

Typical values are `0x3C` or `0x3D`.

Do not use an 8-bit shifted address in a constant documented as 7-bit.

## 2. Changing Bus Speed

Update:

```c
BOARD_DISPLAY_I2C_CLOCK_HZ
```

Verify:

- device supports the speed;
- pull-ups are strong enough;
- rise time is acceptable;
- actual PCLK1 is correct;
- logic analyzer confirms SCL.

## 3. Changing I2C Pins

If staying on I2C1, verify whether remap is required.

Update:

- GPIO port;
- SCL pin;
- SDA pin;
- GPIO clock;
- AFIO remap if used.

## 4. Changing OLED Controller

Keep the Board Display Bus.

Replace the ECUAL driver and adapt the Display Service only if the logical
drawing API must change.

## 5. Changing Resolution

Review together:

- width/height constants;
- page count;
- framebuffer size;
- address-window commands;
- clipping;
- progress/text layout.

Do not change only one dimension constant.

## 6. Adding a Reset Pin

Add reset mapping to BSP.

Expose a board reset operation or integrate it into board/display
initialization.

Keep the reset pin out of Application.

## 7. Changing Power-On Delay

Adjust the configured delay based on actual module requirements.

Do not remove the delay merely because one sample powers up quickly.

## 8. Porting to Another MCU

Preserve:

```text
Application -> Display Service -> SSD1306 ECUAL
```

Replace the Board Display Bus and low-level I2C implementation.

## 9. Verification Checklist

-  correct address;
-  SDA/SCL idle HIGH;
-  SCL frequency correct;
-  address ACK received;
-  command writes succeed;
-  framebuffer write succeeds;
-  no I2C error flags;
-  display orientation correct;
-  layer checker passes.

## 10. Logic Analyzer Checklist

Capture:

```text
START
address + W
ACK
control byte
payload
STOP
```

For data updates, verify control byte `0x40`.

For command sequences, verify `0x00`.

## 11. Common Pitfalls

- confusing 7-bit and shifted I2C address;
- no pull-ups;
- using push-pull instead of open-drain;
- swapped SDA/SCL;
- incorrect display resolution;
- omitting power-on delay;
- allowing an unbounded BUSY wait;
- letting Application include SSD1306 or I2C headers directly.
