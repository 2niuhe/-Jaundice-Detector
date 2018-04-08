#include <stdio.h>
#include <string.h>
#include <LiquidCrystal.h>
#include <OnewireKeypad.h>
#include <SPI.h>
#include <SD.h>
#define MAX_TIMES 5     //一个病人可查询记录的最多条数
//1线控制4×4矩阵键盘（A0）
char KEYS[]= {
  '1','2','3','A',
  '4','5','6','B',
  '7','8','9','C',
  '*','0','#','D'
};
OnewireKeypad <Print, 16 > Keypad(Serial, KEYS, 4, 4, A0, 4700, 1000 );
//6线控制1602lcd屏幕
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(4, 5, 6,7,8,9);
//4线控制SD卡模块
/* int SD_CS = 10;        //片选信号端 默认low选
int SD_SCK = 13;      //SPI时钟     默认low
int SD_MOSI = 11;     //SPI数据输入口  默认high
int SD_MISO = 12;     //SPI数据输出口  默认high */
//1线控制led灯
int led = 2;
//1线控制探测光源的通断
int light = 3;
//1线控制蜂鸣器
int buzzer = 1;       //注意在加载程序是断开pin2
//1线接收信号调理电路传来的模拟信号
int sensor = A3;


//各种flag
//boolean running = false;    //工作流程：运行状态，默认关闭，当running == true，光源打开
//boolean getdata = false;    //模拟口A3开始循环读数，读取12组有意义的模拟量，AD转换后
                //去除最高和最低值，取平均值，若读取成功,getdata == true;
                //当running && getdata == true，表示这次探测成功，将数据存入SD卡
                //显示在屏幕上，然后将running和getdata重新置false

char key_value;         //当前的键值
File myFile;          //创建文件对象
boolean lcd_on = true;
char filename[23];      //患者文件名
char temp_read_value[5];      //从文件中读取的值
char temp [MAX_TIMES][5];



/////////////////////////////////////////////////////////////////////
void write_to_file(char filename[],int temp);
int convert_to_MedicalValue(int temp);
boolean warning(int i);
boolean get_filename(int parameter);
int get_current();
int string_to_int(char str[]);
void light_on();
void buzzer_on();
void light_off();
void show_current_record(char* filename,int record);

void setup ()
{
  //初始化lcd屏幕
  lcd.begin(16, 2);
  lcd.autoscroll();
  pinMode(led, OUTPUT);
  pinMode(light, OUTPUT);
  pinMode(buzzer, OUTPUT);
//初始化SD卡
  if (!SD.begin(10)) 
  {
  lcd.print("initialization failed!");
  return;
  }
  //lcd.print("initialization done.");
  //delay(2000);
  Home();
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  
  //设定键盘中断
  Keypad.addEventKey(Record, 'A');    //开始测试并记录
  Keypad.addEventKey(Home, 'B');      //返回主页
  Keypad.addEventKey(lcd_switch,'C'); //切换lcd的状态，可以节电
  Keypad.addEventKey(search,'D');   //按照病人ID搜寻记录
}
/*
每个病人单独开一个文件，文件名是"ID+序列号.log"
文件内的记录按时间顺序排列，每条记录占用一行，格式如下：
7.89
10.31
9.45

*/
void loop() 
{
  //key_value = Keypad.Getkey();
  Keypad.ListenforEventKey(); // check if an EventKey is found
}

void Record()
{
  digitalWrite(led, LOW); 
  int temp;
  boolean flag;
  flag = get_filename(1);       //参数1表示顺带打开/创建文件
  if (!flag)
  {
    Home();
    return;
  }
  temp = get_current();
  light_off();
  temp = convert_to_MedicalValue(temp);
  write_to_file(filename,temp);
  show_current_record(filename,temp);
}

void Home()
{
  digitalWrite(led, LOW); 
  lcd.clear();
  lcd.noAutoscroll();
  lcd.print("    HD--Test");
  lcd.setCursor(0,1);
  lcd.print("  --By TangKang");
}
//切换lcd的状态，可以节电
void lcd_switch()
{
  if (lcd_on)
  {
    lcd.noDisplay();
    lcd_on = false;
  }
  else
  {
    lcd.display();
    lcd_on = true;
  }
}

void search()
{
  boolean flag;
  

  flag = get_filename(0); 
                      //参数1表示先检测是否存在病人记录
                      //若存在则以只读方式打开
                      //若不存在，则返回false     
  int i;                  //指示读取了几条记录             
  if (!flag)                
  {                   
    return;
  }
  else
  { 
    lcd.clear();
    lcd.print(filename);
    lcd.setCursor(0,1);
    for(i = 0;i < MAX_TIMES;i++)
    { 
      if(read_one_line_from_file())
      {
    int j =0;
    while(j < 4)
    {
      temp[i][j] = temp_read_value[j];        //有问题
      j++;
    }
        lcd.print(temp_read_value);
        lcd.print(' ');
      }
      else
      {
        break;
      }
    }
      
  }
  if(i > 0 && warning(i-1))
  {
    buzzer_on();
    
    led_on();
  }
}
//获取当前测量值
//测量10次，去除最高最低值，然后取平均
int get_current()
{
   const int  numReadings = 10;
   int    readings[numReadings];      // the readings from the analog input
   int    readIndex = 0;              // the index of the current reading
   int    total = 0;                  // the running total
   int    average = 0;                // the average
   int    count = 0;

  light_on();               //打开探测光源

  while (readIndex < 10 && count < 20)
  {
    count++;
    if((readings[readIndex] = analogRead(sensor)) > 10)
    {
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;
    }
    delay(1);        // delay in between reads for stability
  }


  if(readIndex == 10)
  {
    // calculate the average:
    return average = total / numReadings;
  }
  else
  {
    return 0;     //表示读取失败
  }
}
//显示当前的记录
void show_current_record(char* filename,int record)
{
  lcd.clear();
  lcd.setCursor(0,0);
  //lcd.print("hello ");
  lcd.print(filename);
  lcd.setCursor(0,1);
  //lcd.autoscroll();
  lcd.print(record);
  delay(5000);
}

//打开探测光源
void light_on()
{
  digitalWrite(light, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);   
  //digitalWrite(light, LOW); 
}
void light_off()
{
  digitalWrite(light, LOW); 
}
//蜂鸣器报警
void buzzer_on()
{
  digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);   
  digitalWrite(buzzer, LOW); 
}
//LED灯亮
void led_on()
{
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)   
}
//获取由病人ID组成的文件名
boolean get_filename(int parameter)
{ 
  filename[0] = 'I';
  filename[1] = 'D';
  filename[2] = '\0';
  char temp[16];
  int index;
  lcd.clear();
  //lcd.autoscroll();
  lcd.print("input ID:");
  lcd.setCursor(0,1);
  //lcd.noAutoscroll();
  while(true)
  {
    key_value = Keypad.Getkey();
    
      delay(200);
      if(key_value >= '0' && key_value <= '9')
      {
        lcd.print(key_value);
        temp[index] = key_value;
        index++;
      }
      else if(key_value == '#')
      {
        temp[index] = '\0';
        strcat(filename,temp);
        strcat(filename,".log");
        if(parameter)
        {
          myFile = SD.open(filename, FILE_WRITE);
        }
        else
        {
          if(!SD.exists(filename))      //判断文件是否存在
          {
            lcd.clear();
            lcd.print("No such ID!");
            return false;         //表示不存在这个病人的记录
          }
          else
          {
            myFile = SD.open(filename);   //若存在则以只读方式打开
            break;
          }
        }
        if (myFile)
        {
          lcd.clear();
          //lcd.autoscroll();
          lcd.print("Open file done!");
          myFile.close();
          break;
          //return true;
        }
        else
        { 
          lcd.clear();
          lcd.print("Wrong1");
          delay(1000);
          Home();
          break ;
        }
      }
      else if(key_value == '*')
      {
        //index = 0;
        Home();
        break;
        //return false;
      }
      else 
        continue;
        
    
    
  }
  if(index > 0)
  {
    return true;
  }
  else
  {
    return false;           //没有获取有效的病人ID
  }
}

void write_to_file(char filename[],int temp)
{
  myFile = SD.open(filename, FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) 
  {
  //lcd.setCursor(0,0);
  //lcd.autoscroll();
    //lcd.print("Writing to SD...");
  //delay(5000);
    myFile.println(temp);
    // close the file:
    myFile.close();
  //lcd.clear();
    //lcd.print("Done.");
  } 
  else 
  {
    // if the file didn't open, print an error:
  lcd.clear();
    lcd.print("error opening test.log");
  }

}

int convert_to_MedicalValue(int temp)
{
  //此处根据临床数据对仪器进行标定，设计算法
  return temp/8;
}

boolean warning (int i)
{
  int a = i;
  int HD_value[a + 1];
  while(i >= 0)
  {
    HD_value[i] = string_to_int(temp[i]);
    i--;
  }
  if( HD_value[a] > 12)
  {
    return true;
  }
  else if(a == 1 && HD_value[1] > HD_value[0]+2)
  {
    return true;
  }
  else if(a > 1 && HD_value[a] > HD_value[a -1] + 5)
  {
    return true;
    }
  else
    return false;
}

boolean read_one_line_from_file()
{ 
  byte index = 0; 
  temp_read_value[0] = '\0';
  while(myFile.available())
  {
    char temp = myFile.read();
    if( temp  <= '9' && temp >= '0')
    { 
      temp_read_value[index] = temp;
      index++;
    }
    if(temp == '\n')
    { 
      temp_read_value[index] = '\0';
      break;
    }
  }
  if(index > 0)
    return true;
  else
    return false;
}

int string_to_int(char  str[5]) //字符串转为int
{
  int i,n,sum;
  i=0;
  n = 0;
  sum=0;
  while(str[i]) //当str[i]不为\0时执行循环
  {
    n = str[i] - 48;
  sum = sum * 10 + n;
  i++;
  }
  return sum;
}

