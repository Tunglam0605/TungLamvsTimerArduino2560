  /*
    Arduino2560_TIM - Test điều khiển xe robot bằng tay cầm PS2
    Tác giả: Tùng Lâm Automatic

    Sơ đồ chân đấu nối (Arduino Mega 2560):
    - PS2 gamepad:
      PS2_DAT -> D28 (PA6)
      PS2_CMD -> D26 (PA4)
      PS2_SEL/ATT -> D24 (PA2)
      PS2_CLK -> D22 (PA0)
    - Driver động cơ (4 motor, 2 chân hướng/motor):
      PORTC (D37..D30) dùng cho chân IN hướng:
        PC0 D37 -> M1_IN1
        PC1 D36 -> M1_IN2
        PC2 D35 -> M2_IN1
        PC3 D34 -> M2_IN2
        PC4 D33 -> M3_IN1
        PC5 D32 -> M3_IN2
        PC6 D31 -> M4_IN1
        PC7 D30 -> M4_IN2
      PWM từ Timer3/4:
        OC4A PH3 D6 -> M1_PWM
        OC4B PH4 D7 -> M2_PWM
        OC4C PH5 D8 -> M3_PWM
        OC3A PE3 D5 -> M4_PWM

    Lưu ý: Động cơ và Arduino phải chung GND.
    Timer3/4 set Fast PWM 8-bit, prescaler /8 (~7.8 kHz).
  */
  #include "PS2X_lib.h" // Thư viện giao tiếp tay cầm PS2.

  #define PS2_DAT 28 // Chân DATA từ tay cầm về Arduino (input).
  #define PS2_CMD 26 // Chân CMD từ Arduino ra tay cầm (output).
  #define PS2_SEL 24 // Chân ATT/SEL (chip select).
  #define PS2_CLK 22 // Chân CLK đồng bộ xung.

  int LX, LY, RX, RY;      // Giá trị joystick lần đọc 1.
  int LX2, LY2, RX2, RY2;  // Giá trị joystick lần đọc 2.
  PS2X ps2x;               // Đối tượng điều khiển PS2.
  int error = 0;           // Mã lỗi cấu hình tay cầm.
  byte type = 0;           // Loại tay cầm đọc được.
  byte vibrate = 0;        // Mức rung gửi về tay cầm.
  int mode = 1;            // Chế độ điều khiển (chưa dùng).
  int pre;                 // Nhớ hướng trước đó để phanh ABS giả.
  int speed = 150;         // PWM mặc định (0..255).
  int speed1, speed2;      // Biến dự phòng, hiện chưa dùng.
  // Hàm khởi tạo: chạy một lần khi bật nguồn/reset.
  void setup()
    {
      Serial.begin(9600); // Mở Serial để debug.
      // Cấu hình PC0..PC7 làm OUTPUT cho 8 chân IN điều hướng.
      DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5) | (1 << PC6) | (1 << PC7) ;
      // Timer4: Fast PWM 8-bit, bật OC4A/OC4B/OC4C để xuất PWM.
      TCCR4A = (1 << WGM40) | (1 << COM4A1) | (1 << COM4B1) | (1 << COM4C1);
      // Timer4: chọn Fast PWM 8-bit + prescaler /8.
      TCCR4B = (1 << WGM42) | (1 << CS41);
      // Timer3: Fast PWM 8-bit, bật OC3A.
      TCCR3A = (1 << WGM30) | (1 << COM3A1);
      // Timer3: chọn Fast PWM 8-bit + prescaler /8.
      TCCR3B = (1 << WGM32) | (1 << CS31);
      OCR3A = 0; // Duty ban đầu motor 4 = 0%.
      OCR4A = 0; // Duty ban đầu motor 1 = 0%.
      OCR4B = 0; // Duty ban đầu motor 2 = 0%.
      OCR4C = 0; // Duty ban đầu motor 3 = 0%.
      // Cấu hình các chân PWM làm OUTPUT.
      DDRH |= (1 << PH3) | (1 << PH4) | (1 << PH5);
      DDRE |= (1 << PE3);
      Serial.print("Search Controller.."); // Thông báo bắt đầu tìm tay cầm.
      do {
        // Cấu hình tay cầm: thứ tự chân CLK, CMD, SEL, DAT.
        // Tham số true, true: bật chế độ analog + rung.
        error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, true, true);
        if (error == 0) { // Nếu cấu hình thành công.
          Serial.println("\nConfigured successful ");
          break; // Thoát vòng lặp tìm tay cầm.
        } else {
          Serial.println("No"); // Báo chưa tìm thấy.
          STOP(); // Dừng động cơ để an toàn.
          delay(100); // Chờ rồi thử lại.
        }
      } while (1); // Lặp vô hạn cho đến khi có tay cầm.
      type = ps2x.readType(); // Đọc loại tay cầm đang kết nối.
      switch (type) {
        case 0: Serial.println("Unknown Controller type found "); break; // Không xác định.
        case 1: Serial.println("DualShock Controller found "); break; // DualShock thường.
        case 2: Serial.println("GuitarHero Controller found "); break; // GuitarHero.
        case 3: Serial.println("Wireless Sony DualShock Controller found "); break; // DualShock không dây.
      }
      ps2x.read_gamepad(true, 200); // Đọc lần đầu, test rung.
      delay(200); // Chờ ổn định.
      ps2x.read_gamepad(false, 200); // Đọc lại, tắt rung.
      delay(300); // Chờ thêm để ổn định.
      ps2x.read_gamepad(true, 200); // Đọc lần nữa để chắc chắn.
      delay(200); // Chờ ổn định.
    }
  // Vòng lặp chính: chạy liên tục sau setup().
  void loop()
    {
      ps2x.read_gamepad(false, vibrate); // Đọc dữ liệu tay cầm liên tục.
      dk_nut_bam(); // Xử lý nút tăng/giảm tốc độ.
      Joystick1(); // Xử lý joystick kiểu 1.
      Joystick2(); // Xử lý joystick kiểu 2.
    }
      // Hàm chạy tiến.
      void Tien()
        {
          // Cài đặt hướng tiến: các bánh quay cùng chiều tiến.
          PORTC |=  ( 1 << PORTC0) | (1 << PORTC2) | (1 << PORTC4) | (1 << PORTC6);
          // Tắt các chân ngược chiều để tránh chập H-bridge.
          PORTC &= ~((1 << PORTC1) | (1 << PORTC3) | (1 << PORTC5) | (1 << PORTC7));
          PWM(); // Gọi hàm cập nhật PWM theo biến speed.
          // Serial.println("Tiến");
        }

      // Hàm quay trái.
      void Trai()
        {
          // Quay trái: set hướng bánh phù hợp để xoay thân xe.
          PORTC |=  ( 1 << PORTC0) | (1 << PORTC2) | (1 << PORTC5) | (1 << PORTC7);
          // Reset các chân ngược lại.
          PORTC &= ~((1 << PORTC1) | (1 << PORTC3) | (1 << PORTC4) | (1 << PORTC6));
          PWM(); // Cập nhật PWM.
          // Serial.println("Quay trái");
        }
      // Hàm quay phải.
      void Phai()
        {
          // Quay phải: set hướng bánh ngược với quay trái.
          PORTC |=  ( 1 << PORTC1) | (1 << PORTC3) | (1 << PORTC4) | (1 << PORTC6);
          // Reset các chân ngược lại.
          PORTC &= ~((1 << PORTC0) | (1 << PORTC2) | (1 << PORTC5) | (1 << PORTC7));
          PWM(); // Cập nhật PWM.
          // Serial.println("Quay phải");
        }

      // Hàm chạy lùi.
      void Lui()
        {
          // Lùi: đảo chiều so với tiến.
          PORTC |=  ( 1 << PORTC1) | (1 << PORTC3) | (1 << PORTC5) | (1 << PORTC7);
          // Reset các chân ngược lại.
          PORTC &= ~((1 << PORTC0) | (1 << PORTC2) | (1 << PORTC4) | (1 << PORTC6));
          PWM(); // Cập nhật PWM.
          // Serial.println("Lùi");
        }

      // Hàm dừng tất cả động cơ.
      void STOP()
        {
          // Tắt toàn bộ chân điều hướng.
          PORTC &= ~((1 << PORTC1) | (1 << PORTC3) | (1 << PORTC5) | (1 << PORTC7) | ( 1 << PORTC0) | (1 << PORTC2) | (1 << PORTC4) | (1 << PORTC6));
          OCR4A = 0; // Tắt PWM motor 1.
          OCR4B = 0; // Tắt PWM motor 2.
          OCR4C = 0; // Tắt PWM motor 3.
          OCR3A = 0; // Tắt PWM motor 4.
          // Serial.println("STOP");
        }
      // Hàm đi ngang trái (mecanum/omni).
      void N_Trai()
        {
          // Đi ngang trái (mecanum/omni): set hướng bánh theo trục ngang.
          PORTC |=  ( 1 << PORTC0) | (1 << PORTC3) | (1 << PORTC4) | (1 << PORTC7);
          // Reset các chân ngược lại.
          PORTC &= ~((1 << PORTC1) | (1 << PORTC2) | (1 << PORTC5) | (1 << PORTC6));
          PWM(); // Cập nhật PWM.
          // Serial.println("Ngang trái");
        }
      // Hàm đi ngang phải (mecanum/omni).
      void N_Phai()
        {
          // Đi ngang phải (mecanum/omni).
          PORTC |=  ( 1 << PORTC1) | (1 << PORTC2) | (1 << PORTC5) | (1 << PORTC6);
          // Reset các chân ngược lại.
          PORTC &= ~((1 << PORTC0) | (1 << PORTC3) | (1 << PORTC4) | (1 << PORTC7));
          PWM(); // Cập nhật PWM.
          // Serial.println("Ngang phải");
        }

      // ABS: phanh nhanh bằng cách xung ngược trong thời gian ngắn.
        // Hàm ABS giả: phanh nhanh bằng đảo hướng ngắn.
        void ABS()
        {
        if(pre == 0) { STOP();            pre = 0; } // Đang đứng yên -> dừng hẳn.
        if(pre == 1) { Lui();   delay(30); pre = 0; } // Đang tiến -> đảo lùi 30 ms.
        if(pre == 2) { Tien();  delay(30); pre = 0; } // Đang lùi -> đảo tiến 30 ms.
        if(pre == 5) { Trai();  delay(15); pre = 0; } // Đang quay phải -> đảo trái 15 ms.
        if(pre == 6) { Phai();  delay(15); pre = 0; } // Đang quay trái -> đảo phải 15 ms.
        if(pre == 7) { N_Phai();delay(15); pre = 0; } // Đang ngang trái -> đảo ngang phải 15 ms.
        if(pre == 8) { N_Trai();delay(15); pre = 0; } // Đang ngang phải -> đảo ngang trái 15 ms.
        }

      // Hàm cập nhật PWM cho 4 kênh.
      void PWM()
        {
          OCR4A = speed; // Gán PWM cho motor 1 (OC4A).
          OCR4B = speed; // Gán PWM cho motor 2 (OC4B).
          OCR4C = speed; // Gán PWM cho motor 3 (OC4C).
          OCR3A = speed; // Gán PWM cho motor 4 (OC3A).
        }
    // Hàm xử lý nút bấm D-pad để chỉnh tốc độ.
    void dk_nut_bam()
      {
      if (ps2x.ButtonPressed(PSB_PAD_UP)) { // Nhấn D-pad lên để tăng tốc.
        speed += 10; // Tăng tốc độ thêm 10.
        if(speed >= 255) speed = 255; // Giới hạn tối đa 255.
        }
      if (ps2x.ButtonPressed(PSB_PAD_DOWN)) { // Nhấn D-pad xuống để giảm tốc.
        speed -= 10; // Giảm tốc độ 10.
        if(speed <= 100) speed = 100; // Giới hạn tối thiểu 100.
        }
      }
    // Hàm đọc joystick kiểu 2 (LX2/LY2/RX2/RY2).
    void Joystick2()
      {
      LX2 = ps2x.Analog(PSS_LX) -128 ; // Đọc cần trái X, dời về [-128..127].
      LY2 = ps2x.Analog(PSS_LY) -127;  // Đọc cần trái Y, dời về [-127..128].
      RX2 = ps2x.Analog(PSS_RX) -128;  // Đọc cần phải X, dời về [-128..127].
      RY2 = ps2x.Analog(PSS_RY) -127;  // Đọc cần phải Y, dời về [-127..128].
        // Serial.print(LX2); // Debug giá trị LX2.
        // Serial.print("  "); // In khoảng cách.
        // Serial.print(LY2); // Debug giá trị LY2.
        // Serial.print("  "); // In khoảng cách.
        // Serial.print(RX2); // Debug giá trị RX2.
        // Serial.print("  "); // In khoảng cách.
        // Serial.print(RY2); // Debug giá trị RY2.
      // Giá trị cần PS2 về gần 0 khi thả tay.
      if (LX2 ==    0 && LY2 ==    0 && RX2 == 0 && RY2 == 0) { ABS(); } // Thả tay -> phanh.
      if (LX2 ==    0 && LY2 == -127) { Tien();  pre = 1; } // Cần trái lên -> tiến.
      if (LX2 ==    0 && LY2 ==  128) { Lui();   pre = 2; } // Cần trái xuống -> lùi.
      if (LX2 ==  127 && LY2 ==    0) { N_Phai();pre = 4; } // Cần trái phải -> ngang phải.
      if (LX2 == -128 && LY2 ==    0) { N_Trai();pre = 3; } // Cần trái trái -> ngang trái.
      if (RX2 ==  127 && RY2 ==    0) { Phai();  pre = 5; } // Cần phải phải -> quay phải.
      if (RX2 == -128 && RY2 ==    0) { Trai();  pre = 6; } // Cần phải trái -> quay trái.
      if (RX2 ==    0 && RY2 == -127) { Tien();  pre = 1; } // Cần phải lên -> tiến.
      if (RX2 ==    0 && RY2 ==  128) { Lui();   pre = 2; } // Cần phải xuống -> lùi.
      delay(10); // Trễ ngắn để ổn định điều khiển.
      }
    // Hàm đọc joystick kiểu 1 (LX/LY/RX/RY).
    void Joystick1()
      {
      LX = ps2x.Analog(PSS_LX) -127 ; // Đọc cần trái X, dời về [-127..128].
      LY = ps2x.Analog(PSS_LY) -128;  // Đọc cần trái Y, dời về [-128..127].
      RX = ps2x.Analog(PSS_RX) -127;  // Đọc cần phải X, dời về [-127..128].
      RY = ps2x.Analog(PSS_RY) -128;  // Đọc cần phải Y, dời về [-128..127].
      if (LX ==    0 && LY ==    0  && RX == 0 && RY == 0) { ABS(); } // Thả tay -> phanh.
      if (LX ==    0 && LY == -128) { Tien();    pre = 1;   } // Cần trái lên -> tiến.
      if (LX ==    0 && LY ==  127) { Lui();     pre = 2;   } // Cần trái xuống -> lùi.
      if (LX ==  128 && LY ==    0) { N_Phai();  pre = 4;   } // Cần trái phải -> ngang phải.
      if (LX == -127 && LY ==    0) { N_Trai();  pre = 3;   } // Cần trái trái -> ngang trái.
      if (RX ==  128 && RY ==    0) { Phai();    pre = 5;   } // Cần phải phải -> quay phải.
      if (RX == -127 && RY ==    0) { Trai();    pre = 6;   } // Cần phải trái -> quay trái.
      if (RX ==    0 && RY == -128) { Tien();    pre = 1;   } // Cần phải lên -> tiến.
      if (RX ==    0 && RY ==  127) { Lui();     pre = 2;   } // Cần phải xuống -> lùi.
      delay(10); // Trễ ngắn để ổn định điều khiển.
      }
