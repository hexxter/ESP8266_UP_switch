#include <ESP8266WiFi.h>
#include <PubSubClient.h>  // https://github.com/Imroy/pubsubclient
#include <NeoPixelBus.h>
#include <Bounce2.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//extern "C" {
//  #include "user_interface.h"
//  #include "gpio.h"
//  uint16 readvdd33(void);
//  bool wifi_set_sleep_type(enum sleep_type type);
//  enum sleep_type wifi_get_sleep_type(void);
//}

const char *ssid =  "XXXXXX";    // cannot be longer than 32 characters!
const char *password =  "YYYYYYYYYYY";    //
const char* mqtt_server = "ZZZZZZZZZ";
const char* mqtt_username = "UUUUUUUUUU";
const char* mqtt_password = "PPPPPPPPPP";
String MQTT_TOPIC_SUFFIX = "corridor_eg";
String DEVNAME = "switch_1";

#define POWER 5
#define LED 2

#define Btn1 12
#define Btn2 13
#define Btn3 14
#define Btn4 16
// Instantiate a Bounce object
Bounce bonc1 = Bounce();
Bounce bonc2 = Bounce();
Bounce bonc3 = Bounce();
Bounce bonc4 = Bounce(); 

// Neopixel settings
const int numLeds = 1; // change for your setup
const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
NeoPixelBus  leds = NeoPixelBus(numLeds, LED, NEO_GRB | NEO_KHZ800);
int led_r, led_g, led_b = 0;
bool is_dim = true;

#define ONE_WIRE_BUS 4  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
#define REPORTINTERVAL 900000 //mills for next report

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char pl[length+1];
  for(int i = 0; i< length; i++){
    pl[i] = (char)payload[i];
  }
  pl[length] = 0;
  Serial.println( pl );
  String s_pl = String(pl);
  s_pl.trim();

 if(String(topic) == (MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/switch")){
   if( s_pl == "on" ) {
    digitalWrite(POWER, HIGH);
    client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/switch/status").c_str(), "on" );
  }else{
    digitalWrite(POWER, LOW);
    client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/switch/status").c_str(), "off" );
  } 
 }else if(String(topic) == (MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/rgb/set")){
    int c1 = s_pl.indexOf(',');
    int c2 = s_pl.indexOf(',',c1+1);
    led_r = s_pl.toInt();
    led_g = s_pl.substring(c1+1,c2).toInt();
    led_b = s_pl.substring(c2+1).toInt();
    leds.SetPixelColor(0, RgbColor( led_r, led_g, led_b ));
    leds.Show();
    client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/rgb/status").c_str(), s_pl.c_str() );
 }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(("ESP8266Client_"+MQTT_TOPIC_SUFFIX+"_"+DEVNAME).c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement... 
      client.subscribe((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/#").c_str());
     
      Serial.println("MQTT connected");  
    
    } else {
     
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  leds.Begin();
  leds.SetPixelColor(0, RgbColor(0,0,0));
  leds.Show();
  pinMode(Btn1, INPUT);
  pinMode(Btn2, INPUT);
  pinMode(Btn3, INPUT); 
  pinMode(Btn4, INPUT);

  bonc1.attach(Btn1);
  bonc1.interval(100); // interval in ms
  bonc2.attach(Btn2);
  bonc2.interval(100); // interval in ms
  bonc3.attach(Btn3);
  bonc3.interval(100); // interval in ms
  bonc4.attach(Btn4);
  bonc4.interval(100); // interval in ms

  pinMode(POWER, OUTPUT);  
  digitalWrite(POWER, LOW);
 // wifi_set_sleep_type(LIGHT_SLEEP_T);  
  
  // Setup console
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

int dim_count = 0;
bool dim_dir = true;
void do_dim(){
  if( dim_count == 0 ){
    leds.SetPixelColor(0, RgbColor(0,0,0));
    leds.Show();
  }else if( dim_count % 1000 == 0 ){
    int color = dim_count/1000*2;
    leds.SetPixelColor(0, RgbColor(color,color,color));
    leds.Show();
  }
  if( dim_count >= 100000 ){
    dim_dir = false;
  }
  if( dim_count <= 0 ){
    dim_dir = true;
  }
  if( dim_dir ) dim_count++;
  else dim_count--;
}

bool reset_dim = false;
void do_button_color( int but ){
  is_dim = false;
  if( but == 1 ){
    leds.SetPixelColor(0, RgbColor(200,0,0));   
  }else if( but == 2 ){
    leds.SetPixelColor(0, RgbColor(200,200,0)); 
  }else if( but == 3 ){
    leds.SetPixelColor(0, RgbColor(200,0,200)); 
  }else if( but == 4 ){
    leds.SetPixelColor(0, RgbColor(0,0,200)); 
  }
  leds.Show();
  reset_dim = true;
}

bool btn1_state = 0;
bool btn2_state = 0;
bool btn3_state = 0;
bool btn4_state = 0;

int reset_dim_count = 0;

unsigned long time = 0;
float temp;
void loop()
{
  if(!client.connected()){
    Serial.println(client.connected());
    Serial.println( "reconnect" );
    reconnect();
    client.loop();
    delay( 1000 );
  }
  client.loop();

  if( reset_dim ){
    reset_dim_count++;
    delay( 1 );
    if( reset_dim_count == 2000 ){
      reset_dim = false;
      reset_dim_count = 0;
      is_dim = true;
    }
  }

  if( is_dim ){
    do_dim();
  }
  
  bonc1.update();
  bonc2.update();
  bonc3.update();
  bonc4.update();
  
  if( bonc1.read() != btn1_state ){
    btn1_state = bonc1.read();
    //Serial.println( "B1" );
    if( btn1_state ){
      do_button_color( 1 );
      client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/b1/status").c_str(), "push" );
    } 
  }
  if( bonc2.read() != btn2_state ){
    btn2_state = bonc2.read();
    //Serial.println( "B2" );
    if( btn2_state ){
      do_button_color( 2 );
      client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/b2/status").c_str(), "push" );
    }
  }
  if( bonc3.read() != btn3_state ){
    btn3_state = bonc3.read();
    //Serial.println( "B3" );
    if( btn3_state ){
      do_button_color( 3 );
      client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/b3/status").c_str(), "push" );
    }
  }
  if( bonc4.read() != btn4_state ){
    btn4_state = bonc4.read();
    //Serial.println( "B4" );
    if( btn4_state ){
      do_button_color( 4 );
      client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/b4/status").c_str(), "push" );
    }
  }

  if( time < millis() ){
    time = millis()+REPORTINTERVAL;
    DS18B20.requestTemperatures(); 
   
    do {
      temp = DS18B20.getTempCByIndex(0);
    } while (temp == 85.0 || temp == (-127.0));
  
    Serial.print("Temperature ");
    Serial.print(":");
    Serial.println(temp);
    client.publish((MQTT_TOPIC_SUFFIX+"/"+DEVNAME+"/temp/status").c_str(), String(temp).c_str() );   
  }
  
}
