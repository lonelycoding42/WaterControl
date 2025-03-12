#include<WiFi.h>
#include<PubSubClient.h>
#include<ArduinoJson.h>
#include<SimpleDHT.h>

//如果MQTT端需要使用账密登录, 将下一行取消注释
#define need_usr

//WiFi配置
const char *wifi_ssid = "Maker";
const char *wifi_password = "iloveSCU";

//MQTT配置
const char *mqtt_broker="broker-cn.emqx.io";                  //IP地址
const int mqtt_port=1883;                                     //端口
#ifdef need_usr
const char *mqtt_username = "hello";                          //MQTT账号
const char *mqtt_password = "world";                          //MQTT密码
#endif
//主题
const char Topic[11][40] = {
  "esp32/send",
  "esp32/receive"
};
const int Topic_num = 2;                                      //订阅主题数量
char Buffer[100];                                              //发送的消息

WiFiClient espClient;
PubSubClient client(espClient);

//GPIO端口设置
int pump = 1;       //水泵
int DHT = 2;        //温湿度传感器
int SoilMoist = 3;  //土壤湿度传感器

//声明DHT传感器对象
SimpleDHT11 dht11(DHT);

//水泵开关控制
int Switch=0;

//WiFi连接
void WiFi_Connect();

//MQTT设置
void MQTT_Init();

//处理接收到的订阅更新
void callback(char *topic, byte *payload, unsigned int length);

//将数据处理为Json格式并发送
void DataPublish(byte temp, byte humi, float mosi);

void setup() {
  //开启串口通讯
  Serial.begin(115200);

  //联网及MQTT代理连接
  MQTT_Init();

  //GPIO端口设置
  pinMode(pump, OUTPUT);
  pinMode(SoilMoist, INPUT);
}

void loop() {
  //读取空气温湿度
  byte temperature = 0, humidity = 0;
  dht11.read(&temperature, &humidity, NULL);

  //读取土壤湿度
  float moisture = analogRead(SoilMoist);
  
  //判断是否需要启动水泵
  if(Switch){
    digitalWrite(pump, HIGH);
  }
  else{
    digitalWrite(pump, LOW);
  }

  //上传数据
  DataPublish(temperature, humidity, moisture);
  client.loop();
}

/*------------*/
void WiFi_Connect(){
  WiFi.begin(wifi_ssid, wifi_password);
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to WiFi...");
    delay(500);
  }
  Serial.println("Connected to the WiFi network");
}

void callback(char *topic, byte *payload, unsigned int length){
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  Switch = doc["switch"];
}

void MQTT_Init(){
  //连接WiFi
  WiFi_Connect();

  //连接MQTT Broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while(!client.connected()){
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
#ifdef need_usr
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)){
      Serial.println("mqtt broker connected");
    }
#else
    if (client.connect(client_id.c_str())){
      Serial.println("mqtt broker connected");
    }
#endif
    else{
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  //订阅主题
  for(int i=1;i<=Topic_num;i++){
    client.subscribe(Topic[i]);
  }
}

void DataPublish(byte temp, byte humi, float mosi){
  StaticJsonDocument<200> doc;
  doc["Temperature"] = temp;
  doc["Humidity"] = humi;
  doc["SoilMoist"] = mosi;

  serializeJson(doc, Buffer);
  client.publish(Topic[0], Buffer);
}