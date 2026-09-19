# ใบงานการทดลองที่ 8.4 (Labsheet 8.4)
#### แดชบอร์ดมาตรวัดความเร็ว SVG และสนามทดลองสร้างสรรค์ (Creative Playground)

> **คำชี้แจง:** ในใบงานสุดท้ายของสัปดาห์นี้ นักศึกษาจะได้นำทักษะทั้งหมดมาสร้าง **"หน้าปัดมาตรวัดความเร็ว (Speedometer Gauge) ด้วย Pure SVG"** แสดงผลบนเว็บเบราว์เซอร์ โดยไม่ต้องพึ่งพาไลบรารีภายนอก เมื่อหมุน Volume บนบอร์ด ESP32 เข็มไมล์จะกวาดตามมืออย่างนุ่มนวล และปิดท้ายด้วย **Creative Sandbox** ให้เลือกปรับแต่งวิดเจ็ตในสไตล์ของตนเอง

---

## วัตถุประสงค์การทดลอง (Objectives)
1. สามารถกำหนดค่า Kestrel ให้ทำหน้าที่เสิร์ฟไฟล์หน้าเว็บ (Static Files / Single Page App) ได้
2. เข้าใจการเขียนกราฟิกเวกเตอร์สองมิติด้วยแท็ก `<svg>` สำหรับสร้างหน้าปัดเครื่องมือวัด
3. สามารถเขียน JavaScript (`fetch` API) ทำงานแบบ Polling ดึงข้อมูลเซนเซอร์มาอัปเดตหน้าจอแบบต่อเนื่อง
4. สามารถแปลงสเกลเชิงเส้น (Linear Mapping) จากค่า ADC สู่มุมการหมุนของเข็มไมล์ด้วย CSS Transform ได้
5. ได้ใช้ความคิดสร้างสรรค์ในการปรับแต่งหน้าปัด เช่น ปรับธีม, สร้าง VU Meter หรือหน้าจอ 7-Segment Display

---

##  เครื่องมือและสิ่งที่ต้องเตรียม (Prerequisites)
- บอร์ด ESP32 พร้อมสาย USB ที่ต่อวงจร Potentiometer เรียบร้อยแล้ว
- เว็บเบราว์เซอร์ที่รองรับ HTML5 / SVG (Google Chrome, Edge, Safari, Firefox)

> **ข้อกำหนดสำคัญด้านการส่งงาน (Project Isolation & Anti-Cheating):**  
> ในใบงานสุดท้ายที่ 8.4 นี้ ให้นักศึกษาจัดเตรียมโฟลเดอร์แยกต่างหากเป็น **`Lab8-4`** อย่างชัดเจน:
> 1. **ฝั่ง ESP32:** โฟลเดอร์ `Lab8-4/ESP32_ADC_Stream` (คัดลอกหรือสร้างใหม่จากแล็บก่อนหน้า)
> 2. **ฝั่ง Kestrel Dashboard:** โฟลเดอร์ `Lab8-4/Kestrel_SVG_Dashboard`

---

## ขั้นตอนการทดลอง (Step-by-Step Activities)

### กิจกรรมที่ 1: เตรียมโปรเจกต์ Kestrel_SVG_Dashboard และเปิดใช้งาน Static File Server

1. เตรียมโฟลเดอร์สำหรับ Lab 8.4 โดยสร้างโปรเจกต์ใหม่ (หรือคัดลอกโครงสร้างจาก `Lab8-3/Kestrel_Serial_Gateway` มาต่อยอด):
   ```bash
   # หากสร้างใหม่
   mkdir -p Lab8-4/Kestrel_SVG_Dashboard && cd Lab8-4/Kestrel_SVG_Dashboard
   dotnet new web -o .
   dotnet add package System.IO.Ports

   # สร้างโฟลเดอร์ wwwroot สำหรับเก็บไฟล์เว็บ HTML/CSS/SVG
   mkdir wwwroot
   ```

2. ในไฟล์ `Program.cs` ให้ตรวจเช็คว่ามีโค้ด State Store และ Background Worker จากใบงานที่ 8.3 ครบถ้วน จากนั้นเพิ่มคำสั่ง `app.UseFileServer();` ก่อนบรรทัด `app.Run();`:
   ```csharp
   // เปิดใช้งานการเสิร์ฟไฟล์สถิต (HTML, CSS, JS) ใน wwwroot และเปิด index.html อัตโนมัติ
   app.UseFileServer();

   app.Run();
   ```

---

### กิจกรรมที่ 2: สร้างหน้าเว็บและหน้าปัด Speedometer SVG

1. สร้างไฟล์ใหม่ชื่อ `index.html` ไว้ด้านในโฟลเดอร์ `wwwroot/index.html`
2. ใส่โค้ด HTML + SVG + JavaScript ต่อไปนี้ลงไป:

```html
<!DOCTYPE html>
<html lang="th">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IoT Interactive Gateway Dashboard</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            background: radial-gradient(circle at center, #1b263b 0%, #0d1b2a 100%);
            color: #e0e1dd;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 24px;
            padding: 30px;
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.5);
            text-align: center;
            max-width: 500px;
            width: 100%;
        }
        h1 { font-size: 1.5rem; margin-bottom: 5px; color: #00f2fe; }
        .subtitle { font-size: 0.85rem; color: #778da9; margin-bottom: 25px; }
        
        /* สไตล์หน้าปัด SVG */
        .gauge-svg {
            width: 100%;
            max-width: 320px;
            filter: drop-shadow(0 0 15px rgba(0, 242, 254, 0.2));
        }
        #needle {
            transform-origin: 150px 150px;
            transition: transform 0.15s cubic-bezier(0.4, 0, 0.2, 1);
        }
        
        /* การ์ดสถิติ */
        .stats-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 15px;
            margin-top: 25px;
        }
        .stat-card {
            background: rgba(13, 27, 42, 0.6);
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 12px;
            padding: 12px;
        }
        .stat-label { font-size: 0.75rem; color: #778da9; text-transform: uppercase; }
        .stat-val { font-size: 1.4rem; font-weight: bold; color: #4cc9f0; margin-top: 4px; }
        .source-badge {
            display: inline-block;
            margin-top: 15px;
            padding: 6px 14px;
            border-radius: 20px;
            font-size: 0.75rem;
            background: rgba(0, 242, 254, 0.1);
            color: #00f2fe;
            border: 1px solid rgba(0, 242, 254, 0.3);
        }
    </style>
</head>
<body>

<div class="container">
    <h1>🏎️ IoT Edge Speedometer</h1>
    <div class="subtitle">ESP32 Hardware Stream &bull; Kestrel Edge Web Server</div>

    <!-- หน้าปัดวัดความเร็วแบบ SVG แท้ 100% -->
    <svg class="gauge-svg" viewBox="0 0 300 200">
        <!-- รางมาตรวัดพื้นหลัง (สีเทาจาง) -->
        <path d="M 40 150 A 110 110 0 0 1 260 150" fill="none" stroke="#2a3b5c" stroke-width="18" stroke-linecap="round"/>
        
        <!-- รางโซนสีเตือน (เขียว - เหลือง - แดง) -->
        <path d="M 40 150 A 110 110 0 0 1 150 40" fill="none" stroke="#4ade80" stroke-width="6" stroke-linecap="round"/>
        <path d="M 150 40 A 110 110 0 0 1 215 65" fill="none" stroke="#facc15" stroke-width="6"/>
        <path d="M 215 65 A 110 110 0 0 1 260 150" fill="none" stroke="#ef4444" stroke-width="6" stroke-linecap="round"/>

        <!-- ขีดตัวเลขสเกล -->
        <text x="35" y="175" fill="#778da9" font-size="12" text-anchor="middle">0%</text>
        <text x="150" y="25" fill="#778da9" font-size="12" text-anchor="middle">50%</text>
        <text x="265" y="175" fill="#778da9" font-size="12" text-anchor="middle">100%</text>

        <!-- ตัวเลขค่าเปอร์เซ็นต์ดิจิทัลตรงกลาง -->
        <text id="disp-percent" x="150" y="135" fill="#ffffff" font-size="32" font-weight="bold" text-anchor="middle">0.0%</text>

        <!-- เข็มไมล์สีแดงสะท้อนแสง (หมุนรอบจุด 150, 150) -->
        <g id="needle" style="transform: rotate(-90deg);">
            <!-- ตัวเข็มทรงสามเหลี่ยมเรียว -->
            <polygon points="147,150 153,150 151,50 149,50" fill="#ff0055" filter="drop-shadow(0 0 6px #ff0055)"/>
            <!-- ฝาครอบแกนหมุนตรงกลาง -->
            <circle cx="150" cy="150" r="10" fill="#ffffff"/>
            <circle cx="150" cy="150" r="5" fill="#ff0055"/>
        </g>
    </svg>

    <div class="stats-grid">
        <div class="stat-card">
            <div class="stat-label">ADC 12-Bit Raw</div>
            <div class="stat-val" id="disp-raw">0</div>
        </div>
        <div class="stat-card">
            <div class="stat-label">Sensor Voltage</div>
            <div class="stat-val" id="disp-volt">0.00 V</div>
        </div>
    </div>

    <div class="source-badge" id="disp-source">Connecting to Server...</div>
</div>

<script>
    // ฟังก์ชันคำนวณแปลงสเกลเชิงเส้น (Linear Mapping)
    // แปลง 0% - 100% เป็นมุมองศา -90deg ถึง +90deg
    function percentToAngle(pct) {
        return (pct / 100.0) * 180.0 - 90.0;
    }

    async function pollTelemetry() {
        try {
            const res = await fetch('/api/telemetry');
            if (!res.ok) return;
            const data = await res.json();

            // อัปเดตตัวเลขบนหน้าจอ
            document.getElementById('disp-percent').textContent = data.percentage.toFixed(1) + '%';
            document.getElementById('disp-raw').textContent = data.rawValue;
            document.getElementById('disp-volt').textContent = data.voltage.toFixed(2) + ' V';
            document.getElementById('disp-source').textContent = '📡 ' + data.dataSource;

            // สั่งหมุนเข็มไมล์ SVG นุ่มนวลตามมุมที่คำนวณได้
            const angle = percentToAngle(data.percentage);
            document.getElementById('needle').style.transform = `rotate(${angle}deg)`;
        } catch (err) {
            console.error('Polling error:', err);
        }
    }

    // วนลูปดึงข้อมูลทุกๆ 150 มิลลิวินาที (ประมาณ 7 ครั้งต่อวินาที)
    setInterval(pollTelemetry, 150);
</script>

</body>
</html>
```

---

### กิจกรรมที่ 3: ทดสอบหมุนหน้าปัด

1. รันเซิร์ฟเวอร์ด้วยคำสั่ง:
   ```bash
   dotnet run
   ```
2. เปิดเบราว์เซอร์ไปที่: `http://localhost:5000`
3. **สิ่งที่เกิดขึ้น:** หน้าปัด Speedometer สไตล์ซูเปอร์คาร์จะปรากฏขึ้นกลางหน้าจอ
4. **ลงมือทดสอบ:** 
   - ใช้นิ้วหมุนตัวต้านทานปรับค่าได้ (Potentiometer) บนโต๊ะไปทางขวา $\rightarrow$ เข็มไมล์จะกวาดขึ้นอย่างลื่นไหล ตัวเลขเปอร์เซ็นต์วิ่งขึ้นตามมือทันที
   - หมุนกลับมาทางซ้าย $\rightarrow$ เข็มไมล์ตกลงมาที่ศูนย์อย่างนุ่มนวล!

#### ตัวอย่างหน้าจอ

![](Images/Speedometer.png)


>  **ยินดีด้วย!** นักศึกษาได้สร้างระบบ **Full-Stack Physical-to-Web IoT Gateway** ที่สมบูรณ์แบบด้วยฝีมือตนเองตั้งแต่ระดับวงจรแอนะล็อกจนถึงหน้าจอเว็บแอปพลิเคชัน!

---

## สนามทดลองสร้างสรรค์ (Creative Playground: ปล่อยพลังแต่ง Dashboard)

เพื่อให้นักศึกษาได้นำเสนอความคิดสร้างสรรค์และสร้างผลงานเฉพาะตัว ให้เลือกทำกิจกรรมเสริมอย่างน้อย **1 รูปแบบ** จากตัวเลือกด้านล่างนี้:

---

### A หน้าปัดมาตรวัดสัญญาณเสียง (Audio VU Meter / LED Bar)
สร้างแถบหลอดไฟ LED แนวตั้งหรือแนวนอน 10 ดวง (เขียว 6 ดวง, เหลือง 2 ดวง, แดง 2 ดวง) ที่จะค่อยๆ สว่างขึ้นตามระดับความแรงของเซนเซอร์:

```html
<!-- โค้ด SVG ตัวอย่างสำหรับ VU Meter (แทรกในหน้าเว็บ) -->
<svg width="250" height="40" viewBox="0 0 250 40" id="vumeter">
    <!-- แถบ LED 10 แท่ง (ปรับ opacity ด้วย JavaScript) -->
    <rect class="led" x="5" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="28" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="51" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="74" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="97" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="120" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.2"/>
    <rect class="led" x="143" y="5" width="18" height="30" rx="3" fill="#eab308" opacity="0.2"/>
    <rect class="led" x="166" y="5" width="18" height="30" rx="3" fill="#eab308" opacity="0.2"/>
    <rect class="led" x="189" y="5" width="18" height="30" rx="3" fill="#ef4444" opacity="0.2"/>
    <rect class="led" x="212" y="5" width="18" height="30" rx="3" fill="#ef4444" opacity="0.2"/>
</svg>
```
*แนวทาง JavaScript ควบคุม:* คำนวณจำนวนหลอดที่ต้องเปิด = `Math.floor(data.percentage / 10)` แล้วสั่งเปลี่ยน `opacity` เป็น `1.0` พร้อมใส่ Glow Effect!


#### ตัวอย่าง code ในไฟล์ index.html

```Javascript 

 // 1. เพิ่มฟังก์ชันสำหรับอัปเดตแถบไฟ VU Meter
        function updateVuMeter(percentage) {
            const leds = document.querySelectorAll('#vumeter .led');
            const totalLeds = leds.length; // มี 10 หลอด

            // คำนวณจำนวนหลอดที่ต้องเปิด (0 ถึง 10 หลอด)
            // เช่น 45% -> 4.5 -> ปัดเป็น 5 หลอด หรือใช้ Math.floor จะได้ 4 หลอด
            const activeCount = Math.round((percentage / 100.0) * totalLeds);

            leds.forEach((led, index) => {
                if (index < activeCount) {
                    // หลอดที่เปิด: ปรับความสว่างเต็มที่ พร้อมใส่แสงนีออนเรืองแสงตามสีเดิมของหลอด
                    led.style.opacity = '1.0';
                    const color = led.getAttribute('fill');
                    led.style.filter = `drop-shadow(0 0 6px ${color})`;
                } else {
                    // หลอดที่ปิด: ปรับให้หรี่มืดลง
                    led.style.opacity = '0.15';
                    led.style.filter = 'none';
                }
            });
        }

        // 2. ปรับปรุงฟังก์ชัน pollTelemetry ให้เรียกใช้ฟังก์ชัน updateVuMeter
        async function pollTelemetry() {
            try {
                const res = await fetch('/api/telemetry');
                if (!res.ok) return;
                const data = await res.json();

                // 1. อัปเดตตัวเลขแสดงผลด้านล่าง
                const dispPercent = document.getElementById('disp-percent');
                if (dispPercent) {
                    dispPercent.textContent = data.percentage.toFixed(1) + '%';
                }
                document.getElementById('disp-raw').textContent = data.rawValue;
                document.getElementById('disp-volt').textContent = data.voltage.toFixed(2) + ' V';
                document.getElementById('disp-source').textContent = '📡 ' + data.dataSource;

                // 2. เรียกฟังก์ชันอัปเดต VU Meter ตามเปอร์เซ็นต์เซนเซอร์
                updateVuMeter(data.percentage);

            } catch (err) {
                console.error('Polling error:', err);
            }
        }

        // วนลูปดึงข้อมูลทุกๆ 150 มิลลิวินาที
        setInterval(pollTelemetry, 150);
```

#### ตัวอย่างหน้าจอ

![](Images/UV%20Meter.png)





---

### B หน้าจอดิจิทัลเรโทร 7 ส่วน (Retro 7-Segment SVG)
สร้างหน้าปัดตัวเลขดิจิทัลแบบ 7 ส่วนเรืองแสงสไตล์นีออนเรโทร โดยควบคุมชิ้นส่วนของเส้น segment (a, b, c, d, e, f, g) ให้เปิด-ปิดตามตัวเลขเปอร์เซ็นต์ที่อ่านได้

#### 1. หลักการทำงานของ 7-Segment Display
ตัวเลขดิจิทัล 1 หลัก ประกอบด้วยเส้นหลอดไฟ 7 เส้น เรียงตามชื่อมาตรฐานสากล
```text
      -- a --
     |       |
     f       b
     |       |
      -- g --
     |       |
     e       c
     |       |
      -- d --
```
เราจะใช้ **Truth Table (Segment Lookup Table)** จับคู่ตัวเลข `0 - 9` กับสถานะติด/ดับของแต่ละเส้น `[a, b, c, d, e, f, g]` เช่น เลข `8` จะติดทุกเส้น `[1,1,1,1,1,1,1]` ส่วนเลข `1` จะติดเฉพาะเส้นขวา `[0,1,1,0,0,0,0]`

---

#### 2. โค้ด SVG และ CSS สำหรับแทรกใน `index.html`

วางโค้ด `<svg>` และ `<style>` นี้ลงในส่วนแสดงผลของหน้าเว็บ `wwwroot/index.html`

```html
<style>
    /* สไตล์หลอด 7-Segment สไตล์เรโทรนีออน */
    .seg {
        fill: #1e293b;            /* สีตอนปิด (ดับสนิท / เงาดำ) */
        opacity: 0.15;
        transition: opacity 0.08s ease, fill 0.08s ease;
    }

    /* เมื่อ segment ถูกสั่งเปิด (Active) จะเรืองแสงนีออน */
    .seg.active {
        fill: #00ffcc;            /* สีนีออนเขียวอมฟ้า (หรือเปลี่ยนเป็น #ff0055 สีนีออนแดง) */
        opacity: 1;
        filter: drop-shadow(0 0 6px #00ffcc);
    }
</style>

<!-- หน้าปัด 7-Segment แสดงผล 2 หลัก (00 - 99%) -->
<svg width="230" height="130" viewBox="0 0 230 130" id="seven-seg-board">
    <!-- กรอบหน้าปัดสีเข้มสไตล์ Cyberpunk/Retro -->
    <rect x="5" y="5" width="220" height="120" rx="12" fill="#0b0f19" stroke="#1e293b" stroke-width="2"/>

    <!-- หลักสิบ (Tens Digit) -->
    <g id="seg-digit1" transform="translate(25, 15)">
        <polygon class="seg a" points="14,12 19,7 41,7 46,12 41,17 19,17"/>
        <polygon class="seg b" points="48,14 53,19 53,41 48,46 43,41 43,19"/>
        <polygon class="seg c" points="48,54 53,59 53,81 48,86 43,81 43,59"/>
        <polygon class="seg d" points="14,88 19,83 41,83 46,88 41,93 19,93"/>
        <polygon class="seg e" points="12,54 17,59 17,81 12,86 7,81 7,59"/>
        <polygon class="seg f" points="12,14 17,19 17,41 12,46 7,41 7,19"/>
        <polygon class="seg g" points="14,50 19,45 41,45 46,50 41,55 19,55"/>
    </g>

    <!-- หลักหน่วย (Ones Digit) -->
    <g id="seg-digit2" transform="translate(100, 15)">
        <polygon class="seg a" points="14,12 19,7 41,7 46,12 41,17 19,17"/>
        <polygon class="seg b" points="48,14 53,19 53,41 48,46 43,41 43,19"/>
        <polygon class="seg c" points="48,54 53,59 53,81 48,86 43,81 43,59"/>
        <polygon class="seg d" points="14,88 19,83 41,83 46,88 41,93 19,93"/>
        <polygon class="seg e" points="12,54 17,59 17,81 12,86 7,81 7,59"/>
        <polygon class="seg f" points="12,14 17,19 17,41 12,46 7,41 7,19"/>
        <polygon class="seg g" points="14,50 19,45 41,45 46,50 41,55 19,55"/>
    </g>

    <!-- สัญลักษณ์เปอร์เซ็นต์ % -->
    <text x="175" y="90" fill="#00ffcc" font-family="'Courier New', monospace" font-size="28" font-weight="bold" opacity="0.85">%</text>
</svg>
```

---

#### 3. โค้ด JavaScript ควบคุมการเปิด-ปิด Segment

เพิ่มตารางค่า Segment และฟังก์ชัน `update7Segment()` ลงในแท็ก `<script>`:

```javascript
// ตารางสถานะของหลอด 7-Segment (a, b, c, d, e, f, g) สำหรับเลข 0 ถึง 9
const SEGMENT_MAP = {
    0: [1, 1, 1, 1, 1, 1, 0],
    1: [0, 1, 1, 0, 0, 0, 0],
    2: [1, 1, 0, 1, 1, 0, 1],
    3: [1, 1, 1, 1, 0, 0, 1],
    4: [0, 1, 1, 0, 0, 1, 1],
    5: [1, 0, 1, 1, 0, 1, 1],
    6: [1, 0, 1, 1, 1, 1, 1],
    7: [1, 1, 1, 0, 0, 0, 0],
    8: [1, 1, 1, 1, 1, 1, 1],
    9: [1, 1, 1, 1, 0, 1, 1]
};

const SEG_NAMES = ['a', 'b', 'c', 'd', 'e', 'f', 'g'];

// ฟังก์ชันสั่งเปิด-ปิด segment ของตัวเลข 1 หลัก
function setDigit(digitGroupId, num) {
    const group = document.getElementById(digitGroupId);
    if (!group) return;

    const pattern = SEGMENT_MAP[num] || [0, 0, 0, 0, 0, 0, 0];

    SEG_NAMES.forEach((segName, index) => {
        const segElement = group.querySelector(`.${segName}`);
        if (segElement) {
            if (pattern[index] === 1) {
                segElement.classList.add('active');
            } else {
                segElement.classList.remove('active');
            }
        }
    });
}

// ฟังก์ชันแยกหลักสิบและหลักหน่วย แล้วสั่งอัปเดตหน้าจอ
function update7Segment(percentage) {
    // จำกัดค่า 0 - 99 เพื่อแสดงผล 2 หลัก
    const val = Math.min(99, Math.max(0, Math.round(percentage)));
    const tens = Math.floor(val / 10);
    const ones = val % 10;

    setDigit('seg-digit1', tens);
    setDigit('seg-digit2', ones);
}
```

---

#### 4. นำไปเชื่อมต่อกับลูป Polling (`pollTelemetry`)

ในฟังก์ชัน `pollTelemetry()` ให้เรียก `update7Segment(data.percentage)` เพื่อให้หน้าจอเปลี่ยนตามการหมุนตัวต้านทานแบบ Real-time:

```javascript
async function pollTelemetry() {
    try {
        const response = await fetch('/api/telemetry');
        if (!response.ok) return;

        const data = await response.json();

        // 1. อัปเดตการ์ดตัวเลขและสถานะด้านล่าง
        const dispRaw = document.getElementById('disp-raw');
        if (dispRaw) dispRaw.textContent = data.rawValue;

        const dispVolt = document.getElementById('disp-volt');
        if (dispVolt) dispVolt.textContent = data.voltage.toFixed(2) + ' V';

        const dispSource = document.getElementById('disp-source');
        if (dispSource) dispSource.textContent = '📡 ' + data.dataSource;

        // 2. เรียกฟังก์ชันอัปเดต 7-Segment ตามเปอร์เซ็นต์เซนเซอร์
        update7Segment(data.percentage);

    } catch (err) {
        console.error('Polling error:', err);
    }
}

// วนลูปดึงข้อมูลทุกๆ 150 มิลลิวินาที
setInterval(pollTelemetry, 150);
```

#### ตัวอย่างหน้าจอ

![](Images/Seven%20segments.png)


---

### 🛢️ ตัวเลือก C: ถังระดับของเหลวอุตสาหกรรม (Liquid Level Tank)
สร้างหน้าปัดแท็งก์น้ำทรงกระบอกอุตสาหกรรม ที่ระดับน้ำสีฟ้าเรืองแสงจะกระเพื่อมและสูงขึ้น-ลดลงในถังตามระดับการหมุนตัวต้านทาน

#### 1. หลักการทำงานของ Liquid Level Tank
- ใช้เทคนิค **SVG `<clipPath>`** เพื่อจำกัดขอบเขตของระดับของเหลว ให้อยู่เฉพาะภายในกรอบขอบโค้งของถัง ไม่ล้นออกมาภายนอก
- ใช้ **SVG `<linearGradient>`** สร้างมิติแสงสะท้อนของของเหลวเรืองแสง (Water Glow)
- คำนวณพิกัดระดับน้ำด้วยสมการแกน Y (เนื่องจากในระบบพิกัด SVG จุด `(0,0)` อยู่มุมซ้ายบน เมื่อระดับน้ำสูงขึ้น ค่าพิกัด `y` จะต้องลดลง):
  $$\text{fillHeight} = \left(\frac{\text{percentage}}{100}\right) \times \text{maxHeight}$$
  $$\text{fillY} = \text{tankBottomY} - \text{fillHeight}$$

---

#### 2. โค้ด SVG และ CSS สำหรับแทรกใน `index.html`

วางโค้ด `<svg>` และ `<style>` นี้ลงในส่วนแสดงผลของหน้าเว็บ `wwwroot/index.html`:

```html
<style>
    /* แอนิเมชันให้ระดับน้ำเคลื่อนที่ขึ้นลงอย่างนุ่มนวล */
    #water-fill {
        transition: y 0.15s ease-out, height 0.15s ease-out;
    }
</style>

<!-- ถังระดับของเหลวอุตสาหกรรม (Liquid Level Tank) -->
<svg width="220" height="220" viewBox="0 0 220 220" id="liquid-tank">
    <defs>
        <!-- การไล่เฉดสีของของเหลวสีฟ้าเรืองแสง (Neon Cyan to Blue) -->
        <linearGradient id="water-grad" x1="0%" y1="0%" x2="0%" y2="100%">
            <stop offset="0%" stop-color="#00f2fe" stop-opacity="0.95"/>
            <stop offset="100%" stop-color="#4facfe" stop-opacity="0.8"/>
        </linearGradient>

        <!-- คลิปพาร์ททรงถังน้ำขอบมน (ป้องกันน้ำล้นออกนอกถัง) -->
        <clipPath id="tank-shape">
            <rect x="60" y="25" width="80" height="135" rx="18"/>
        </clipPath>
    </defs>

    <!-- เงาพื้นหลังของถัง (ความลึกด้านใน) -->
    <rect x="60" y="25" width="80" height="135" rx="18" fill="#07101e" stroke="#1e293b" stroke-width="2"/>

    <!-- ระดับของเหลว (ถูกครอบด้วย clip-path ทรงถัง) -->
    <g clip-path="url(#tank-shape)">
        <rect id="water-fill" x="60" y="160" width="80" height="0" fill="url(#water-grad)" filter="drop-shadow(0 0 8px #00f2fe)"/>
    </g>

    <!-- ขอบกระจกถังน้ำ (Glass Body Outline) -->
    <rect x="60" y="25" width="80" height="135" rx="18" fill="none" stroke="#38bdf8" stroke-width="3" opacity="0.6"/>

    <!-- แถบสะท้อนแสงผิวกระจกด้านข้าง (Glossy Reflection) -->
    <rect x="66" y="32" width="6" height="120" rx="3" fill="#ffffff" opacity="0.15"/>

    <!-- สเกลวัดระดับขีดเปอร์เซ็นต์ (Scale Markings) -->
    <!-- 100% -->
    <line x1="145" y1="25" x2="155" y2="25" stroke="#94a3b8" stroke-width="2"/>
    <text x="162" y="29" fill="#94a3b8" font-size="11" font-family="sans-serif">100%</text>

    <!-- 75% -->
    <line x1="145" y1="59" x2="151" y2="59" stroke="#64748b" stroke-width="1.5"/>
    <text x="162" y="63" fill="#64748b" font-size="10" font-family="sans-serif">75%</text>

    <!-- 50% -->
    <line x1="145" y1="92" x2="155" y2="92" stroke="#94a3b8" stroke-width="2"/>
    <text x="162" y="96" fill="#94a3b8" font-size="11" font-family="sans-serif">50%</text>

    <!-- 25% -->
    <line x1="145" y1="126" x2="151" y2="126" stroke="#64748b" stroke-width="1.5"/>
    <text x="162" y="130" fill="#64748b" font-size="10" font-family="sans-serif">25%</text>

    <!-- 0% -->
    <line x1="145" y1="160" x2="155" y2="160" stroke="#94a3b8" stroke-width="2"/>
    <text x="162" y="164" fill="#94a3b8" font-size="11" font-family="sans-serif">0%</text>

    <!-- ตัวเลขแสดงเปอร์เซ็นต์แบบดิจิทัลใต้ถังน้ำ -->
    <text id="tank-val" x="100" y="195" text-anchor="middle" fill="#00f2fe" font-family="'Segoe UI', sans-serif" font-size="22" font-weight="bold">0.0%</text>
    <text x="100" y="212" text-anchor="middle" fill="#64748b" font-family="sans-serif" font-size="11">TANK LEVEL</text>
</svg>
```

---

#### 3. โค้ด JavaScript คำนวณและปรับระดับน้ำ

เพิ่มฟังก์ชัน `updateTankLevel()` ลงในแท็ก `<script>`:

```javascript
// ฟังก์ชันคำนวณและอัปเดตระดับของเหลวในถัง
function updateTankLevel(percentage) {
    const maxHeight = 135;   // ความสูงสูงสุดของถัง (px)
    const tankBottomY = 160; // พิกัดแกน Y ก้นถัง

    // จำกัดขอบเขต 0 - 100%
    const clampedPct = Math.min(100, Math.max(0, percentage));

    // คำนวณความสูงและพิกัด Y ของระดับน้ำ
    const fillHeight = (clampedPct / 100.0) * maxHeight;
    const fillY = tankBottomY - fillHeight;

    const water = document.getElementById('water-fill');
    if (water) {
        water.setAttribute('height', fillHeight);
        water.setAttribute('y', fillY);
    }

    // อัปเดตตัวเลขเปอร์เซ็นต์ดิจิทัลใต้ถัง
    const tankText = document.getElementById('tank-val');
    if (tankText) {
        tankText.textContent = clampedPct.toFixed(1) + '%';
    }
}
```

---

#### 4. นำไปเชื่อมต่อกับลูป Polling (`pollTelemetry`)

ในฟังก์ชัน `pollTelemetry()` ให้เรียก `updateTankLevel(data.percentage)`:

```javascript
async function pollTelemetry() {
    try {
        const response = await fetch('/api/telemetry');
        if (!response.ok) return;

        const data = await response.json();

        // 1. อัปเดตการ์ดตัวเลขและสถานะด้านล่าง
        const dispRaw = document.getElementById('disp-raw');
        if (dispRaw) dispRaw.textContent = data.rawValue;

        const dispVolt = document.getElementById('disp-volt');
        if (dispVolt) dispVolt.textContent = data.voltage.toFixed(2) + ' V';

        const dispSource = document.getElementById('disp-source');
        if (dispSource) dispSource.textContent = '📡 ' + data.dataSource;

        // 2. เรียกฟังก์ชันอัปเดตระดับน้ำในถัง
        updateTankLevel(data.percentage);

    } catch (err) {
        console.error('Polling error:', err);
    }
}

// วนลูปดึงข้อมูลทุกๆ 150 มิลลิวินาที
setInterval(pollTelemetry, 150);
```


#### ตัวอย่างหน้าจอ

![](Images/Liquid%20Tank.png)
---

## 📋 ส่งงานและประเมินผล (Submission Check)
1. บันทึกวิดีโอคลิปสั้น (15-30 วินาที) โดยในคลิปต้องเห็น:
   - นิ้วมือนักศึกษากำลังหมุนตัวต้านทานปรับค่าได้บนบอร์ด ESP32
   - หน้าจอคอมพิวเตอร์ที่เข็มไมล์ Speedometer / VU Meter กวาดตามมืออย่างชัดเจน
1. แนบภาพหน้าจอซอร์สโค้ดและรายงานการทดลอง
https://drive.google.com/file/d/1BjCcbNe-gF282dphieTQmEElFWHK7_9n/view?usp=drivesdk

```html
<!DOCTYPE html>

<html lang="en">

<head>

    <meta charset="UTF-8">

    <meta name="viewport" content="width=device-width, initial-scale=1.0">

    <title>IoT Edge VU Meter (LDR Sensor)</title>

    <style>

        body {

            background-color: #0b0f19;

            color: #e2e8f0;

            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;

            display: flex;

            justify-content: center;

            align-items: center;

            height: 100vh;

            margin: 0;

        }

        .dashboard-card {

            background-color: #1e293b;

            padding: 40px;

            border-radius: 16px;

            box-shadow: 0 10px 25px rgba(0,0,0,0.5);

            text-align: center;

            border: 1px solid #334155;

        }

        h2 { margin-top: 0; color: #38bdf8; }

        .led { transition: opacity 0.1s ease, filter 0.1s ease; }

        .stats-container {

            margin-top: 30px;

            background-color: #0f172a;

            padding: 15px;

            border-radius: 10px;

            display: grid;

            grid-template-columns: 1fr 1fr;

            gap: 15px;

            text-align: left;

            font-size: 14px;

        }

        .stat-box { display: flex; flex-direction: column; }

        .stat-label { color: #94a3b8; font-size: 12px; margin-bottom: 4px; }

        .stat-value { font-weight: bold; color: #f8fafc; font-size: 16px; }

        .source-box { grid-column: span 2; text-align: center; margin-top: 10px; color: #64748b; font-size: 12px; }

    </style>

</head>

<body>

  

    <div class="dashboard-card">

        <h2>Light Intensity VU Meter</h2>

        <!-- โค้ด SVG หน้าปัด VU Meter -->

        <svg width="250" height="40" viewBox="0 0 250 40" id="vumeter" style="margin: 20px 0;">

            <rect class="led" x="5" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="28" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="51" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="74" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="97" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="120" y="5" width="18" height="30" rx="3" fill="#22c55e" opacity="0.15"/>

            <rect class="led" x="143" y="5" width="18" height="30" rx="3" fill="#eab308" opacity="0.15"/>

            <rect class="led" x="166" y="5" width="18" height="30" rx="3" fill="#eab308" opacity="0.15"/>

            <rect class="led" x="189" y="5" width="18" height="30" rx="3" fill="#ef4444" opacity="0.15"/>

            <rect class="led" x="212" y="5" width="18" height="30" rx="3" fill="#ef4444" opacity="0.15"/>

        </svg>

  

        <!-- ตัวเลขเปอร์เซ็นต์ขนาดใหญ่ -->

        <h1 id="disp-percent" style="font-size: 48px; margin: 10px 0; color: #f8fafc;">0.0%</h1>

  

        <!-- กรอบแสดงค่าดิบและสถานะ -->

        <div class="stats-container">

            <div class="stat-box">

                <span class="stat-label">RAW ADC</span>

                <span class="stat-value" id="disp-raw">0</span>

            </div>

            <div class="stat-box">

                <span class="stat-label">VOLTAGE</span>

                <span class="stat-value" id="disp-volt">0.00 V</span>

            </div>

            <div class="source-box" id="disp-source">📡 Waiting for data...</div>

        </div>

    </div>

  

    <!-- โค้ด JavaScript ควบคุมหน้าจอ -->

    <script>

        // ฟังก์ชันอัปเดตแถบไฟ VU Meter

        function updateVuMeter(percentage) {

            const leds = document.querySelectorAll('#vumeter .led');

            const totalLeds = leds.length; // 10 หลอด

            const activeCount = Math.round((percentage / 100.0) * totalLeds);

  

            leds.forEach((led, index) => {

                if (index < activeCount) {

                    led.style.opacity = '1.0';

                    const color = led.getAttribute('fill');

                    led.style.filter = `drop-shadow(0 0 8px ${color})`;

                } else {

                    led.style.opacity = '0.15';

                    led.style.filter = 'none';

                }

            });

        }

  

        // ฟังก์ชันดึงข้อมูลจากเซิร์ฟเวอร์

        async function pollTelemetry() {

            try {

                const res = await fetch('/api/telemetry');

                if (!res.ok) return;

                const data = await res.json();

  

                // อัปเดตตัวเลข

                document.getElementById('disp-percent').textContent = data.percentage.toFixed(1) + '%';

                document.getElementById('disp-raw').textContent = data.rawValue;

                document.getElementById('disp-volt').textContent = data.voltage.toFixed(2) + ' V';

                document.getElementById('disp-source').textContent = '📡 ' + data.dataSource;

  

                // เรียกฟังก์ชันอัปเดตไฟ LED

                updateVuMeter(data.percentage);

  

            } catch (err) {

                console.error('Polling error:', err);

            }

        }

  

        // วนลูปดึงข้อมูลทุกๆ 150 มิลลิวินาที

        setInterval(pollTelemetry, 150);

    </script>

</body>

</html>
```

Program
```csharp
using System.IO.Ports;

  

var builder = WebApplication.CreateBuilder(args);

  

// ลงทะเบียน Services สำหรับจัดการข้อมูลเบื้องหลัง

builder.Services.AddSingleton<TelemetryStateStore>();

builder.Services.AddHostedService<SerialBridgeWorker>();

  

var app = builder.Build();

  

// 1. สั่งให้เซิร์ฟเวอร์แสดงหน้าเว็บ (index.html) ที่เราเพิ่งทำไป

app.UseDefaultFiles();

app.UseStaticFiles();

  

// 2. Endpoint สำหรับส่งข้อมูลให้หน้าเว็บ (ที่หน้าเว็บกำลังรอรับอยู่)

app.MapGet("/api/telemetry", (TelemetryStateStore state) => {

    var (raw, voltage, percent, source, updated) = state.GetSnapshot();

    return Results.Ok(new {

        sensor = "ESP32-LDR",

        rawValue = raw,

        voltage = voltage,

        percentage = percent,

        dataSource = source,

        timestamp = updated.ToString("yyyy-MM-ddTHH:mm:ss.fffZ")

    });

});

  

app.Run();

  

// ============================================================================

// คลาสเก็บสถานะข้อมูล

// ============================================================================

public class TelemetryStateStore

{

    private readonly object _lock = new();

    private int _rawValue = 0;

    private DateTime _lastUpdated = DateTime.UtcNow;

    private string _source = "Initializing";

  

    public void Update(int rawValue, string source)

    {

        lock (_lock)

        {

            _rawValue = rawValue;

            _source = source;

            _lastUpdated = DateTime.UtcNow;

        }

    }

  

    public (int raw, double voltage, double percent, string source, DateTime updated) GetSnapshot()

    {

        lock (_lock)

        {

            double voltage = Math.Round((_rawValue / 4095.0) * 3.3, 2);

            double percent = Math.Round((_rawValue / 4095.0) * 100.0, 1);

            return (_rawValue, voltage, percent, _source, _lastUpdated);

        }

    }

}

  

// ============================================================================

// คลาสสำหรับอ่านข้อมูลจาก ESP32 ผ่านสาย USB

// ============================================================================

public class SerialBridgeWorker : BackgroundService

{

    private readonly TelemetryStateStore _stateStore;

  

    public SerialBridgeWorker(TelemetryStateStore stateStore)

    {

        _stateStore = stateStore;

    }

  

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)

    {

        await Task.Yield();

  

        while (!stoppingToken.IsCancellationRequested)

        {

            string[] availablePorts = SerialPort.GetPortNames();

            if (availablePorts.Length > 0)

            {

                string? targetPort = "COM3"; // ตรงนี้ตั้งเป็น COM3 ไว้ให้แล้วครับ

                string? selectedPort = null;

                if (!string.IsNullOrEmpty(targetPort) && availablePorts.Contains(targetPort, StringComparer.OrdinalIgnoreCase))

                {

                    selectedPort = targetPort;

                }

                else

                {

                    selectedPort = availablePorts.FirstOrDefault(p => !p.Equals("COM1", StringComparison.OrdinalIgnoreCase)) ?? availablePorts[0];

                }

  

                try

                {

                    using var serial = new SerialPort(selectedPort, 115200);

                    serial.ReadTimeout = 2000;

                    serial.Open();

                    serial.DiscardInBuffer();

  

                    while (!stoppingToken.IsCancellationRequested && serial.IsOpen)

                    {

                        try

                        {

                            if (serial.BytesToRead > 0)

                            {

                                string line = serial.ReadLine().Trim();

                                if (int.TryParse(line, out int val))

                                {

                                    _stateStore.Update(val, $"Live Hardware ({selectedPort})");

                                }

                            }

                            else

                            {

                                await Task.Delay(50, stoppingToken);

                            }

                        }

                        catch (TimeoutException) { await Task.Delay(50, stoppingToken); }

                    }

                }

                catch (Exception) { /* เงียบไว้ถ้าเชื่อมต่อพอร์ตไม่ได้ */ }

            }

  

            // โหมดจำลองข้อมูลถ้าสายหลุด

            for (int i = 0; i < 20 && !stoppingToken.IsCancellationRequested; i++)

            {

                double t = Environment.TickCount64 / 1000.0;

                int simAdc = (int)((Math.Sin(t * 1.5) + 1.0) / 2.0 * 4095);

                _stateStore.Update(simAdc, "Simulation Mode (Sine Wave)");

                await Task.Delay(100, stoppingToken);

            }

        }

    }

}
```
