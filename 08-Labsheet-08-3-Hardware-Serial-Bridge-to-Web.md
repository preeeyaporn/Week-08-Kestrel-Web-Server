![[Micro-Challenge.mp4]]# ใบงานการทดลองที่ 8.3
### สะพานเชื่อมฮาร์ดแวร์จริงสู่เว็บเซิร์ฟเวอร์ (Hardware Serial Bridge)

> **คำชี้แจง:** ในใบงานนี้นักศึกษาจะนำสองโลกมาผสานกัน: **โลกฮาร์ดแวร์ (ESP32)** และ **โลกเว็บเซิร์ฟเวอร์ (.NET Kestrel)** โดยเขียนระบบ Background Worker เพื่อดักอ่านข้อมูลจากสาย USB เข้าสู่หน่วยความจำกลาง และเปิดเป็น REST API ให้เบราว์เซอร์เข้ามาเรียกดูได้ทันที

---

## วัตถุประสงค์การทดลอง (Objectives)
1. สามารถติดตั้งแพ็กเกจ `System.IO.Ports` ลงในโปรเจกต์ .NET Core ผ่าน CLI ได้
2. สามารถสร้างและกำหนดค่า `BackgroundService` เพื่อทำงานดักรับข้อมูลแบบ Non-blocking เธรดเบื้องหลังได้
3. เข้าใจการสร้างคลาสเก็บสถานะข้อมูลแบบปลอดภัยต่อการเข้าถึงพร้อมกัน (Thread-Safe Shared State)
4. สามารถเชื่อมโยงข้อมูลจากสาย USB สู่ Web Endpoint `/api/telemetry` ได้อย่างสมบูรณ์
5. สัมผัสความมหัศจรรย์ของ **Dual-Mode System**: ระบบสลับสู่โหมดจำลอง (Simulator) อัตโนมัติเมื่อถอดสาย

---

## เครื่องมือและสิ่งที่ต้องเตรียม (Prerequisites)
- บอร์ด ESP32 ที่ต่อวงจร Potentiometer พร้อมสาย USB
- **สำคัญมาก:** ต้องปิดหน้าต่าง Serial Monitor / Serial Plotter ในโปรแกรมอื่นให้เรียบร้อย เพื่อปลดล็อกพอร์ต COM

> **ข้อกำหนดสำคัญด้านการส่งงาน (Project Isolation & Anti-Cheating)**  
> ในใบงานที่ 8.3 นักศึกษาต้องสร้างโฟลเดอร์ทำงานใหม่แยกเฉพาะเป็น **`Lab8-3`** โดยมีการฝึกปฏิบัติซ้ำทั้งสองฝั่ง
> 1. **ฝั่ง ESP32:** ทำงานในโฟลเดอร์ `Lab8-3/ESP32_ADC_Stream` (คัดลอกหรือสร้างใหม่จาก Lab8-2)
> 2. **ฝั่ง Kestrel Gateway:** สร้างโปรเจกต์ใหม่ใน `Lab8-3/Kestrel_Serial_Gateway`

---

## ขั้นตอนการทดลอง (Step-by-Step Activities)

### กิจกรรมที่ 1: สร้างโปรเจกต์ Kestrel_Serial_Gateway และติดตั้ง System.IO.Ports

ฝึกฝนการสร้างโปรเจกต์ Web และติดตั้งแพ็กเกจด้วยตนเองอีกครั้ง

1. เปิด Terminal สร้างโฟลเดอร์และโปรเจกต์ใหม่
   ```bash
   # สร้างโฟลเดอร์โปรเจกต์สำหรับ Lab 8.3
   mkdir -p Lab8-3/Kestrel_Serial_Gateway && cd Lab8-3/Kestrel_Serial_Gateway

   # สร้างโปรเจกต์ Web เปล่า
   dotnet new web -o .
   ```

2. พิมพ์คำสั่งติดตั้งไลบรารีสื่อสาร Serial Port ของ .NET:
   ```bash
   dotnet add package System.IO.Ports
   ```
3. ตรวจสอบไฟล์ `Kestrel_Serial_Gateway.csproj` จะเห็นว่ามีบรรทัด `<PackageReference Include="System.IO.Ports" ... />` เพิ่มเข้ามา

---

### 🌟 กิจกรรมที่ 2: สร้าง Shared Memory เก็บค่าเซนเซอร์ (Thread-Safe State Store)

เพื่อป้องกันปัญหาข้อมูลเสียหายจากการอ่านและเขียนพร้อมกัน เราจะสร้างคลาสตัวกลางสำหรับเก็บค่า:

1. เปิดไฟล์ `Program.cs` และเพิ่มคลาส `TelemetryStateStore` ไว้ส่วนล่างสุดของไฟล์:

```csharp
// ============================================================================
// 1. THREAD-SAFE STATE STORE (คลังเก็บค่าสถานะกลาง)
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
```

---

### 🌟 กิจกรรมที่ 3: สร้าง Background Worker พร้อมระบบ Auto-Detect & Simulation Mode

สร้าง Worker เพื่อทำหน้าที่เชื่อมต่อฮาร์ดแวร์ หรือจำลองสัญญาณกรณีไม่มีบอร์ด:

เพิ่มคลาส `SerialBridgeWorker` ลงใน `Program.cs`:

```csharp
// ============================================================================
// 2. BACKGROUND WORKER (คนงานดักฟังข้อมูลจากสาย USB)
// ============================================================================
using System.IO.Ports;

public class SerialBridgeWorker : BackgroundService
{
    private readonly TelemetryStateStore _stateStore;
    private readonly ILogger<SerialBridgeWorker> _logger;

    public SerialBridgeWorker(TelemetryStateStore stateStore, ILogger<SerialBridgeWorker> logger)
    {
        _stateStore = stateStore;
        _logger = logger;
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        // คืนสิทธิ์ให้ Host สามารถเริ่ม Kestrel Web Server ได้อย่างราบรื่น ไม่ติดบล็อก
        await Task.Yield();

        while (!stoppingToken.IsCancellationRequested)
        {
            // 1. ค้นหาพอร์ต COM ทั้งหมดที่มีอยู่ในเครื่อง
            string[] availablePorts = SerialPort.GetPortNames();
            
            if (availablePorts.Length > 0)
            {
                _logger.LogInformation("📋 รายการ COM Ports ในระบบ: [{Ports}]", string.Join(", ", availablePorts));

                // เลือกระหว่าง:
                // 1) ระบุพอร์ตเจาะจง เช่น "COM24" (ถ้ามีอยู่ในระบบ)
                // 2) หรือให้ระบบเลือกพอร์ตแรกที่ไม่ใช่ COM1 อัตโนมัติ
                string? targetPort = "COM24"; // <-- ใส่พอร์ตที่ต้องการ หรือตั้งเป็น null เพื่อให้ออโต้
                
                string? selectedPort = null;
                if (!string.IsNullOrEmpty(targetPort) && availablePorts.Contains(targetPort, StringComparer.OrdinalIgnoreCase))
                {
                    selectedPort = targetPort;
                }
                else
                {
                    if (!string.IsNullOrEmpty(targetPort))
                    {
                        _logger.LogWarning("⚠️ ไม่พบพอร์ต {Target} ในระบบ! ระบบจะเลือกพอร์ตอื่นให้อัตโนมัติ...", targetPort);
                    }
                    selectedPort = availablePorts.FirstOrDefault(p => !p.Equals("COM1", StringComparison.OrdinalIgnoreCase)) ?? availablePorts[0];
                }

                _logger.LogInformation("🔌 กำลังทดลองเชื่อมต่อพอร์ต: {Port}", selectedPort);

                try
                {
                    using var serial = new SerialPort(selectedPort, 115200);
                    serial.ReadTimeout = 2000;
                    serial.Open();
                    serial.DiscardInBuffer(); // เคลียร์ขยะเก่าในบัฟเฟอร์
                    _logger.LogInformation("✅ เชื่อมต่อฮาร์ดแวร์สำเร็จบน {Port} (Live Mode)", selectedPort);

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
                        catch (TimeoutException) 
                        { 
                            await Task.Delay(50, stoppingToken);
                        }
                    }
                }
                catch (Exception ex)
                {
                    _logger.LogWarning("⚠️ ไม่สามารถเปิด {Port} ({Msg}) -> สลับเข้าโหมดจำลอง", selectedPort, ex.Message);
                }
            }
            else
            {
                _logger.LogInformation("🔍 ไม่พบพอร์ต USB -> ทำงานในโหมดจำลอง (Simulation Mode)");
            }

            // 2. โหมดจำลองสัญญาณอัตโนมัติ (Fallback Simulation Mode)
            // วนจำลองสัญญาณ 2 วินาที (20 รอบ รอบละ 100ms) ก่อนจะกลับไปตรวจเช็คพอร์ตใหม่ เพื่อไม่ให้ Log แสดงผลรัวเกินไป
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

---

### 🌟 กิจกรรมที่ 4: ผูกระบบเข้ากับ Minimal API

นำ Service ทั้งหมดมาลงทะเบียนลงใน Dependency Injection (DI) Container และสร้าง Endpoint:

ปรับปรุงส่วนหัวของไฟล์ `Program.cs` ให้เป็นดังนี้:

```csharp
var builder = WebApplication.CreateBuilder(args);

// ลงทะเบียน StateStore เป็น Singleton (มีชิ้นเดียวตลอดอายุโปรแกรม)
builder.Services.AddSingleton<TelemetryStateStore>();

// ลงทะเบียน SerialBridgeWorker เป็น Background Hosted Service
builder.Services.AddHostedService<SerialBridgeWorker>();

var app = builder.Build();

// Endpoint หน้าแรก
app.MapGet("/", () => "IoT Edge Gateway Online! Visit /api/telemetry to view live data.");

// Endpoint สำหรับดึงค่า Telemetry ล่าสุด
app.MapGet("/api/telemetry", (TelemetryStateStore state) => {
    var (raw, voltage, percent, source, updated) = state.GetSnapshot();
    return Results.Ok(new {
        sensor = "ESP32-Potentiometer",
        rawValue = raw,
        voltage = voltage,
        percentage = percent,
        dataSource = source,
        timestamp = updated.ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
    });
});

app.Run();
```

---

### 🌟 กิจกรรมที่ 5: ทดสอบการทำงานสดๆ (The Magic Moment)

1. เสียบสาย USB ESP32 เข้ากับคอมพิวเตอร์
2. พิมพ์คำสั่งสั่งรัน:
   ```bash
   dotnet run
   ```
   *สังเกต Terminal: ระบบจะตรวจพบพอร์ต COM และขึ้นข้อความ `✅ เชื่อมต่อฮาร์ดแวร์สำเร็จ`*
3. เปิดเบราว์เซอร์ไปที่: `http://localhost:5000/api/telemetry`
4. **ทดสอบหมุนตัวต้านทานปรับค่าได้บนโต๊ะ แล้วกด Refresh (F5) บนเบราว์เซอร์:**
   - **สิ่งที่สังเกตได้:** ตัวเลข `rawValue`, `voltage`, และ `percentage` บนหน้าจอเบราว์เซอร์จะเปลี่ยนไปตามมุมที่มือนักศึกษาหมุนบอร์ดเป๊ะๆ!

> 💡 **จุดว้าวที่ 3:** โลกกายภาพ (นิ้วมือหมุน Volume) ได้ส่งสัญญาณไฟฟ้า ทะลุสาย USB ผ่าน .NET Web Server และมาปรากฏเป็นข้อมูลบนหน้าเว็บได้อย่างสมบูรณ์แบบแล้ว!

---

#### 📋 [Checkpoint 3.1: ทดสอบกลไก Fallback Simulation]
1. ขณะที่โปรแกรม C# กำลังรันอยู่ ให้ถอดสาย USB ของ ESP32 ออกจากคอมพิวเตอร์
2. กด Refresh (F5) บนหน้าเบราว์เซอร์หลายๆ ครั้งติดต่อกัน สังเกตว่า:
   - - **ค่า dataSource เปลี่ยนเป็นอะไร?:** เปลี่ยนเป็นข้อความ **`"Simulation Mode (Sine Wave)"`**
  - **ค่า rawValue ยังเปลี่ยนได้อยู่หรือไม่ และเปลี่ยนในลักษณะใด?:** ค่า rawValue **ยังคงเปลี่ยนแปลงได้อยู่** โดยตัวเลขจะเปลี่ยนสลับขึ้นลงอย่างนุ่มนวลอัตโนมัติ (แกว่งอยู่ในช่วง 0 ถึง 4095) ซึ่งเป็นลักษณะการเปลี่ยนแปลงแบบ **คลื่นไซน์ (Sine Wave)** ตามสมการคณิตศาสตร์ที่ตั้งโปรแกรมจำลองเอาไว้เพื่อไม่ให้ระบบเซิร์ฟเวอร์ค้างเมื่ออุปกรณ์ขาดการเชื่อมต่อ
   - **คำตอบ:** (สาเหตุที่เป็นแบบนี้ เพราะในโค้ด C# เราใส่ระบบ `try-catch` เอาไว้ครับ เมื่อเซิร์ฟเวอร์ตรวจพบว่าสาย USB ถูกถอดหรือเกิด Error ระบบจะสลับตัวเองเข้าสู่โหมดจำลองคลื่น Sine Wave อัตโนมัติ เพื่อให้หน้าเว็บยังมีข้อมูลแสดงผลต่อไปได้โดยไม่พัง

---

## 🎯 ภารกิจท้าทาย (Micro-Challenge)
- ให้นักศึกษาเพิ่มฟิลด์ `alertLevel` เข้าไปในผลลัพธ์ JSON ของ `/api/telemetry`:
  - ถ้า `percentage` มากกว่า 85.0% ให้ส่งค่า `"DANGER (HIGH)"`
  - ถ้า `percentage` ระหว่าง 70.0% - 85.0% ให้ส่งค่า `"WARNING"`
  - ถ้า `percentage` ต่ำกว่า 70.0% ให้ส่งค่า `"NORMAL"`
- บันทึกภาพหน้าจอเบราว์เซอร์ขณะหมุนไปที่ระดับต่างๆ เพื่อแสดงว่าฟิลด์ `alertLevel` ทำงานถูกต้อง
<img width="960" height="672" alt="image" src="https://github.com/user-attachments/assets/f47cf908-3080-4de1-b520-139813027150" />
<img width="890" height="700" alt="image" src="https://github.com/user-attachments/assets/40ae1da3-198a-448a-8ce2-bd25e5c0cf8d" />
<img width="935" height="672" alt="image" src="https://github.com/user-attachments/assets/10a3fc17-7b51-47c9-be78-ed3bd09a5625" />
