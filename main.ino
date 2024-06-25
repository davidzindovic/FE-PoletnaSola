#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK D5
#define BME_MISO D7
#define BME_MOSI D6
#define BME_CS D8
#define SEALEVELPRESSURE_HPA (1013.25)

#define TIPKA_MODE D0
#define TIPKA_A D9
#define TIPKA_B D10

#define ST_VZORCEV 30
#define ST_VELICIN 7
#define PROSTOR_ZA_MOJE_MERITVE 10 //lahko pomni največ 10 meritev lastnih

#define TIPKE_TIME_THRESHOLD 200 //v ms


uint16_t povprecja[ST_VELICIN][ST_VZORCEV]={0}; //tabela preteklih meritev, iz kjer računamo povprečje
uint32_t moje_meritve_vrednosti[PROSTOR_ZA_MOJE_MERITVE][2];//hrani v prvem stolpcu izmerjeno vrednost, v drugem cas zadnje meritve
String moje_meritve_napisi[PROSTOR_ZA_MOJE_MERITVE][2];    //hrani v prvem stolpcu ime velicine, v drugem pa enote

String velicine[ST_VELICIN]={"Loudness: ","Humidity: ","Bright: ","Temperature: ","Pressure: ","Gas: ","Altitude: "};
String enote[ST_VELICIN]={"Db","%","lx","C","hPa","k","m"};

uint8_t kazalec=0; //da vem katera meritva je bila zadnja

byte stopinje[8] = {
  0b01100,
  0b10010,
  0b01100,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte ohm[8] = {
  0b00000,
  0b00000,
  0b01110,
  0b10001,
  0b10001,
  0b11011,
  0b01010,
  0b11011
};

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

void setup() {

pinMode(TIPKA_MODE,INPUT_PULLUP);
pinMode(TIPKA_A,INPUT_PULLUP);
pinMode(TIPKA_B,INPUT_PULLUP);

pinMode(D3,OUTPUT); //en izbor za mux
pinMode(D4,OUTPUT); //drug izbor za mux (ni se mi dal negatorja dewat)
pinMode(A0,INPUT);  //mam sam en adc
Serial.begin(115200);
digitalWrite(D3,0);
digitalWrite(D4,1);

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

lcd.init();
lcd.backlight();
}

void loop() {

//spremenljivka za menjavanje kanala za prikaz
//ko prideš v nek mode se spremeni, da prideš ven
//moraš pritisniti b(back_button)
static bool window_pointer=1;//dokler je 1 lahko greš v mode
static uint8_t izbrana_velicina=0;
static uint8_t moje_meritve_pointer=0;
static bool moje_meritve_ptr_of=0; //za spremljanje ponovnega poteka

//maske za mode:
static uint8_t mask_mode_scroll=0b100;
static uint8_t mask_save_scroll=0b001; // scrolla po saveanih meritvah, napiše čas od meritve 
static uint8_t mask_back_ok_button=0b010;
static uint8_t mask_save=0b011;
static uint8_t mask_wifi_publish=0b110; //poskusi se dat na wifi in publishat
static uint8_t mask_display_toggle=0b101;


uint8_t TIPKE=fronte_tipk();
//da ne gledamo pogojev če ni treba:
if(TIPKE>0){
if(TIPKE&mask_display_toggle)
{
  static bool display_on=1;
  display_on=!display_on;
  if(display_on)lcd.backlight();
  else lcd.noBacklight();
}
else if((TIPKE&mask_wifi_publish)&&(window_pointer))
{ window_pointer=0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Publishing...");
  
  if(wifi_publish(velicine[izbrana_velicina],povprecja[izbrana_velicina][kazalec]))
  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Successfully");
  lcd.setCursor(0,1);
  lcd.print("published!");
  }
  else
  {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Failed to");
  lcd.setCursor(0,1);
  lcd.print("publish :(");
  }
}
else if((TIPKE&mask_save)&&(window_pointer))
{ window_pointer=0;
  moje_meritve_napisi[moje_meritve_pointer][0]=velicine[izbrana_velicina];//zapišem v tabelo "ime veličine"
  moje_meritve_vrednosti[moje_meritve_pointer][0]=povprecja[izbrana_velicina][kazalec]; //prepišem zadnjo vrednost
  moje_meritve_napisi[moje_meritve_pointer][1]=enote[izbrana_velicina];
  moje_meritve_vrednosti[moje_meritve_pointer][1]=millis();
  moje_meritve_pointer++;
  if(moje_meritve_pointer==PROSTOR_ZA_MOJE_MERITVE){moje_meritve_pointer=0;moje_meritve_ptr_of=1;}
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("You saved the");
  lcd.setCursor(0,1);
  lcd.print("measurment!");
  
}
else if((TIPKE&mask_back_ok_button)&&(!window_pointer))
{
  window_pointer=1; //vrne na default window value, kar pomeni da smo na home screen
}
else if((TIPKE&mask_save_scroll)&&(window_pointer))
{ static uint8_t save_scroll_pointer=0;
  window_pointer=0;
  save_scroll_pointer++; //tukaj imam 1, v funkciji zbije na 0 (torej -1)
  if((((save_scroll_pointer-1)==moje_meritve_pointer)&&(!moje_meritve_ptr_of))||((save_scroll_pointer)==PROSTOR_ZA_MOJE_MERITVE))save_scroll_pointer=1; //resetam na 1
  IzpisLastnihMeritev(save_scroll_pointer);
}
else if((TIPKE&mask_mode_scroll)&&(window_pointer))
{
  izbrana_velicina++;
  if(izbrana_velicina==ST_VELICIN)izbrana_velicina=0;
}
}

//izven checkanja tipk:
if(window_pointer)IzpisNaEkran(izbrana_velicina,povprecja[izbrana_velicina][kazalec],1); //povprecje skos izpisuje ker mi je zmanjkal tipk lol


//test izpisa arraya velicin (samo napisov):
//for (uint8_t i=0;i<ST_VELICIN;i++){IzpisNaEkran(i,(uint16_t)i*4,0);delay(500);lcd.clear();}

}


//velicina je pozicija velicine v arrayu, ne pa lastnih meritev
void IzpisNaEkran(uint8_t velicina_index, uint16_t vrednost,bool avrg)
{ lcd.clear();
  uint8_t unit_rezerva; //+1 za presledek med vrednostjo in enoto
  if((velicina_index==0) || (velicina_index==2) || (velicina_index==3)||(velicina_index==5)) unit_rezerva=2+1;
  else if(velicina_index==4)unit_rezerva=3+1;
  else unit_rezerva=1+1;
  
  lcd.setCursor(0, 0);
  lcd.print(velicine[velicina_index]);
  lcd.setCursor(16-(vrednost>=0)-(vrednost>9)-(vrednost>99)-unit_rezerva,0); //rezerviramo dovolj števk
  lcd.print(vrednost);
  lcd.setCursor(16-unit_rezerva+1,0); //-1 da bo presledek

  if(velicina_index==3)
  {
    lcd.createChar(0, stopinje);
    lcd.setCursor(16-unit_rezerva+1,0);
    lcd.write((byte)0);
    lcd.print(enote[velicina_index]);
  }
  else if(velicina_index==5)
    {
    lcd.createChar(0, ohm);
    lcd.setCursor(16-unit_rezerva+1,0);
    lcd.print(enote[velicina_index]);
    lcd.setCursor(16-unit_rezerva+2,0);
    lcd.write((byte)0);
  }
  else lcd.print(enote[velicina_index]);

  if(avrg)
  {
    uint8_t index=0;
    uint32_t povprecje=0;

    while(povprecja[velicina_index][index]==0 || index>=ST_VZORCEV)
    {
      povprecje+=povprecja[velicina_index][index];
      index++;
    }
    povprecje/=index;
    lcd.setCursor(0,1);
    lcd.print("Average: ");
    lcd.setCursor(11,1);
    lcd.print(povprecje);
    lcd.setCursor(11+povprecje>=0+povprecje>9+povprecje>99+1,1);
    lcd.print(enote[velicina_index]);
  }
  
}

void IzpisLastnihMeritev(uint8_t SaveScrool_ptr)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(moje_meritve_napisi[SaveScrool_ptr-1][0]); //izpis imena velicine
  lcd.setCursor(16-(moje_meritve_vrednosti[SaveScrool_ptr-1][0]>=0)-(moje_meritve_vrednosti[SaveScrool_ptr-1][0]>9)-(moje_meritve_vrednosti[SaveScrool_ptr-1][0]>99)-3,0); //rezerva je 3 ker je hPa najdaljša enota, jbg
  lcd.print(moje_meritve_vrednosti[SaveScrool_ptr-1][0]); //izpis izmerjene vrednosti
  lcd.setCursor(16-3,0); //rezerva je 3 ker je hPa najdaljša enota, jbg
  lcd.print(moje_meritve_napisi[SaveScrool_ptr-1][1]); //izpis enote
  lcd.setCursor(0,1);
  lcd.print("Time past: ");
  uint32_t sekunde=millis()/1000-moje_meritve_vrednosti[SaveScrool_ptr-1][1]/1000-moje_meritve_vrednosti[SaveScrool_ptr-1][1]/1000/60*60;
  uint32_t minute=millis()/1000/60-moje_meritve_vrednosti[SaveScrool_ptr-1][1]/1000/60;
  lcd.setCursor(11+4-(sekunde>9)-2*(minute>0)-(minute>9),1); //plac za minute rabi se enoto zato 2*
  if(minute>0)
  {
    lcd.print(minute);
    lcd.setCursor(11+4-(sekunde>9)-2*(minute>0)-(minute>9)+(minute>0)+(minute>9),1);
    lcd.print("m");
    lcd.setCursor(16-1-sekunde>9,1);
    lcd.print(sekunde);
    lcd.setCursor(16,1);
    lcd.print("s");
  }
  else
  {
    lcd.print(sekunde);
    lcd.setCursor(16,1);
    lcd.print("s");
  }
}


void meritve()
{

povprecja[0][kazalec]=hrup();
povprecja[1][kazalec]= bme.humidity;
povprecja[2][kazalec]=osvetljenost();
povprecja[3][kazalec]=bme.temperature;
povprecja[4][kazalec]=bme.pressure/100.0;
povprecja[5][kazalec]=bme.gas_resistance/1000.0;
povprecja[6][kazalec]=bme.readAltitude(SEALEVELPRESSURE_HPA);


kazalec++;
if(kazalec>=ST_VZORCEV)kazalec=0;
}

uint16_t hrup()
{
  digitalWrite(D3,0);
  digitalWrite(D4,1);
  delay(10);
  return(analogRead(A0));
}

uint16_t osvetljenost()
{
  digitalWrite(D4,0);
  digitalWrite(D3,1);
  delay(10);
  return(analogRead(A0));
}

uint8_t fronte_tipk()
{
static uint8_t tipke_cajt=0;
bool return_flag=0;
static uint8_t tipke_prej;

uint8_t tipke=(digitalRead(TIPKA_MODE)<<2)||(digitalRead(TIPKA_A)<<1)||(digitalRead(TIPKA_B)<<0);

if((tipke_prej!=tipke)&&((millis()-tipke_cajt)>TIPKE_TIME_THRESHOLD)){return_flag=1;tipke_cajt=millis();}

tipke_prej=tipke;

if(return_flag)return ~(tipke_prej&tipke);
else return 0;
}

bool wifi_publish(String velicina_text,uint16_t velicina_vrednost)
{
  
}

void tetris()
{
  
}
