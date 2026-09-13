// ============================================================
// SWITCH ZERO
// Date: 5th September 2026
// Author(s): Y. Panigrahi
// Connections: I. Rajput
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <BluetoothSerial.h>

#include <SPI.h>
#include <MFRC522.h>


// ============================================================
// OLED
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);


// ============================================================
// BUTTONS
// ============================================================

#define BUTTON_UP     25
#define BUTTON_DOWN   26
#define BUTTON_LEFT   27
#define BUTTON_RIGHT  33


// ============================================================
// RFID
// MFRC522
// ============================================================

#define RFID_SS   5
#define RFID_RST  4

MFRC522 rfid(RFID_SS, RFID_RST);


// ============================================================
// BLUETOOTH
// ============================================================

BluetoothSerial SerialBT;
BLEScan *bleScan;


// ============================================================
// MENU
// ============================================================

const int MENU_ITEMS = 5;

const char *menuItems[MENU_ITEMS] = {
  "Wi-Fi Scan",
  "BLE Scan",
  "BT Classic",
  "RFID Read",
  "About"
};

int menuIndex = 0;


// ============================================================
// MENU VIEWPORT
// ============================================================

const int VISIBLE_MENU_ITEMS = 4;

int menuTop = 0;


// ============================================================
// WAIT FOR SPECIFIC BUTTON RELEASE
//
// Pressed  = HIGH / 1
// Released = LOW  / 0
//
// No debounce.
// No edge detection.
// ============================================================

void waitForSpecificRelease(char button)
{
  int pin;

  if (button == 'U')
    pin = BUTTON_UP;

  else if (button == 'D')
    pin = BUTTON_DOWN;

  else if (button == 'L')
    pin = BUTTON_LEFT;

  else
    pin = BUTTON_RIGHT;


  while (digitalRead(pin) == HIGH)
  {
    delay(10);
  }
}


// ============================================================
// WAIT FOR BUTTON
// ============================================================

char waitForButton()
{
  while (true)
  {
    if (digitalRead(BUTTON_LEFT) == HIGH)
    {
      waitForSpecificRelease('L');
      return 'L';
    }

    if (digitalRead(BUTTON_RIGHT) == HIGH)
    {
      waitForSpecificRelease('R');
      return 'R';
    }

    if (digitalRead(BUTTON_UP) == HIGH)
    {
      waitForSpecificRelease('U');
      return 'U';
    }

    if (digitalRead(BUTTON_DOWN) == HIGH)
    {
      waitForSpecificRelease('D');
      return 'D';
    }

    delay(10);
  }
}


// ============================================================
// STARTUP SCREEN
// ============================================================

void startupScreen()
{
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(20, 10);
  display.println("SWITCH");

  display.setCursor(35, 35);
  display.println("ZERO");

  display.display();

  delay(2000);
}


// ============================================================
// DRAW MENU
//
// Header is ALWAYS fixed.
// Only the menu items move.
// ============================================================

void drawMenu(int topItem, int selectedItem, int offset)
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);


  // ==========================================================
  // FIXED HEADER
  // ==========================================================

  display.setCursor(0, 0);
  display.println("=== SWITCH ZERO ===");


  // ==========================================================
  // MENU ITEMS
  // ==========================================================

  for (int i = 0; i < VISIBLE_MENU_ITEMS + 1; i++)
  {
    int itemIndex = topItem + i;

    if (itemIndex < 0 || itemIndex >= MENU_ITEMS)
      continue;


    int y = 14 + (i * 12) + offset;


    // Don't draw outside the menu area
    if (y < 10 || y > 63)
      continue;


    display.setCursor(0, y);


    if (itemIndex == selectedItem)
      display.print("> ");
    else
      display.print("  ");


    display.println(menuItems[itemIndex]);
  }


  display.display();
}


// ============================================================
// SHOW MENU
// ============================================================

void showMenu()
{
  if (menuIndex < menuTop)
    menuTop = menuIndex;


  if (menuIndex >= menuTop + VISIBLE_MENU_ITEMS)
    menuTop = menuIndex - VISIBLE_MENU_ITEMS + 1;


  drawMenu(menuTop, menuIndex, 0);
}


// ============================================================
// ANIMATED MENU SCROLL
// ============================================================

void animateMenuScroll(
  int oldTop,
  int newTop,
  int oldIndex,
  int newIndex
)
{
  if (oldTop == newTop)
  {
    drawMenu(newTop, newIndex, 0);
    return;
  }


  // ==========================================================
  // Determine scroll direction
  // ==========================================================

  int direction;

  if (newTop > oldTop)
    direction = -1;     // Move list upward

  else
    direction = 1;      // Move list downward


  // ==========================================================
  // Animate
  // ==========================================================

  for (int offset = 0; offset <= 12; offset += 2)
  {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);


    // ========================================================
    // FIXED HEADER
    // ========================================================

    display.setCursor(0, 0);
    display.println("=== SWITCH ZERO ===");


    // ========================================================
    // OLD VIEW + NEW VIEW
    // ========================================================

    for (int i = 0; i < VISIBLE_MENU_ITEMS + 1; i++)
    {
      int itemIndex = oldTop + i;

      if (itemIndex < 0 || itemIndex >= MENU_ITEMS)
        continue;


      int y =
        14 +
        (i * 12) +
        (direction * offset);


      if (y < 10 || y > 63)
        continue;


      display.setCursor(0, y);


      if (itemIndex == newIndex)
        display.print("> ");
      else
        display.print("  ");


      display.println(menuItems[itemIndex]);
    }


    // ========================================================
    // Draw entering item during scroll
    // ========================================================

    if (newTop != oldTop)
    {
      int enteringIndex;

      if (direction == -1)
        enteringIndex = newTop + VISIBLE_MENU_ITEMS - 1;

      else
        enteringIndex = newTop;


      if (enteringIndex >= 0 && enteringIndex < MENU_ITEMS)
      {
        int y;

        if (direction == -1)
        {
          y = 14 + (VISIBLE_MENU_ITEMS * 12) - offset;
        }
        else
        {
          y = 14 - 12 + offset;
        }


        if (y >= 10 && y <= 63)
        {
          display.setCursor(0, y);

          if (enteringIndex == newIndex)
            display.print("> ");
          else
            display.print("  ");

          display.println(menuItems[enteringIndex]);
        }
      }
    }


    display.display();

    delay(20);
  }


  // ==========================================================
  // Final frame
  // ==========================================================

  menuTop = newTop;

  drawMenu(menuTop, newIndex, 0);
}


// ============================================================
// MOVE MENU UP
// ============================================================

void moveMenuUp()
{
  int oldIndex = menuIndex;
  int oldTop = menuTop;


  menuIndex--;


  if (menuIndex < 0)
    menuIndex = MENU_ITEMS - 1;


  int newTop = oldTop;


  if (menuIndex < newTop)
    newTop = menuIndex;


  if (menuIndex >= newTop + VISIBLE_MENU_ITEMS)
    newTop = menuIndex - VISIBLE_MENU_ITEMS + 1;


  animateMenuScroll(
    oldTop,
    newTop,
    oldIndex,
    menuIndex
  );


  menuTop = newTop;
}


// ============================================================
// MOVE MENU DOWN
// ============================================================

void moveMenuDown()
{
  int oldIndex = menuIndex;
  int oldTop = menuTop;


  menuIndex++;


  if (menuIndex >= MENU_ITEMS)
    menuIndex = 0;


  int newTop = oldTop;


  if (menuIndex < newTop)
    newTop = menuIndex;


  if (menuIndex >= newTop + VISIBLE_MENU_ITEMS)
    newTop = menuIndex - VISIBLE_MENU_ITEMS + 1;


  animateMenuScroll(
    oldTop,
    newTop,
    oldIndex,
    menuIndex
  );


  menuTop = newTop;
}


// ============================================================
// WIFI SCAN
// ============================================================

void wifiScan()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("WIFI SCANNING...");

  display.setCursor(0, 20);
  display.println("Please wait...");

  display.display();


  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);


  int networkCount = WiFi.scanNetworks();


  // ==========================================================
  // NO NETWORKS
  // ==========================================================

  if (networkCount <= 0)
  {
    display.clearDisplay();

    display.setCursor(0, 15);
    display.println("NO NETWORKS");

    display.setCursor(0, 35);
    display.println("LEFT = BACK");

    display.display();


    while (true)
    {
      char button = waitForButton();

      if (button == 'L')
      {
        WiFi.scanDelete();
        return;
      }
    }
  }


  // ==========================================================
  // DISPLAY NETWORKS
  // ==========================================================

  for (int i = 0; i < networkCount; i++)
  {
    String ssid = WiFi.SSID(i);

    int rssi = WiFi.RSSI(i);

    int channel = WiFi.channel(i);


    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.print("WIFI ");
    display.print(i + 1);
    display.print("/");
    display.println(networkCount);


    display.setCursor(0, 14);

    if (ssid.length() > 0)
      display.println(ssid);

    else
      display.println("<Hidden>");


    display.setCursor(0, 29);

    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");


    display.setCursor(0, 42);

    display.print("CH: ");
    display.println(channel);


    display.setCursor(0, 54);

    display.println("R=NEXT  L=BACK");


    display.display();


    while (true)
    {
      char button = waitForButton();


      if (button == 'L')
      {
        WiFi.scanDelete();
        return;
      }


      if (button == 'R')
      {
        break;
      }
    }
  }


  WiFi.scanDelete();


  display.clearDisplay();

  display.setCursor(0, 20);
  display.println("END OF RESULTS");

  display.setCursor(0, 40);
  display.println("LEFT = BACK");

  display.display();


  while (true)
  {
    char button = waitForButton();

    if (button == 'L')
      return;
  }
}


// ============================================================
// BLE SCAN
// ============================================================

void bleScanDevices()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("BLE SCANNING...");

  display.setCursor(0, 20);
  display.println("Bluetooth 4.x");

  display.display();


  BLEScanResults *results =
    bleScan->start(5, false);


  int deviceCount =
    results->getCount();


  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("BLE SCAN COMPLETE");

  display.setCursor(0, 20);

  display.print("DEVICES: ");
  display.println(deviceCount);


  display.setCursor(0, 45);
  display.println("RIGHT = VIEW");

  display.display();


  delay(1000);


  // ==========================================================
  // DISPLAY BLE DEVICES
  // ==========================================================

  for (int i = 0; i < deviceCount; i++)
  {
    BLEAdvertisedDevice device =
      results->getDevice(i);


    String name = "Unknown";


    if (device.haveName())
      name = device.getName().c_str();


    String address =
      device.getAddress().toString().c_str();


    int rssi =
      device.getRSSI();


    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.print("BLE ");
    display.print(i + 1);
    display.print("/");
    display.println(deviceCount);


    display.setCursor(0, 14);


    if (name.length() > 20)
      display.println(name.substring(0, 20));

    else
      display.println(name);


    display.setCursor(0, 29);

    display.println(address);


    display.setCursor(0, 44);

    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");


    display.setCursor(0, 55);

    display.println("R=NEXT L=BACK");


    display.display();


    while (true)
    {
      char button = waitForButton();


      if (button == 'L')
      {
        bleScan->clearResults();
        return;
      }


      if (button == 'R')
      {
        break;
      }
    }
  }


  bleScan->clearResults();


  display.clearDisplay();

  display.setCursor(0, 20);
  display.println("END OF RESULTS");

  display.setCursor(0, 40);
  display.println("LEFT = BACK");

  display.display();


  while (true)
  {
    char button = waitForButton();

    if (button == 'L')
      return;
  }
}


// ============================================================
// BLUETOOTH CLASSIC SCAN
// ============================================================

void bluetoothClassicScan()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("BT CLASSIC SCAN");

  display.setCursor(0, 20);
  display.println("Scanning...");

  display.display();


  BTScanResults *results =
    SerialBT.discover(10000);


  if (results == nullptr)
  {
    display.clearDisplay();

    display.setCursor(0, 15);
    display.println("BT SCAN FAILED");

    display.setCursor(0, 35);
    display.println("LEFT = BACK");

    display.display();


    while (true)
    {
      char button = waitForButton();

      if (button == 'L')
        return;
    }
  }


  int count =
    results->getCount();


  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("BT SCAN COMPLETE");

  display.setCursor(0, 20);

  display.print("DEVICES: ");
  display.println(count);


  display.setCursor(0, 45);
  display.println("RIGHT = VIEW");

  display.display();


  delay(1000);


  // ==========================================================
  // DISPLAY CLASSIC BT DEVICES
  // ==========================================================

  for (int i = 0; i < count; i++)
  {
    BTAdvertisedDevice *device =
      results->getDevice(i);


    String info =
      device->toString().c_str();


    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.print("BT ");
    display.print(i + 1);
    display.print("/");
    display.println(count);


    display.setCursor(0, 15);


    if (info.length() > 20)
      display.println(info.substring(0, 20));

    else
      display.println(info);


    display.setCursor(0, 40);

    display.println("R=NEXT L=BACK");


    display.display();


    while (true)
    {
      char button = waitForButton();


      if (button == 'L')
        return;


      if (button == 'R')
        break;
    }
  }


  display.clearDisplay();

  display.setCursor(0, 20);
  display.println("END OF RESULTS");

  display.setCursor(0, 40);
  display.println("LEFT = BACK");

  display.display();


  while (true)
  {
    char button = waitForButton();

    if (button == 'L')
      return;
  }
}


// ============================================================
// RFID READ ONLY
// ============================================================

void rfidRead()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("RFID READ");

  display.setCursor(0, 18);
  display.println("Waiting for tag...");

  display.setCursor(0, 45);
  display.println("LEFT = BACK");

  display.display();


  while (true)
  {
    // ========================================================
    // LEFT = BACK
    // ========================================================

    if (digitalRead(BUTTON_LEFT) == HIGH)
    {
      waitForSpecificRelease('L');
      return;
    }


    // ========================================================
    // CHECK FOR RFID TAG
    // ========================================================

    if (!rfid.PICC_IsNewCardPresent())
    {
      delay(50);
      continue;
    }


    if (!rfid.PICC_ReadCardSerial())
    {
      delay(50);
      continue;
    }


    // ========================================================
    // BUILD UID
    // ========================================================

    String uidString = "";


    for (byte i = 0; i < rfid.uid.size; i++)
    {
      if (rfid.uid.uidByte[i] < 0x10)
        uidString += "0";


      uidString +=
        String(rfid.uid.uidByte[i], HEX);


      if (i < rfid.uid.size - 1)
        uidString += " ";
    }


    uidString.toUpperCase();


    // ========================================================
    // CARD TYPE
    // ========================================================

    MFRC522::PICC_Type piccType =
      rfid.PICC_GetType(rfid.uid.sak);


    String cardType =
      String(rfid.PICC_GetTypeName(piccType));


    // ========================================================
    // DISPLAY TAG
    // ========================================================

    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("RFID TAG FOUND");


    display.setCursor(0, 14);
    display.println("UID:");


    display.setCursor(0, 26);

    if (uidString.length() > 21)
      display.println(uidString.substring(0, 21));

    else
      display.println(uidString);


    display.setCursor(0, 40);

    if (cardType.length() > 20)
      display.println(cardType.substring(0, 20));

    else
      display.println(cardType);


    display.setCursor(0, 54);
    display.println("LEFT = BACK");


    display.display();


    // ========================================================
    // STOP COMMUNICATION WITH CARD
    //
    // READ ONLY.
    // No writing.
    // No cloning.
    // No modification.
    // ========================================================

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();


    // ========================================================
    // WAIT FOR TAG TO BE REMOVED
    // Prevent same tag from displaying repeatedly.
    // ========================================================

    while (rfid.PICC_IsNewCardPresent())
    {
      if (digitalRead(BUTTON_LEFT) == HIGH)
      {
        waitForSpecificRelease('L');
        return;
      }

      delay(100);
    }


    // Return to waiting screen
    display.clearDisplay();

    display.setCursor(0, 0);
    display.println("RFID READ");

    display.setCursor(0, 18);
    display.println("Waiting for tag...");

    display.setCursor(0, 45);
    display.println("LEFT = BACK");

    display.display();
  }
}


// ============================================================
// ABOUT
// ============================================================

void showAbout()
{
  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("=== SWITCH ZERO ===");


  display.setCursor(0, 14);
  display.println("ESP32 Multi Scanner");


  display.setCursor(0, 27);
  display.println("Wi-Fi");


  display.setCursor(0, 38);
  display.println("BLE / BT 4.x");


  display.setCursor(0, 49);
  display.println("BT Classic");


  display.display();


  while (true)
  {
    char button = waitForButton();

    if (button == 'L')
      return;
  }
}


// ============================================================
// SELECT MENU ITEM
// ============================================================

void selectMenuItem()
{
  switch (menuIndex)
  {
    case 0:
      wifiScan();
      break;


    case 1:
      bleScanDevices();
      break;


    case 2:
      bluetoothClassicScan();
      break;


    case 3:
      rfidRead();
      break;


    case 4:
      showAbout();
      break;
  }
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);


  // ==========================================================
  // OLED
  // ==========================================================

  Wire.begin(21, 22);


  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C))
  {
    Serial.println("OLED not found!");

    while (1);
  }


  // ==========================================================
  // BUTTONS
  //
  // Pressed  = HIGH / 1
  // Released = LOW  / 0
  // ==========================================================

  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);

  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);


  // ==========================================================
  // STARTUP
  // ==========================================================

  startupScreen();


  // ==========================================================
  // WIFI
  // ==========================================================

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);


  // ==========================================================
  // BLE
  // ==========================================================

  BLEDevice::init("");

  bleScan = BLEDevice::getScan();

  bleScan->setActiveScan(true);

  bleScan->setInterval(100);

  bleScan->setWindow(80);


  // ==========================================================
  // BLUETOOTH CLASSIC
  // ==========================================================

  SerialBT.begin("Switch Zero");


  // ==========================================================
  // RFID / SPI
  //
  // SCK  = GPIO 18
  // MISO = GPIO 19
  // MOSI = GPIO 23
  // SS   = GPIO 5
  // RST  = GPIO 4
  // ==========================================================

  SPI.begin(
    18,
    19,
    23,
    RFID_SS
  );


  rfid.PCD_Init();

  delay(50);


  // ==========================================================
  // MENU
  // ==========================================================

  menuIndex = 0;
  menuTop = 0;

  showMenu();
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  char button = waitForButton();


  // ==========================================================
  // UP
  // ==========================================================

  if (button == 'U')
  {
    moveMenuUp();
  }


  // ==========================================================
  // DOWN
  // ==========================================================

  else if (button == 'D')
  {
    moveMenuDown();
  }


  // ==========================================================
  // RIGHT = SELECT
  // ==========================================================

  else if (button == 'R')
  {
    selectMenuItem();

    showMenu();
  }


  // ==========================================================
  // LEFT
  // ==========================================================

  else if (button == 'L')
  {
    showMenu();
  }
}