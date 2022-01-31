#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1331.h"

// Define OLED PIN I/O
#define MOSI   51
#define CLK    52

#define DC1    36
#define CS1    37
#define RST1   35

#define DC2    32
#define CS2    33
#define RST2   31

#define DC3    28
#define CS3    29
#define RST3   27

// Color definitions
#define  BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
/*
Adafruit_SSD1331 dsp_1 = Adafruit_SSD1331(CS1, DC1, MOSI, CLK, RST1);
Adafruit_SSD1331 dsp_2 = Adafruit_SSD1331(CS2, DC2, MOSI, CLK, RST2);
Adafruit_SSD1331 dsp_3 = Adafruit_SSD1331(CS3, DC3, MOSI, CLK, RST3);
*/
Adafruit_SSD1331 dsp_1(&SPI, CS1, DC1, RST1);
Adafruit_SSD1331 dsp_2(&SPI, CS2, DC2, RST2);
Adafruit_SSD1331 dsp_3(&SPI, CS3, DC3, RST3);

// Analog Key Data
#define KeySet1 A1
#define KetSet2 A2

int KeyA = 0xFF;
int KeyB = 0xFF;

unsigned long pre_key_A_readtime = millis();
unsigned long pre_key_B_readtime = millis();
bool is_key_A_change = false;
bool is_key_B_change = false;
#define TEMP_UP     0x01
#define TEMP_DN     0x02

#define MOTOR_CW    0x04
#define MOTOR_CCW   0x08
#define MOTOR_SEL   0x10
#define MOTOR_WORK  0x0C

#define PSI_UP      0x20
#define PSI_DN      0x40
#define PSI_SEL     0x80
#define PSI_WORK    0x60


void Key_Read_A(void);
void Key_Scan_A(void);
void Key_Proc_A(void);

void Key_Read_B(void);
void Key_Scan_B(void);
void Key_Proc_B(void);
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("M-Cure Demo Board Start");
  Serial.println("Use Command cli (N)");

  Serial.println("OLED Test Start");
  delay(50);

  dsp_1.begin();
  delay(50);

  dsp_1.fillScreen(RED);
  dsp_1.setCursor(0,0);
  dsp_1.setTextColor(WHITE);
  dsp_1.setTextSize(1);
  dsp_1.print("Disp_1 Work");

  delay(50);
  
  dsp_2.begin();
  delay(50);
  dsp_2.fillScreen(GREEN);
  dsp_2.setCursor(0,0);
  dsp_2.setTextColor(WHITE);
  dsp_2.setTextSize(1);
  dsp_2.print("Disp_2 Work");
  delay(50);
  
  dsp_3.begin();
  delay(50);
  dsp_3.fillScreen(BLUE);
  dsp_3.setTextColor(WHITE);
  dsp_3.setTextSize(1);
  dsp_3.print("Disp_3 Work");
  delay(50);
  
  Serial.println("OLED Test end");
  delay(50);

}

void loop() 
{
  // put your main code here, to run repeatedly:
  Key_Scan_A();
  Key_Proc_A();

  Key_Scan_B();
  Key_Proc_B();
}


void Key_Read_A(void)
{
  int data1 = analogRead(A1);
  
  if( data1 > 937 ){ KeyA = 0xFF; }
  else if(data1 > 819){ KeyA = TEMP_UP; }
  else if(data1 > 694){ KeyA = TEMP_DN; }
  else if(data1 > 558){ KeyA = MOTOR_CW; }
  else if(data1 > 388){ KeyA = MOTOR_SEL; }
  else if(data1 > 146) { KeyA = MOTOR_CCW; }
  else { KeyA = 0xFF; }
}

void Key_Scan_A(void)   //10ms
{
  static unsigned char AutoKeyCountA = 0;
  static unsigned char f_PressedKeyA = 0;
  static unsigned char PrevKeyA = 0xFF;

  if( millis() - pre_key_A_readtime < 100) return;
  pre_key_A_readtime = millis();

  Key_Read_A();             // update Key value
    
  if(KeyA!=0xFF && PrevKeyA==KeyA) 
  {
    if(f_PressedKeyA == 0) 
    {   // First Detect - Pressed Key
      is_key_A_change = 1;
      f_PressedKeyA = 1;
        AutoKeyCountA = 15;    // about 0.5s
      }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCountA==0) 
      {
        if( KeyA == TEMP_UP || KeyA == TEMP_DN || KeyA == MOTOR_CW || KeyA == MOTOR_CCW  ) 
        {
          is_key_A_change = 1;
          AutoKeyCountA = 3; // about 0.15s
        }
      }
    }
  }
  else  
  {
      f_PressedKeyA = 0;
  }
  PrevKeyA = KeyA;
}

void Key_Proc_A(void)
{ 
  if(is_key_A_change == false) return;
  is_key_A_change = false;
  switch(KeyA)
  {
    case TEMP_UP  : Serial.println("TEMP_UP");   
      dsp_1.fillScreen(RED);
      dsp_1.setCursor(0,0);
      dsp_1.setTextColor(WHITE);
      dsp_1.setTextSize(1);
      dsp_1.print("temp up");
    break;
    case TEMP_DN  : Serial.println("TEMP_Dn");
      dsp_1.fillScreen(RED);
      dsp_1.setCursor(0,0);
      dsp_1.setTextColor(WHITE);
      dsp_1.setTextSize(1);
      dsp_1.print("temp dn");
    break;
    
    case MOTOR_CW : Serial.println("MOTOR_CW");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor cw");
    break;
    
    case MOTOR_CCW: Serial.println("MOTOR_CCW");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor ccw");
    break;
    
    case MOTOR_SEL: Serial.println("MOTOR_SEL");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor sel");
    break;

    default : break;
  }
}


void Key_Read_B(void)
{
  int data2 = analogRead(A2);

  if( data2 > 937 ){ KeyB = 0xFF; }
  else if(data2 > 819){ KeyB = PSI_UP; }
  else if(data2 > 694){ KeyB = PSI_DN; }
  else if(data2 > 558){ KeyB = PSI_SEL; }
  else if(data2 > 388){ KeyB = PSI_WORK; }
  else if(data2 > 146) { KeyB = MOTOR_WORK; }
  else { KeyB = 0xFF; }
}

void Key_Scan_B(void)   //10ms
{
  static unsigned char AutoKeyCountB = 0;
  static unsigned char f_PressedKeyB = 0;
  static unsigned char PrevKeyB = 0xFF;
  
  if( millis() - pre_key_B_readtime < 100) return;
  pre_key_B_readtime = millis();

  Key_Read_B();             // update Key value
    
  if(KeyB!=0xFF && PrevKeyB==KeyB) 
  {
    if(f_PressedKeyB == 0) 
    {   // First Detect - Pressed Key
      is_key_B_change = 1;
      f_PressedKeyB = 1;
        AutoKeyCountB = 15;    // about 0.5s
      }
    else 
    {         // Hold - Pressed Key
      if(--AutoKeyCountB==0) 
      {
        if( KeyB == PSI_UP || KeyB == PSI_DN  ) 
        {
          is_key_B_change = 1;
          AutoKeyCountB = 3; // about 0.15s
        }
      }
    }
  }
  else  
  {
      f_PressedKeyB = 0;
  }
  PrevKeyB = KeyB;
}

void Key_Proc_B(void)
{ 
  if(is_key_B_change == false) return;
  is_key_B_change = false;

  switch(KeyB)
  {
    case MOTOR_WORK  : Serial.println("MOTOR_WORK");
      dsp_2.fillScreen(GREEN);
      dsp_2.setCursor(0,0);
      dsp_2.setTextColor(WHITE);
      dsp_2.setTextSize(1);
      dsp_2.print("motor work");
    break;
    
    case PSI_UP  : Serial.println("PSI_UP");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi up");
    break;
    
    case PSI_DN : Serial.println("PSI_DN");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi dn");
    break;
    
    case PSI_SEL: Serial.println("PSI_SEL");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi sel");
    break;
    
    case PSI_WORK: Serial.println("PSI_WORK");
      dsp_3.fillScreen(BLUE);
      dsp_3.setCursor(0,0);
      dsp_3.setTextColor(WHITE);
      dsp_3.setTextSize(1);
      dsp_3.print("psi work");
    break;
    
    default : break;
  }

}
