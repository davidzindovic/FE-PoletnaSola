#include <LiquidCrystal_I2C.h>

//#define TIPKA_MODE
//#define TIPKA_A
//#define TIPKA_B

#define ST_VZORCEV 30

// 1.VRSTICA HRUP, 2. VLAGA, 3. OSVETLJENOST, 4. TEMPERATURA
uint16_t povprecja[4][ST_VZORCEV]={0};

String velicine[4]={"Loud: ","Humid: ","Bright: ","Temp: "};
String enote[4]={"Db","%","lx","C"};

byte customChar[8] = {
  0b01100,
  0b10010,
  0b01100,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 


void setup() {

//pinMode(,INPUT_PULLUP);
//pinMode(,INPUT_PULLUP);
//pinMode(,INPUT_PULLUP);

pinMode(D3,OUTPUT); //en izbor za mux
pinMode(D4,OUTPUT); //drug izbor za mux (ni se mi dal negatorja dewat)
pinMode(A0,INPUT);  //mam sam en adc
Serial.begin(9600);
digitalWrite(D4,1);

lcd.init();
lcd.backlight();
}

void loop() {
  // put your main code here, to run repeatedly:
static bool choice=0;


/*if (choice)
{
  digitalWrite(D3,0);
  digitalWrite(D4,1);
  delay(50);
  Serial.print("MICROPHONE: ");
  Serial.println(analogRead(A0));
}
else
{
  digitalWrite(D4,0);
  digitalWrite(D3,1);
  delay(50);
  Serial.print("LDR: ");
  Serial.println(analogRead(A0));  
}

choice=!choice;
delay(50);
*/

//test izpisa arraya:
//for (uint8_t i=0;i<4;i++){IzpisNaEkran(i,(uint16_t)i*4,0);delay(500);lcd.clear();}

//plan poteka:
//IzpisNaEkran(izbrana_velicina,povprecje[izbrana_velicina][meritve()],povprecje_izbor);

}


//velicina je pozicija velicine v arrayu
void IzpisNaEkran(uint8_t velicina_index, uint16_t vrednost,bool avrg)
{//lcd.clear();
  uint8_t unit_rezerva; //+1 za presledek med vrednostjo in enoto
  if((velicina_index==0) || (velicina_index==2) || (velicina_index==3)) unit_rezerva=2+1;
  else unit_rezerva=1+1;
  
  lcd.setCursor(0, 0);
  lcd.print(velicine[velicina_index]);
  lcd.setCursor(16-(vrednost>=0)-(vrednost>9)-(vrednost>99)-unit_rezerva,0); //rezerviramo dovolj števk
  lcd.print(vrednost);
  lcd.setCursor(16-unit_rezerva+1,0); //-1 da bo presledek

  if(velicina_index!=3)lcd.print(enote[velicina_index]);
  else
  {
    lcd.createChar(0, customChar);
    lcd.setCursor(16-unit_rezerva+1,0);
    lcd.write((byte)0);
    lcd.print(enote[velicina_index]);
  }

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

uint8_t meritve()
{static uint8_t kazalec=0;

povprecja[0][kazalec]=hrup();
//povprecja[1][kazalec]= vlaga!!!;
povprecja[2][kazalec]=osvetljenost();
//povprecja[3][kazalec]=temp!!!;


kazalec++:
if(kazalec>=ST_VZORCEV){kazalec=0; return ST_VZORCEV-1;}
else return kazalec;
}

uint16_t hrup()
{
  digitalWrite(D3,0);
  digitalWrite(D4,1);
  delay(50);
  return(analogRead(A0));
}

uint16_t osvetljenost()
{
  digitalWrite(D4,0);
  digitalWrite(D3,1);
  delay(50);
  return(analogRead(A0));
}

void tetris()
{
  
}
