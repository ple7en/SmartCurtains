#include <FS.h>               //this needs to be first, or it all crashes and burns...
#define BLYNK_PRINT Serial//Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>      //https://github.com/bblanchon/ArduinoJson
#include <SPI.h>
#include <SimpleTimer.h>
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>
#include <AccelStepper.h>

#define IN1 5
#define IN2 0
#define IN3 4
#define IN4 2

AccelStepper s28BYJ(8, IN1, IN3, IN2, IN4);



bool shouldSaveConfig = false;//флаг для сохранения данных
char blynk_token[34] = "";

SimpleTimer timer;





void saveConfigCallback () {//обратный вызов, уведомляющий нас о необходимости сохранить конфигурацию
  Serial.println("Данные требуется сохранить, т.к. внесены изменения");//Следует сохранить конфигурацию
  shouldSaveConfig = true;//Меняем флаг shouldSaveConfig на true
}


void setup()
{

  Serial.begin(115200);
  Serial.println();



//чистая ФС, для тестирования
//SPIFFS.format();

  Serial.println("Монтируем файловую систему...");////читаем конфигурацию из FS json

  if (SPIFFS.begin()) {
    Serial.println("Файловая система смонтирована");
    if (SPIFFS.exists("/config.json")) {
  //файл существует, чтение и загрузка
      Serial.println("Чтение конфигурационного файла");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("Конфигурационный файл открыт");
        size_t size = configFile.size();
    //Выделить буфер для хранения содержимого файла
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");          
          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("Не удалось загрузить конфигурацию json");
        }
      }
    }
  } else {
    Serial.println("Неудалось смонтировать файловую систему");
  }

  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 33);//was 32 length
  
  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);//установить конфигурацию сохранить уведомить обратный вызов

  
  wifiManager.addParameter(&custom_blynk_token);
  strcpy(blynk_token, custom_blynk_token.getValue());//прочтем обновленные параметры
  String S = blynk_token;
   
    if (S.length() == 0)
    {
      wifiManager.resetSettings();      
    }

      
    if (!wifiManager.autoConnect("SmartCurtains", "12345678")) {//Задайте здесь параметры точки доступа (SSID, password)
    Serial.println("Не удалось подключиться и истекло время ожидания");
    delay(3000);//перезагрузите и попробуйте снова, или, возможно, положить его в глубокий сон
    ESP.reset();
    delay(5000);
   }

  Serial.println("Подключение... :)");//if you get here you have connected to the WiFi

  strcpy(blynk_token, custom_blynk_token.getValue());//прочтем обновленные параметры
  
//сохранить пользовательские параметры в FS
  if (shouldSaveConfig) {  //прочтем обновленные параметры
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Не удалось открыть файл конфигурации для записи");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
//конец сохранения
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  
  if (WiFi.status() == WL_CONNECTED) {

  Blynk.config(blynk_token);
  Blynk.connect();
  Serial.println("соединение с WiFi-сетью успешно установлено"); 
  Serial.print("Подключение к Blynk ");
  Serial.println(Blynk.connected());
  }
  else
  {
    Serial.println("Нет соединения с WiFi"); 
  }

Blynk.virtualWrite(V3, 600);
Blynk.virtualWrite(V4, 350);
Blynk.virtualWrite(V5, 2000);


}


int StepButton = 0;

BLYNK_WRITE(V1)
{  
  StepButton = param.asInt();
  if (StepButton>0)
  {
    StepButton=StepButton+9999999999999999999999;
  }
  s28BYJ.move(StepButton);
}

BLYNK_WRITE(V2)
{  
  StepButton = param.asInt();
  if (StepButton<0)
  {
    StepButton=StepButton-9999999999999999999999;
  }
  s28BYJ.move(StepButton);
}
  

  BLYNK_WRITE(V3)
  {
    // Задаём максимальную скорость двигателя
    s28BYJ.setMaxSpeed(param.asInt());
  }
  BLYNK_WRITE(V4)
  {
    // Задаём ускорение двигателя
    s28BYJ.setAcceleration(param.asInt());
  }
  BLYNK_WRITE(V5)
  {
    // Установим скорость в шагах за секунду
    s28BYJ.setSpeed(param.asInt());
  }
   
  



  void loop()
{

 // Blynk.syncVirtual(V1);
  Blynk.run();//Initiates Blynk
  timer.run();//Initiates SimpleTimer  
  s28BYJ.run();


}
