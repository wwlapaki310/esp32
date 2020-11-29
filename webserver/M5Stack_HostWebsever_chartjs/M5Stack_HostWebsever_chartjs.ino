

#include <HTTP_Method.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WebSocketsServer.h> // arduinoWebSocketsライブラリ
#include <elapsedMillis.h> // elapsedMillisライブラリ
#include <SPIFFS.h>
#include "index_html.h" // web server root index
WebServer Server(80); 

#define M5STACK_MPU6886 
// #define M5STACK_MPU9250 
// #define M5STACK_MPU6050
// #define M5STACK_200Q
#include <M5Stack.h>

float accX = 0.0F;
float accY = 0.0F;
float accZ = 0.0F;

float gyroX = 0.0F;
float gyroY = 0.0F;
float gyroZ = 0.0F;

float pitch = 0.0F;
float roll  = 0.0F;
float yaw   = 0.0F;

float temp = 0.0F;

// サンプリング周期
elapsedMillis sensorElapsed;
const unsigned long DELAY = 1000; // ms



// Webサーバー 192.168.4.1:80
WebServer webServer(80); // 80番ポート
// Websocketサーバー 192.68.4.1:81
WebSocketsServer webSocket = WebSocketsServer(81); // 81番ポート

// WiFi設定
const char *ssid = "ESP32AP-WiFi"; // 各自のSSIDを入力
const char *password = "esp32apwifi"; //各自のパスワードを入力
const IPAddress ip(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);



// Webコンテンツのイベントハンドラ
void handleRoot() {
  String s = INDEX_HTML; // index_html.hより読み込み
  Serial.printf("%s",s);
  webServer.send(200, "text/html", s);
}
void handleNotFound() {
  webServer.send(404, "text/plain", "File not found.");
  Serial.printf("not");
}

//==========================================
// (3) センサの初期化
//==========================================
void sensor_init() {
  //htu21d.begin(); // 温度・湿度センサの初期化
}

// センサのデータ(JSON形式)
const char SENSOR_JSON[] PROGMEM = R"=====({"val1":%.1f})=====";

// データの更新
void sensor_loop() {
   M5.IMU.getGyroData(&gyroX,&gyroY,&gyroZ);
  M5.IMU.getAccelData(&accX,&accY,&accZ);
  M5.IMU.getAhrsData(&pitch,&roll,&yaw);
  M5.IMU.getTempData(&temp);
  
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.printf("%6.2f  %6.2f  %6.2f      ", gyroX, gyroY, gyroZ);
  M5.Lcd.setCursor(220, 42);
  M5.Lcd.print(" o/s");
  M5.Lcd.setCursor(0, 65);
  M5.Lcd.printf(" %5.2f   %5.2f   %5.2f   ", accX, accY, accZ);
  M5.Lcd.setCursor(220, 87);
  M5.Lcd.print(" G");
  M5.Lcd.setCursor(0, 110);
  M5.Lcd.printf(" %5.2f   %5.2f   %5.2f   ", pitch, roll, yaw);
  M5.Lcd.setCursor(220, 132);
  M5.Lcd.print(" degree");
  M5.Lcd.setCursor(0, 155);
  M5.Lcd.printf("Temperature : %.2f C", temp);
  //Serial.printf("%6.2f, %6.2f, %6.2f\r\n", roll, pitch, yaw);
  
  char payload[16];
//=============================================
// (4) センシング
  //float temp = htu21d.readTemperature();
  snprintf_P(payload, sizeof(payload), SENSOR_JSON, temp);
//============================================= 
  Serial.printf(payload);

  // WebSocketでデータ送信(全端末へブロードキャスト)
  webSocket.broadcastTXT(payload, strlen(payload));
//  Serial.println(payload);
}

void setup() {
  // シリアル通信設定
  Serial.begin(115200);
  delay(100);

  
  M5.begin();
  M5.Power.begin();
  M5.IMU.Init();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN , BLACK);
  M5.Lcd.setTextSize(2);

  // Wi-Fi設定
  WiFi.disconnect(true);
  delay(1000);
  WiFi.softAP(ssid, password);
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);
  IPAddress myIP = WiFi.softAPIP();

  //sensor_init(); // センサの初期化

  // Webサーバーのコンテンツ設定
  // favicon.ico, Chart.min.jsは dataフォルダ内に配置
  SPIFFS.begin();
  webServer.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  webServer.serveStatic("/Chart.min.js", SPIFFS, "/Chart.min.js");
  webServer.on("/", handleRoot);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  // WebSocketサーバー開始
  webSocket.begin();
}

void loop(void) {
  webSocket.loop();
  webServer.handleClient();

  // 一定の周期でセンシング
  if (sensorElapsed > DELAY) {
    sensorElapsed = 0;
    sensor_loop();
  }
}
