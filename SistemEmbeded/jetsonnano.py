# jetsonn@jetsonn-desktop:~$ python --version
# Python 3.8.0

# requirements.txt
# adafruit-blinka
# adafruit-circuitpython-ssd1306
# adafruit-circuitpython-busdevice
# Pillow==8.3.2
# requests==2.25.1
# Flask==2.0.3  # Optional, kalau mau tambah web endpoint nanti
# adafruit-circuitpython-dht

import time
import board
import busio
import adafruit_ssd1306
from PIL import Image, ImageDraw, ImageFont
import socket
import requests  # Back to direct requests for reliability
from datetime import datetime, timedelta
import adafruit_dht  # For DHT22

# Display Parameters
WIDTH = 128
HEIGHT = 64
LOOPTIME = 5.0  # Duration to display each screen (seconds)

# Initialize I2C and OLED
i2c = busio.I2C(board.SCL, board.SDA)
oled = adafruit_ssd1306.SSD1306_I2C(WIDTH, HEIGHT, i2c, addr=0x3C)
oled.fill(0)
oled.show()

# Initialize DHT22 (Pin 11 / D11)
try:
    dht_device = adafruit_dht.DHT22(board.D11)
    DHT_AVAILABLE = True
    print("✅ DHT22 initialized on Pin 11")
except Exception as e:
    print(f"⚠️ DHT22 error: {e}")
    DHT_AVAILABLE = False
    dht_device = None

# Create Canvas
image = Image.new('1', (WIDTH, HEIGHT))
draw = ImageDraw.Draw(image)

# Load Fonts
try:
    font = ImageFont.truetype('PixelOperator.ttf', 12)
    status_font = ImageFont.truetype('PixelOperator.ttf', 10)
except:
    font = ImageFont.load_default()
    status_font = ImageFont.load_default()
    def estimate_text_width(text, font_size=10):
        return len(text) * (font_size // 2)

# Eye Positions
LEFT_EYE = (28, 10, 52, 34)
RIGHT_EYE = (76, 10, 100, 34)

# Check Internet Connection
def check_internet(host="8.8.8.8", port=53, timeout=1):
    try:
        socket.setdefaulttimeout(timeout)
        socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect((host, port))
        return True
    except Exception:
        return False

# Draw Animated Eyes
def draw_eyes(left_offset=0, right_offset=0, blink=False):
    draw.rectangle((0, 0, WIDTH, HEIGHT), outline=0, fill=0)
    if blink:
        draw.line((LEFT_EYE[0], 22, LEFT_EYE[2], 22), fill=255, width=2)
        draw.line((RIGHT_EYE[0], 22, RIGHT_EYE[2], 22), fill=255, width=2)
    else:
        draw.ellipse((LEFT_EYE[0]+left_offset, LEFT_EYE[1], LEFT_EYE[2]+left_offset, LEFT_EYE[3]), outline=255, fill=255)
        draw.ellipse((RIGHT_EYE[0]+right_offset, RIGHT_EYE[1], RIGHT_EYE[2]+right_offset, RIGHT_EYE[3]), outline=255, fill=255)
    
    status = "ONLINE" if check_internet() else "OFFLINE"
    try:
        bbox = draw.textbbox((0, 0), status, font=status_font)
        w = bbox[2] - bbox[0]
    except ValueError:
        w = estimate_text_width(status) if 'estimate_text_width' in locals() else len(status) * 5
    
    draw.text(((WIDTH - w) // 2, HEIGHT - 10), status, font=status_font, fill=255)
    oled.image(image)
    oled.show()

# Fetch Weather Data for Tangerang (Direct requests, more reliable)
def fetch_weather():
    if not check_internet():
        return {"error": "No Net"}
    
    # Tangerang coordinates
    latitude = -6.1784
    longitude = 106.6297
    timezone = "Asia/Jakarta"
    
    # Get 48 hours for today + tomorrow
    start_date = datetime.now().strftime("%Y-%m-%d")
    end_date = (datetime.now() + timedelta(days=1)).strftime("%Y-%m-%d")
    
    url = (
        f"https://api.open-meteo.com/v1/forecast?"
        f"latitude={latitude}&longitude={longitude}"
        f"&hourly=temperature_2m,precipitation,wind_speed_10m"
        f"&start_date={start_date}&end_date={end_date}&timezone={timezone}"
    )
    
    try:
        resp = requests.get(url, timeout=10)  # Increased timeout
        if resp.status_code == 200:
            data = resp.json()
            hourly_temps = data['hourly']['temperature_2m']
            hourly_precip = data['hourly']['precipitation']
            
            # Current (first valid hour, approx now)
            current_temp = hourly_temps[0]
            current_precip = hourly_precip[0]
            
            # High/Low for today (first 24 hours)
            today_temps = hourly_temps[:24]
            high = max(today_temps)
            low = min(today_temps)
            
            return {
                "temp": f"{current_temp:.0f}°",
                "precip": current_precip,
                "high": high,
                "low": low,
                "hourly_temps": today_temps  # For fallback
            }
        else:
            return {"error": "API Fail"}
    except Exception as e:
        return {"error": f"Conn: {str(e)[:8]}"}

# Get DHT22 Temp
def get_dht_temp():
    if not DHT_AVAILABLE:
        return None
    try:
        temp = dht_device.temperature
        if temp is not None:
            return temp
    except Exception:
        pass
    return None

# Draw Weather Information (Rapih layout)
def draw_weather_info():
    draw.rectangle((0, 0, WIDTH, HEIGHT), fill=0)
    x = 2
    y = 0
    line_height = 11  # Smaller for fit

    # Title
    draw.text((x, y), "Tangerang Weather", font=font, fill=255)
    y += line_height + 1

    weather_data = fetch_weather()
    dht_temp = get_dht_temp()

    if "error" in weather_data:
        draw.text((x, y), f"Err: {weather_data['error'][:12]}", font=font, fill=255)
        y += line_height
    else:
        # Current temp (big, direct like "28°")
        draw.text((x, y), weather_data["temp"], font=font, fill=255)
        y += line_height + 2  # Extra space after temp

        # Condition based on precip
        precip = weather_data["precip"]
        if precip == 0:
            condition = "Mostly Clear"
        elif precip < 1:
            condition = "Mostly Cloudy"
        else:
            condition = "Rainy"
        draw.text((x, y), condition, font=status_font, fill=255)
        y += line_height

        # High / Low
        high = weather_data["high"]
        low = weather_data["low"]
        draw.text((x, y), f"H:{high:.0f}° L:{low:.0f}°", font=status_font, fill=255)
        y += line_height

    # DHT22 temp at bottom
    if dht_temp is not None:
        draw.text((x, y), f"DHT22: {dht_temp:.0f}°", font=status_font, fill=255)

    oled.image(image)
    oled.show()

# Animation Loop
pos_list = [-4, -2, 0, 2, 4, 2, 0, -2]

try:
    while True:
        draw_weather_info()
        time.sleep(LOOPTIME)
        
        start_time = time.time()
        while time.time() - start_time < LOOPTIME:
            for pos in pos_list:
                if time.time() - start_time >= LOOPTIME:
                    break
                draw_eyes(left_offset=pos, right_offset=pos, blink=False)
                time.sleep(0.15)
            draw_eyes(blink=True)
            time.sleep(0.2)
            draw_eyes(blink=False)
            time.sleep(0.2)
except KeyboardInterrupt:
    draw.rectangle((0, 0, WIDTH, HEIGHT), outline=0, fill=0)
    oled.image(image)
    oled.show()
    print("Program stopped.")
