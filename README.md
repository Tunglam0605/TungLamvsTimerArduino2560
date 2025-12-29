<div align="center">
  <h1>Arduino2560_TIM</h1>
  <p><strong>Test dieu khien xe robot bang tay cam PS2</strong></p>
  <p>PWM bang Timer3/Timer4 + dieu khien truc tiep bang thanh ghi AVR</p>
  <p>Tac gia: Tung Lam Automatic</p>
  <p>
    <img src="https://img.shields.io/badge/Board-Arduino%20Mega%202560-0B7C3E" />
    <img src="https://img.shields.io/badge/Input-PS2%20Gamepad-1D4ED8" />
    <img src="https://img.shields.io/badge/PWM-Timer3%2FTimer4-F97316" />
    <img src="https://img.shields.io/badge/Language-C%2B%2B-111827" />
  </p>
</div>

---

## Gioi thieu
Day la code test dieu khien xe robot bang tay cam PS2. Du lieu tay cam duoc doc qua thu vien PS2X, dieu khien huong bang PORTC va phat xung PWM bang Timer3/Timer4. Toan bo PWM va chieu quay duoc set truc tiep tren thanh ghi AVR de dam bao toc do va do on dinh.

## Tinh nang
- Doc tay cam PS2 (analog + D-pad).
- Dieu khien 4 dong co DC (2 chan huong/motor).
- PWM 8-bit tu Timer3/Timer4 (~7.8 kHz).
- ABS don gian (phanh bang xung nguoc ngan).
- Tang/giam toc do bang D-pad.

## So do chan dau noi (Arduino Mega 2560)

### PS2 gamepad
| PS2 Pin | Mega 2560 | Ghi chu |
| --- | --- | --- |
| DAT | D28 (PA6) | Data in |
| CMD | D26 (PA4) | Command out |
| ATT/SEL | D24 (PA2) | Chip select |
| CLK | D22 (PA0) | Clock |
| VCC | 5V (hoac 3.3V tuy module) | Nguon |
| GND | GND | Chung mass |

### Driver dong co (4 motor)
| Cong | Mega 2560 | Ghi chu |
| --- | --- | --- |
| M1_IN1 | D37 (PC0) | Huong 1 |
| M1_IN2 | D36 (PC1) | Huong 2 |
| M2_IN1 | D35 (PC2) | Huong 1 |
| M2_IN2 | D34 (PC3) | Huong 2 |
| M3_IN1 | D33 (PC4) | Huong 1 |
| M3_IN2 | D32 (PC5) | Huong 2 |
| M4_IN1 | D31 (PC6) | Huong 1 |
| M4_IN2 | D30 (PC7) | Huong 2 |
| M1_PWM | D6 (PH3 / OC4A) | PWM |
| M2_PWM | D7 (PH4 / OC4B) | PWM |
| M3_PWM | D8 (PH5 / OC4C) | PWM |
| M4_PWM | D5 (PE3 / OC3A) | PWM |

> Luu y: Dong co va Arduino phai chung GND. Neu dung driver L298N/L293D hoac module cong suat khac, hay map chan IN/PWM tuong ung.

## Timer / PWM
Timer3 va Timer4 duoc cau hinh Fast PWM 8-bit, prescaler /8. Tan so PWM xap xi:

```
F_PWM = 16 MHz / 8 / 256 ~= 7.8 kHz
```

PWM duoc cap nhat qua OCR3A, OCR4A, OCR4B, OCR4C.

## Dieu khien
- Can trai/phai: day het huong -> tien/lui/quay/di ngang.
- Tha tay can -> ABS (phanh ngan).
- D-pad Up: tang toc do (+10).
- D-pad Down: giam toc do (-10).

## Thu vien can thiet
- PS2X_lib (PS2X Library by Bill Porter).

## Cach nap
1. Mo `Arduino2560_TIM/Arduino2560_TIM.ino` trong Arduino IDE.
2. Cai thu vien PS2X_lib.
3. Chon board: `Arduino Mega 2560`.
4. Chon cong COM phu hop va Upload.

## Cau truc thu muc
- `Arduino2560_TIM/Arduino2560_TIM.ino` - code chinh.

## Ghi chu an toan
- Su dung nguon rieng cho dong co va chung GND voi Arduino.
- Kiem tra muc dien ap cua tay cam PS2/module thu.

## Tac gia
Tung Lam Automatic

