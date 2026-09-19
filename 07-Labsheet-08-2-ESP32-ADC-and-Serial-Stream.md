# ใบงานการทดลองที่ 8.2
### การสตรีมสัญญาณแอนะล็อก ESP32 ออกพอร์ตสื่อสารอนุกรม (UART)

> **คำชี้แจง:** ในใบงานนี้ นักศึกษาจะเตรียมฝั่งฮาร์ดแวร์จริง โดยการต่อตัวต้านทานปรับค่าได้ (Potentiometer) เข้ากับขาแอนะล็อก (ADC) ของ ESP32 เขียนเฟิร์มแวร์เพื่ออ่านค่า กรองสัญญาณรบกวน และสตรีมข้อมูลผ่านสาย USB (UART) อย่างต่อเนื่องและเสถียร

---

##  วัตถุประสงค์การทดลอง (Objectives)
1. สามารถต่อวงจรตัวต้านทานปรับค่าได้ (Potentiometer) เข้ากับขาอินพุตแอนะล็อก (ADC) ของ ESP32 ได้อย่างถูกต้องและปลอดภัย
2. เข้าใจข้อจำกัดของวงจรแปลงสัญญาณแอนะล็อกเป็นดิจิทัล (ADC1 vs ADC2) บนบอร์ด ESP32
3. สามารถเขียนโปรแกรมภาษา C/C++ บน Arduino IDE เพื่ออ่านค่าและกรองสัญญาณรบกวน (Signal Smoothing) ได้
4. สามารถสตรีมข้อมูลเซนเซอร์ออกทางพอร์ตสื่อสารอนุกรม (Serial Port @ 115200 bps) เพื่อเตรียมป้อนให้แก่ Web Server ในใบงานถัดไป

---

## อุปกรณ์ที่ใช้ในการทดลอง (Hardware Components)
- บอร์ดไมโครคอนโทรลเลอร์ **ESP32** จำนวน 1 บอร์ด
- สายเชื่อมต่อ **USB Data Cable** (Micro-USB หรือ Type-C) จำนวน 1 เส้น
- **ตัวต้านทานปรับค่าได้ (Potentiometer)** ขนาด $10\text{ k}\Omega$ จำนวน 1 ตัว
- แผงต่อวงจร (Breadboard) และสายไฟจัมเปอร์ (Jumper Wires) 3 เส้น

---

## วงจรและการต่อสายฮาร์ดแวร์ (Schematic & Wiring)

```
        ESP32 Board                  Potentiometer (10k)
      +--------------+                 +-------------+
      |         3.3V |-----------------| ขา 1 (VCC)  |
      |          GND |-----------------| ขา 3 (GND)  |
      |      GPIO 34 |-----------------| ขา 2 (Wiper)| (ขากลางรับแรงดัน 0 - 3.3V)
      +--------------+                 +-------------+
```

### ⚠️ ข้อควรระวัง
1. **ห้ามต่อขา VCC ของ Potentiometer เข้ากับไฟ 5V (VIN)** เด็ดขาด! เพราะขา GPIO ของ ESP32 ทนแรงดันได้สูงสุดเพียง **3.3V** หากต่อ 5V อาจทำให้ชิปเสียหายถาวร
2. **ทำไมจึงเลือกใช้ GPIO 34**
   - บน ESP32 ขา ADC แบ่งเป็น 2 กลุ่ม คือ **ADC1** (GPIO 32 - 39) และ **ADC2** (GPIO 0, 2, 4, 12-15, 25-27)
   - **ADC2** จะถูกระบบ Wi-Fi ยึดครองการใช้งาน เมื่อเปิดใช้งาน Wi-Fi (เช่น ในสัปดาห์ถัดไป เมื่อต่อ MQTT) จะอ่านค่าไม่ได้
   - ดังนั้น การเลือกใช้ **GPIO 34 (อยู่ในกลุ่ม ADC1)** จึงเป็นมาตรฐานที่ปลอดภัยที่สุดและทำงานได้เสถียรเสมอ

---

## ขั้นตอนการทดลอง (Step-by-Step Activities)

### กิจกรรมที่ 1: การเขียนเฟิร์มแวร์อ่านค่า ADC ด้วย ESP-IDF v6.x

ใน ESP-IDF v6.x การอ่านค่าสัญญาณแอนะล็อกจะใช้ไดรเวอร์ **`esp_adc/adc_oneshot.h`** ซึ่งมีประสิทธิภาพสูงและปลอดภัยต่อหน่วยความจำ

>  **ข้อกำหนดสำคัญด้านการส่งงาน (Project Isolation):**  
> ให้สร้างโปรเจกต์นี้แยกไว้ในโฟลเดอร์ **`Lab8-2/ESP32_ADC_Stream`** ห้ามเขียนทับกับใบงานอื่น

1. เปิด Terminal หรือคอนเทนเนอร์ ESP-IDF v6.x แล้วสร้างโปรเจกต์ใหม่ในโฟลเดอร์ `Lab8-2`
   ```bash
   # สร้างโฟลเดอร์ Lab8-2 และเข้าไปด้านใน
   mkdir -p Lab8-2 && cd Lab8-2

   # สร้างโปรเจกต์ชื่อ ESP32_ADC_Stream
   idf.py create-project ESP32_ADC_Stream
   cd ESP32_ADC_Stream
   ```


กรณีใช้ docker เมื่ออยู่ในโฟลเดอร์ Lab8-2 แล้ว ให้รันคำสั่งต่อไปนี้ เพื่อสร้าง project  ใหม่
```powershell
docker run --rm --mount "type=bind,source=$((Get-Location).Path),target=/workspace" -w /workspace espressif/idf:release-v6.1  idf.py create-project ESP32_ADC_Stream
```


2. เปิดไฟล์ `main/main.c` แล้วเขียนโค้ดอ่านค่า ADC ดังนี้:

```c
// ============================================================================
// ใบงานที่ 8.2: ESP32 Potentiometer Serial Stream (ESP-IDF v6.x)
// ============================================================================
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

// ----------------------------------------------------------------------------
// การกำหนดขา ADC ตามประเภทชิป:
// - ESP32 Classic (NodeMCU-32S): ขา GPIO 34 คือ ADC_UNIT_1, ADC_CHANNEL_6
// - ESP32-C6: ขา GPIO 4 คือ ADC_UNIT_1, ADC_CHANNEL_4 (หรือ GPIO 2: ADC_CHANNEL_2)
// ----------------------------------------------------------------------------
#if CONFIG_IDF_TARGET_ESP32C6
#define POT_ADC_CHANNEL    ADC_CHANNEL_4   // GPIO 4 บน ESP32-C6
#else
#define POT_ADC_CHANNEL    ADC_CHANNEL_6   // GPIO 34 บน ESP32 WROOM
#endif

void app_main(void)
{
    printf("\n[SYSTEM] ESP-IDF v6.x Potentiometer Stream Starting...\n");

    // 1. สร้างและตั้งค่า ADC Unit 1
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // 2. กำหนดค่าความละเอียด 12-bit (0-4095) และอัตราขยายสัญญาณ (Attenuation 12dB สำหรับ 0-3.3V)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_ADC_CHANNEL, &config));

    printf("[SYSTEM] ADC Initialized. Streaming raw values @ 115200 bps...\n");

    int raw_val = 0;
    while (1) {
        // 3. อ่านค่า ADC แบบ Oneshot
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_ADC_CHANNEL, &raw_val));

        // 4. สตรีมตัวเลขเดี่ยวออกทาง UART (stdout) ปิดท้ายด้วย \n
        // รูปแบบ: ส่งตัวเลขบรรทัดละค่า เพื่อให้ฝั่ง C# อ่านง่ายที่สุด
        printf("%d\n", raw_val);

        // หน่วงเวลา 100ms ด้วย FreeRTOS Task Delay (10 ครั้งต่อวินาที)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

3. สั่งคอมไพล์และแฟลชลงบอร์ด ESP32 ผ่านคำสั่ง
   ```bash
   # หากใช้บอร์ด ESP32 ทั่วไป (หรือระบุ esp32c6 หากใช้บอร์ด C6)
   idf.py set-target esp32

   # คอมไพล์ แฟลช และเปิดหน้าจอ Monitor (แทน COMx ด้วยพอร์ตจริงของบอร์ด เช่น COM3)
   idf.py -p COMx flash monitor
   ```

กรณีใช้ docker ให้รันคำสั่งต่อไปนี้ เพื่อ build

 
```powershell
## cd เข้าไปใน ESP32_ADC_Stream
cd ESP32_ADC_Stream

## build ใน docker
docker run --rm --mount "type=bind,source=$((Get-Location).Path),target=/workspace" -w /workspace espressif/idf:release-v6.1  idf.py build 
```

Flash & Monitor ไปยัง ESP32 ตาม port ที่กำหนด (ในตัวอย่างนี้คือ COM24)

```powershell
python -m esptool -p COM24 --chip esp32 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 2MB --flash_freq 40m 0x1000 build\bootloader\bootloader.bin 0x8000 build\partition_table\partition-table.bin 0x10000 build\esp32_adc_stream.bin && idf -p COM24 monitor
```

---

### กิจกรรมที่ 2: ตรวจสอบสัญญาณผ่าน ESP-IDF Monitor

1. ในหน้าจอ **ESP-IDF Monitor**
   - สังเกตตัวเลขที่วิ่งขึ้นมาบนหน้าจอเทอร์มินัล
     - เมื่อหมุน Volume ไปทางซ้ายสุด ค่าควรเข้าใกล้ `0`
     - เมื่อหมุน Volume ไปทางขวาสุด ค่าควรเข้าใกล้ `4095`
2. **วิธีออกจากหน้าจอ Monitor**
   - กดปุ่ม `Ctrl + ]` (กดปุ่ม Control ค้างไว้แล้วกดเครื่องหมายก้ามปูปิด หรือปุ่ม 'ล ลิง" บนแป้นพิมพ์ภาษาไทย) เพื่อออกจาก Monitor และปลดล็อกพอร์ต COM
     ถ้าไม่ปลดล็อก COM port จะใช้งานใน Kestrel  ไม่ได้
---

### กิจกรรมที่ 3 เสริมเสถียรภาพด้วยการกรองสัญญาณ (Exponential Moving Average Filter)

ในสภาพแวดล้อมจริง สัญญาณแอนะล็อกจากตัวต้านทานปรับค่าได้อาจมีสัญญาณรบกวน (Noise) สั่นไหวเล็กน้อย เราสามารถปรับปรุงลูป `while (1)` ในไฟล์ `main/main.c` ให้มีการกรองสัญญาณแบบ EMA

```c
    int raw_val = 0;
    float filtered_val = 0.0f;
    const float alpha = 0.25f; // ค่าสัมประสิทธิ์การกรอง (0.0 - 1.0)

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_ADC_CHANNEL, &raw_val));

        // การกรองแบบ Exponential Moving Average (EMA)
        filtered_val = (alpha * raw_val) + ((1.0f - alpha) * filtered_val);

        // ส่งค่าที่ผ่านการกรองแล้วออกทาง Serial
        printf("%d\n", (int)filtered_val);

        // สตรีมเร็วขึ้นที่ 20 ครั้งต่อวินาที (20 Hz)
        vTaskDelay(pdMS_TO_TICKS(50));
    }
```

แฟลชโปรแกรมใหม่อีกครั้ง และทดสอบหมุนดูว่าตัวเลขที่ออกมานิ่งและนุ่มนวลขึ้นอย่างเห็นได้ชัด

---

#### [Checkpoint 2.1 ทดสอบความเข้าใจ]
1. ใน ESP-IDF v6.x ฟังก์ชัน `adc_oneshot_read()` คืนค่าผลลัพธ์เป็นตัวเลขแบบใด และมีช่วงค่าต่ำสุด-สูงสุดเท่าใดเมื่อตั้งค่าเป็น `ADC_BITWIDTH_12`
   - **คำตอบ:**  คืนค่าผลลัพธ์เป็น **ตัวเลขจำนวนเต็ม (Integer)** ของแรงดันไฟฟ้าดิบ โดยมีช่วงค่าตั้งแต่ **0 ถึง 4095** (เนื่องจากความละเอียด 12-bit จะสามารถแบ่งระดับการวัดได้ 4,096 ระดับ)
1. ทำไมเราจึงต้องกดปุ่ม `Ctrl + ]` เพื่อปิด **ESP-IDF Monitor** ก่อนที่เราจะรันโปรแกรมฝั่ง C# Kestrel ในใบงานถัดไป
   *(คำใบ้: หากโปรเซส idf.py monitor ยังเปิดค้างอยู่ จะเกิดอะไรขึ้นกับพอร์ต COM)*
   - **คำตอบ:** เพราะพอร์ตสื่อสาร (COM Port) ของระบบปฏิบัติการสามารถเปิดใช้งานได้เพียง **1 โปรแกรมต่อ 1 เวลาเท่านั้น** หากเราไม่ปิด ESP-IDF Monitor ระบบจะยังคงจอง (Hold) พอร์ตนั้นไว้ ทำให้เมื่อโปรแกรมฝั่ง C# พยายามจะเข้าไปอ่านข้อมูลจากพอร์ตเดียวกัน จะเกิดข้อผิดพลาด (Access Denied / Port in use) และไม่สามารถดึงข้อมูลได้

---

## ภารกิจท้าทาย (Micro-Challenge) (ประสบการณ์จากใบงาน LDR)
ให้นักศึกษาทดลองเปลี่ยนตัวต้านทานปรับค่าได้เป็น **LDR (Light Dependent Resistor)** ร่วมกับตัวต้านทาน $10\text{ k}\Omega$ แบ่งแรงดัน

 ทดลองใช้ไฟฉายจากโทรศัพท์มือถือส่อง และเอามือปิดบังแสง สังเกตค่า ADC บนหน้าจอเทอร์มินัลว่าเปลี่ยนแปลงอย่างไร
- **เมื่อเอามือปิดบังแสง (สภาวะมืด):** ค่า ADC บนหน้าจอเทอร์มินัลมีค่าลดลงต่ำสุด (อยู่ในช่วงประมาณ 70 - 300) เนื่องจากตัวรับแสง LDR ได้รับแสงน้อย ทำให้มีความต้านทานไฟฟ้าสูงขึ้น กระแสไฟจึงไหลผ่านวงจรแบ่งแรงดันเข้าสู่ขา GPIO 34 ได้น้อยลง
- **เมื่อใช้ไฟฉายส่อง (สภาวะสว่างมาก):** ค่า ADC พุ่งสูงขึ้นอย่างชัดเจน (ไปถึงช่วงประมาณ 3,800) ซึ่งเข้าใกล้ค่าสูงสุดของระบบ 12-bit (4,095) เนื่องจากเมื่อ LDR ได้รับแสงสว่าง ความต้านทานจะลดลงอย่างรวดเร็ว ทำให้กระแสไฟ 3.3V ไหลผ่านเข้าสู่ขารับสัญญาณได้มากขึ้น
- **เมื่ออยู่ในสภาพแสงปกติ (ไฟห้อง):** ค่า ADC จะรักษาระดับคงที่อยู่ในช่วงกลาง (ประมาณ 2,800 - 3,000)
- **ผลจากการใช้ EMA Filter:** จากการสังเกตเพิ่มเติมพบว่า เมื่อมีการเปลี่ยนแปลงแสงอย่างฉับพลัน (เช่น การเปิดไฟฉายส่องทันที) ตัวเลข ADC จะไม่กระโดดพรวดพราด แต่จะค่อยๆ ไต่ระดับค่าขึ้นหรือลงอย่างนุ่มนวลและเสถียร ซึ่งเป็นผลมาจากการกรองสัญญาณรบกวนด้วยสมการ Exponential Moving Average
บันทึกภาพถ่ายการต่อวงจรและภาพหน้าจอ Monitor ลงในรายงานผลการทดลอง
![[Pasted image 20260908145030.png]]![[Pasted image 20260908145051.png]]