/* =============== library ================== */
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h> 
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

/* =============== GIS ================== */
static const int rxPinGIS = 15, txPinGIS = 14 ;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(rxPinGIS,txPinGIS);

/* =============== WDT / Transmisi ================== */
#define rxPin 19
#define txPin 18
SoftwareSerial xbee =  SoftwareSerial(rxPin, txPin);

/* =============== suhu & kelembaban tanah ================== */
/*  suhu tanah */

// sensor suhu tanah diletakkan di pin 4
int temp_sensor = 4; 

OneWire oneWire(temp_sensor); 
DallasTemperature sensorSuhuTanah(&oneWire);

/* kelembabab tanah */

// pin sensor
int sensorPin = A0;  

// untuk pengganti VCC
int powerPin = 2;    


/* sensor pH tanah */
#define analogInPin A1  

//variable
int sensorValue = 0;        //ADC value from sensor
float phValue = 0.0;        //pH value after conversion



/* =============== suhu & kelembaban udara ================== */
DHT dht(3, DHT11); //Pin, Jenis DHT



/* =============== Keterangan Node ================== */
String node1 = "Node 1";
String node2 = "Node 2";
String node3 = "Node 3";




void setup(){
 Serial.begin(9600);
// Serial.println("");
// Serial.println(node2+" is Sensing");
 
 dht.begin();

 // jadikan pin power sebagai output
  pinMode(powerPin, OUTPUT);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
   
  // default bernilai LOW
  digitalWrite(powerPin, LOW);

  // mulai komunikasi serial
  xbee.begin(9600);
  ss.begin(9600);
}

void loop(){
   Serial.println("");
   Serial.print("Node pengiriman: ");
   Serial.println(node1);
   Serial.print("Kelembaban Tanah: "); 
   Serial.print(bacaSensorKelembabanTanah());
   Serial.println("^-10 %");
   Serial.print("Kelembaban Udara: ");
   Serial.print(bacaSensorKelembabanUdara());
   Serial.println(" %");
   Serial.print("Suhu Tanah: ");
   Serial.print(bacaSensorSuhuTanah());
   Serial.println(" C");
   Serial.print("Suhu Udara: ");
   Serial.print(bacaSensorSuhuUdara() );
   Serial.println(" C");
   Serial.print("Kadar Keasaman Tanah (pH): ");
   Serial.println(bacaSensorPhTanah());
   Serial.println("");
   Serial.println(WrapperStatusNodeDanSensing());

  /* =============== GIS ================== */

  if (gps.encode(ss.read()))
    displayInfoGIS();

  if (millis() > 3000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS detected: check wiring."));
    
/* =============================== */
  
  String pesan = conversiDataFloatToString();
  
  transmisiPengiriman(pesan);

  if (xbee.available()) {
    byte temp= xbee.read();
    if(temp=='a'||temp=='b'){
      Serial.println(temp);
    }
  }

}
  

  
 

/* =============== Koding ================== */

float bacaSensorKelembabanTanah() {
  // hidupkan power
  digitalWrite(powerPin, HIGH);
  delay(2000);
  
  // baca nilai analog dari sensor
  int nilaiSensor = analogRead(sensorPin);
  digitalWrite(powerPin, LOW);
  
  // makin lembab maka makin tinggi nilai outputnya
  // konversi sensor kelembaban tanah (analog ke digital)
  return (1023 - nilaiSensor);
}

float bacaSensorSuhuUdara(){
  float suhu = dht.readTemperature();
  return suhu;
}

float bacaSensorKelembabanUdara(){
  float kelembaban = dht.readHumidity();
  return kelembaban;
}

float bacaSensorSuhuTanah()
{
   sensorSuhuTanah.requestTemperatures();
   float suhuTanah = sensorSuhuTanah.getTempCByIndex(0);
   return suhuTanah;   
}


float bacaSensorPhTanah(){
  //mengambil data sensing sensor
  sensorValue = analogRead(analogInPin);

  //Mathematical conversion from ADC to pH
  //Konversi analog menjadi digital
  //rumus didapat berdasarkan datasheet 
  phValue = (-0.0693*sensorValue)+7.3855;

  return phValue*-1;
}

String conversiDataFloatToString(){
  String kelembabanTanah = String(bacaSensorKelembabanTanah());
  String kelembabanUdara = String(bacaSensorKelembabanUdara());
  String suhuTanah = String(bacaSensorSuhuTanah());
  String suhuUdara = String(bacaSensorSuhuUdara());
  String phTanah = String(bacaSensorPhTanah());
  String res = node1+"|"+kelembabanTanah+"|"+kelembabanUdara+"|"+suhuTanah+"|"+suhuUdara+"|"+phTanah+"|"+WrapperStatusNodeDanSensing();
  return res;
}


String WrapperStatusNodeDanSensing(){
    boolean isActive = checkStatusNode();
    boolean isSensing = checkStatusSensing();
    String isActiveStr = "";
    String isSensingStr = "";
    String res = "";
    
    if(isActive==true){
       //Serial.println(node2+" is Online");
       isActiveStr = "Online";
    }else{
      //Serial.println(node2+" is Offline");
      isActiveStr = "Offline";
    }

    
    if(isSensing==true){
     //Serial.println(node2+" is Sensing..");
      isSensingStr = "Sensing";
    }else{
      //Serial.println(node2+" is Not Sensing ..");
      isSensingStr = "Not Sensing";
    }

    res = isActiveStr+"|"+isSensingStr;

    return res;
}

String transmisiPengiriman(String pesan){
  xbee.print(pesan);
}

boolean checkStatusNode(){
   if(checkStatusSensing()==true){
      return true;
    }
    else{ //check lampu indikator 'on' arduino
      if(digitalRead(LED_BUILTIN == HIGH)){
        return true;  
      }
      else{
        return false;  
      }
    }
  }

boolean checkStatusSensing(){
  boolean kelembabanTanah = false;
  boolean kelembabanUdara = false;
  boolean suhuTanah = false;
  boolean suhuUdara = false;
  boolean phTanah = true; //tidak memiliki value pasti ketika sensor bermasalah
  boolean isSensing = false;
  
    if(bacaSensorKelembabanTanah()!=1023.0){ //1023.00 adalah value yang didapatkan jika sensor suhu tanah bermasalah
        kelembabanTanah = true;
    }
    if(isnan(bacaSensorKelembabanUdara())==false){ //bukan nan (mengembalikan value), jika bermasalah value = nan
        kelembabanUdara = true;
    }
    if(bacaSensorSuhuTanah()!=-127.00){ //-127.00 adalah value yang didapatkan jika sensor suhu tanah bermasalah
        suhuTanah = true;
    }
    if(isnan(bacaSensorSuhuUdara())==false){ //bukan nan (mengembalikan value), jika bermasalah value = nan
        suhuUdara = true;
    }

    if((kelembabanTanah&&kelembabanUdara&&suhuTanah&&suhuUdara&&phTanah)==true){
      isSensing = true;
    }
    return isSensing;
}




void displayInfoGIS()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  
  Serial.println();
}
