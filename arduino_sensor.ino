#define TINY_GSM_MODEM_SIM800
#define ONE_WIRE_BUS 9

#include <SoftwareSerial.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TinyGPS++.h>

const char* CONTAINER_NUMBER = "TEST_CONT";

static const uint32_t BAUD = 9600;

//Network details
const char apn[]  = "internet.mts.ru";
const char user[] = "mts";
const char pass[] = "mts";
static const int NET_RX = 10, NET_TX = 11;

// MQTT details
const char* broker = "XXX.XXX.XXX.XXX";
const char* topic = "TELEMETRY";
//login
//pass

//GPS details
static const int GPS_RX = 4, GPS_TX = 3;

SoftwareSerial SerialAT(NET_RX, NET_TX);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
TinyGPSPlus gps;
SoftwareSerial gps_serial(GPS_RX, GPS_TX);
PubSubClient mqtt(client);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temp(&oneWire);

void setup()
{
  Serial.begin(9600);
  SerialAT.begin(9600);
  gps_serial.begin(BAUD);
  Serial.println("System start.");
  modem.restart();
  temp.begin();
  }

void loop()
{   
  while (!GPRS_connection()) continue;
  send_data();
  delay(5000);
  }

boolean GPRS_connection()
{
  Serial.println("Connecting to mobile operator");
  if(!modem.waitForNetwork()) return false;
  Serial.println("Signal Quality: " + String(modem.getSignalQuality()));
  Serial.println("Connected to GPRS: " + String(apn));
  mqtt.setServer(broker, 1883);
  return true;
  }

void send_data()
{
  if (!modem.gprsConnect(apn, user, pass)) return;
  Serial.println("Connecting to MQTT Broker: " + String(broker));
  if (!mqtt.connect(CONTAINER_NUMBER)) return;
  char send_string[90];
  String payload = CONTAINER_NUMBER;
  payload.concat(",");
  payload.concat(String(modem.getSignalQuality()));
  payload.concat(",");
  payload.concat(payload_temperature()); //temp
  payload.concat(","); //temp,
  payload.concat(payload_gps()); //,temp,loc_age,lat,lng,date_raw,time_raw,speed,alt
  payload.toCharArray(send_string, payload.length()+1);
  mqtt.publish(topic, send_string);
  delay(3000);
  Serial.println("Connected");    
  modem.gprsDisconnect();
  Serial.println("Disconected");
  delay(10000);
  }

String payload_temperature()
{
 temp.requestTemperatures();
 return String(temp.getTempCByIndex(0));
 }
  
String payload_gps()
{
  while (gps_serial.available() > 0) gps.encode(gps_serial.read());
  //loc_age+lat+lng+date_raw+time_raw+speed+alt
  String payload = String(gps.location.age());
  payload.concat(",");
  payload.concat(String(gps.location.lat(),10));
  payload.concat(",");
  payload.concat(String(gps.location.lng(),10));
  payload.concat(",");
  payload.concat(String(gps.date.value()));
  payload.concat(",");
  payload.concat(String(gps.time.value()));
  payload.concat(",");
  payload.concat(String(gps.speed.kmph()));
  payload.concat(",");
  payload.concat(String(gps.altitude.meters()));
  payload.concat(",");
  payload.concat(String(gps.satellites.value()));
  return payload;
  }


