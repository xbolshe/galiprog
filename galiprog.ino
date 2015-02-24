/*
=============================================================================

GaliProg, version 1.1
Flash tool for Intel Galileo board

=============================================================================

This software allows to program Intel Galileo's SPI Flash memory
with using another Intel Galileo board. Galiprog may help in a situation 
when Galileo board is bricked after a failed firmware upgrade.
Have a nice flash programming :)

If you want to contact me, you may find my account on
https://communities.intel.com/community/makers/galileo/forums/
or send e-mail on pub@relvarsoft.com

xbolshe

=============================================================================
The MIT License (MIT)

Copyright (c) 2015 xbolshe, http://www.relvarsoft.com/arduino/galiprog

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

=============================================================================

Version 1.1, Feb 2015, xbolshe
    - added Galileo Gen 1 support to use it as a programmator
    - pack 1.0.4 support is added

Version 1.0, Feb 2015, xbolshe
    - first public release (available on https://github.com/xbolshe/galiprog )
    
=============================================================================
*/


#include <SPI.h>
#include <SD.h>

const int cspin = 7;

const byte CMD_WriteStatusRegister = 1;
const byte CMD_PageProgram = 2;
const byte CMD_ReadData = 3;
const byte CMD_WriteDisable = 4;
const byte CMD_ReadStatusReg1 = 5;
const byte CMD_WriteEnable = 6;
const byte CMD_FastRead = 0xB;
const byte CMD_SectorErase4k = 0x20;
const byte CMD_ReadStatusReg2 = 0x35;
const byte CMD_EnableQPI = 0x38;
const byte CMD_ProgramSecurityRegisters = 0x42;
const byte CMD_EraseSecurityRegisters = 0x44;
const byte CMD_ReadSecurityRegisters = 0x48;
const byte CMD_ReadUniqueID = 0x4B;
const byte CMD_VolatileSRWriteEnable = 0x50;
const byte CMD_BlockErase32k = 0x52;
const byte CMD_ReadSFDPreg = 0x5A;
const byte CMD_ChipErase60 = 0x60;
const byte CMD_EnableReset = 0x66;
const byte CMD_EraseProgramSuspend = 0x75;
const byte CMD_EraseProgramResume = 0x7A;
const byte CMD_ReleasePowerDownID = 0xAB;
const byte CMD_ManufacturerDeviceID = 0x90;
const byte CMD_Reset = 0x99;
const byte CMD_JEDEC_ID = 0x9F;
const byte CMD_PowerDown = 0xB9;
const byte CMD_ChipEraseC7 = 0xC7;
const byte CMD_BlockErase64k = 0xD8;

const byte ManuID_WinbondSPIflash = 0xEF;
const byte ManuID_WinbondW25Q64FV = 0x16;
const int DevID_W25Q64FV_SPI = 0x4017;
const int DevID_W25Q64FV_QPI = 0x6017;

int state = 0;
int pack104 = 0;
int platform;
int thisboard = 0;
char mac[20];

byte txcmd;
byte val1;
byte val2;
byte val3;
byte val4;
byte val5;
byte val6;
byte val7;
byte val8;
byte rval1;
byte rval2;
byte rval3;
byte rval4;
byte rval5;
byte rval6;
byte rval7;
byte rval8;

void setup() {
  Serial.begin(115200);
  Serial.println(" Setup");
  detectBoard();
  pinMode(cspin, OUTPUT);
  digitalWrite(cspin, HIGH);
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  if(thisboard==2){
    SPI.setClockDivider(SPI_CLOCK_DIV2);
  }
  SD.begin();
  delay(100);
  Serial.println(" Setup done");
}

int cmpbyte(byte* str1, byte* str2, int len){
  for(int i=0; i<len; i++){
    if(str1[i] != str2[i]){return(0);}
  }
  return(1);
}


long detectBoard(){
  int board=0;
  char str_board[256];
  char str_version[256];
  char str_flash[256];
  File myfile;
  long filesize;
  str_board[0]=0;
  str_version[0]=0;
  str_flash[0]=0;
  
  if(thisboard) {return(thisboard);}
  
  Serial.println(" === programmer board detection ===");
  system("cat < /sys/devices/virtual/dmi/id/board_name > /media/mmcblk0p1/galiprog_board.txt");
  if(SD.exists("galiprog_board.txt")){
    myfile = SD.open("galiprog_board.txt", FILE_READ);
    if(myfile){
      filesize=myfile.size();
      if(filesize>255) {filesize=255;}
      myfile.read(str_board,filesize);
      myfile.close();
      str_board[filesize]=0;
    }
  }
  system("cat < /sys/devices/virtual/dmi/id/board_version > /media/mmcblk0p1/galiprog_version.txt");
  if(SD.exists("galiprog_version.txt")){
    myfile = SD.open("galiprog_version.txt", FILE_READ);
    if(myfile){
      filesize=myfile.size();
      if(filesize>255) {filesize=255;}
      myfile.read(str_version,filesize);
      myfile.close();
      str_version[filesize]=0;
    }
  }
  system("cat < /sys/firmware/board_data/flash_version > /media/mmcblk0p1/galiprog_flash.txt");
  if(SD.exists("galiprog_flash.txt")){
    myfile = SD.open("galiprog_flash.txt", FILE_READ);
    if(myfile){
      filesize=myfile.size();
      if(filesize>255) {filesize=255;}
      myfile.read(str_flash,filesize);
      myfile.close();
      str_flash[filesize]=0;
    }
  }
  Serial.print(" Board name: ");
  Serial.print(str_board);
  Serial.print(" Board version: ");
  Serial.print(str_version);
  Serial.print(" Board flash: ");
  Serial.print(str_flash);
  if(cmpbyte((byte*)str_board,(byte*)"GalileoGen2",11)) {
      thisboard = 2;      
      Serial.print(" Board is identified as Galileo Gen2");
  }else if(cmpbyte((byte*)str_board,(byte*)"Galileo",7)) {
    if(cmpbyte((byte*)str_version,(byte*)"FAB-D",5)){
      thisboard = 1;      
      Serial.print(" Board is identified as Galileo Gen1");
    }
  } 
  if(!thisboard) {Serial.print(" Board detection failed.");thisboard=-1;}
  Serial.println(" ");
  return(thisboard);
}

long dospi_status(){
  long mystat=0;
  txcmd=CMD_ReadStatusReg1;
  val1=0;val2=0;val3=0;val4=0;val5=0;
  rval2=0xDE;
  dospi(3);
  mystat = rval2 & 0xFF;
  txcmd=CMD_ReadStatusReg2;
  rval2=0xAD;
  dospi(3);
  mystat += (rval2 << 8) & 0xFF;
  return(mystat);
}

long dospi_devid(){
  long res;
  txcmd=CMD_JEDEC_ID;
  val1=0;val2=0;val3=0;val4=0;val5=0;
  dospi(6);
  res=(rval1<<16)+(rval2<<8)+rval3;
  return(res);
}

void dospi_we(){
  txcmd=CMD_WriteEnable;
  dospi(1);
}

void dospi_we2(){
  byte wrbuf[1]={CMD_WriteEnable};
  byte rdbuf[1];
  digitalWrite(cspin,LOW);
  SPI.transferBuffer(wrbuf,rdbuf,1);
  digitalWrite(cspin,HIGH);  
}


byte dospi_rdbyte(long addr){
  byte res;
  txcmd=CMD_ReadData;
  val1=(addr>>16)&0xff;val2=(addr>>8)&0xff;val3=addr&0xff;val4=0;val5=0;
  dospi(5);
  return(rval4);
}

void dospi_rdbytestr(long addr, byte* str, long len){
  long i,j;
  byte wrbuf[16384];
  if(len<16385){
  if(thisboard==2){
    digitalWrite(cspin,LOW);
    SPI.transfer(CMD_ReadData);
    SPI.transfer((addr>>16)&0xFF);
    SPI.transfer((addr>>8)&0xFF);
    SPI.transfer(addr&0xFF);
    SPI.transferBuffer(wrbuf,str,len);
    digitalWrite(cspin,HIGH);
  }else{
    int i,j=0;
    int l=len;
    while(l){
      if(l>128){i=128;}else{i=l;}
      digitalWrite(cspin,LOW);
      digitalWrite(cspin,LOW);
      SPI.transfer(CMD_ReadData);
      SPI.transfer(((addr+j)>>16)&0xFF);
      SPI.transfer(((addr+j)>>8)&0xFF);
      SPI.transfer((addr+j)&0xFF);
      SPI.transferBuffer(wrbuf,&str[j],i);
      digitalWrite(cspin,HIGH);
      digitalWrite(cspin,HIGH);
      j+=i;
      l-=i;
    }
  }
  }
}

void dospi_rddump16(long addr){
  long addrx=addr;
  byte i;
  int k;
  byte rxbuf[16];

  Serial.print("[");
  if(addr<0x10){Serial.print("000");}else
  if(addr<0x100){Serial.print("00");}else
  if(addr<0x1000){Serial.print("0");}
  Serial.print(addr,HEX);
  Serial.print("]: ");
  for(k=0;k<16;k++){
    i=dospi_rdbyte(addrx);
    rxbuf[k]=i;
    addrx++;
    if(i<0x10){Serial.print("0");}
    Serial.print(i,HEX);
  }
  Serial.print(" ");
  for(k=0;k<16;k++){
    i=rxbuf[k];
    if(i<0x20){Serial.print(".");}else{Serial.print(char(i));}
  }
  Serial.println(" ");
}


void dospi_reset(){
  digitalWrite(cspin,LOW);
  SPI.transfer(CMD_EnableReset);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,LOW);
  digitalWrite(cspin,LOW);
  SPI.transfer(CMD_Reset);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,HIGH);
  digitalWrite(cspin,HIGH);
}

int dospi(byte len){
  digitalWrite(cspin,LOW);
  SPI.transfer(txcmd);
  if(len>1){
    rval1 = SPI.transfer(val1);
    if(len>2){
      rval2 = SPI.transfer(val2);
      if(len>3){
        rval3 = SPI.transfer(val3);
        if(len>4){
          rval4 = SPI.transfer(val4);
          if(len>5){
            rval5 = SPI.transfer(val5);
            if(len>6){
              rval6 = SPI.transfer(val6);
              if(len>7){
                rval7 = SPI.transfer(val7);
                if(len>8){
                  rval8 = SPI.transfer(val8);
                }
              }
            }
          }
        }
      }
    }
  }
  digitalWrite(cspin,HIGH);
}


int sd_dump(){
  File myfile;
  byte rdbuf[4096];
  if(SD.exists("galiprog_flash_dump.bin")){
    Serial.println("  ..Delete an existed file galiprog_flash_dump.bin");
    SD.remove("galiprog_flash_dump.bin");
  }
  Serial.print("  ..Create new file: galiprog_flash_dump.bin ");
  Serial.println(" ");
  system("touch /media/mmcblk0p1/galiprog_flash_dump.bin");
  myfile = SD.open("galiprog_flash_dump.bin", FILE_WRITE);
  if(myfile){
    long i,m=0;
    myfile.seek(0);
    Serial.print("  ..Dump data from flash to file: ");
    for(i=0;i<4096*2048;i+=4096){
      dospi_rdbytestr(i,rdbuf,4096);
      myfile.write(rdbuf,4096);
      if(m>127){Serial.print(".");m=0;}
      m++;
    }
    myfile.flush();
    myfile.close();
    Serial.println(" ");
    Serial.println("  ..Well done!");
  }else{
    Serial.println("  ..Some problems with opening a file :( ");
  }
  return(0);
}

int erase(){
  long mys;
  long i;
  mys=dospi_status();
  if(mys == 0){
    dospi_we();
    mys=dospi_status();
    if(mys & 2){
      Serial.print("  ..Erasing a flash memory ");
      txcmd=CMD_ChipErase60;
      dospi(1);
      mys=dospi_status();
      for(i=0;i<60;i++){
          delay(1000);
          mys=dospi_status();
          if(mys&1) {Serial.print(".");}else{break;}
      }
      if(i>55){
        Serial.println(" ");
        Serial.println("  ..ERROR: Some problems with erase (4) ");
        return(0);
      }
      Serial.println(" ");
      return(1);
    }else{
      Serial.println("  ..ERROR: Some problems with erase (2) ");
      return(0);
    }
  }
  Serial.println("  ..ERROR: Some problems with erase (1) ");
  return(0);
}

int program(){
  byte wrbuf[260];
  byte rdbuf[260];
  File myfile;
  long filesize,rdsz;
  long mys;
  long i,j,jmax,k;
  long addr;
  int maxwr=256;
  if(SD.exists("galiprog_flash_write.bin")){
  }else{
    Serial.println("  ..ERROR: Source file 'galiprog_flash_write.bin' was not found.");
    Serial.println("           No data was written.");
    return(0);
  }
  myfile = SD.open("galiprog_flash_write.bin", FILE_READ);
  if(!myfile){
    Serial.println("  ..ERROR: Cannot open a source file 'galiprog_flash_write.bin'");
    Serial.println("           No data was written.");
    return(0);
  }
  filesize=myfile.size();
  Serial.print("  ..'galiprog_flash_write.bin' has ");
  Serial.print(filesize,DEC);
  Serial.println(" bytes.");
  if(filesize<1024*10){
    Serial.println("  ..ERROR: A source file 'galiprog_flash_write.bin' has a size < 10KBytes");
    Serial.println("           No data was written.");
    myfile.close();
    return(0);
  }
  if(filesize>1024*1024*8){
    Serial.println("  ..ERROR: A source file 'galiprog_flash_write.bin' has a size > 8MBytes");
    Serial.println("           No data was written.");
    myfile.close();
    return(0);
  }
  dospi_reset();
  if(!erase()){
    Serial.println("           Flash data may be damaged.");
    myfile.close();
    return(0);
  } 
  dospi_reset();
  Serial.println("  ..Writing ");
  myfile.seek(0);
  //if(thisboard==2){maxwr=256;jmax=1024;}else{maxwr=128;jmax=2048;}
  maxwr=256;jmax=1024;
  mys=dospi_status();
  if(mys == 0){
    dospi_we();
    mys=dospi_status();
    if(mys & 2){
      j=0;
      addr=0;
      Serial.print("    .");
      wrbuf[0]=CMD_PageProgram;
      while(filesize){
        if(filesize>=maxwr) {rdsz=maxwr;}else{rdsz=filesize;}
        myfile.read(&wrbuf[4],rdsz);
        if(j>jmax){j=0;Serial.print(".");}
        wrbuf[1]=(addr>>16)&0xFF;
        wrbuf[2]=(addr>>8)&0xFF;
        wrbuf[3]=addr&0xFF;
        digitalWrite(cspin,LOW);
        SPI.transferBuffer(wrbuf,rdbuf,(4+rdsz));
        digitalWrite(cspin,HIGH);
        dospi_we2();
        filesize-=rdsz;
        addr+=rdsz;
        j++;
      }
      Serial.println(" ");
      myfile.close();
      return(1);
    }else{
      Serial.println("  ..ERROR: Some problems with write (2) ");
      Serial.println("           No data was written.");
      myfile.close();
      return(0);
    }
  }
  Serial.println("  ..ERROR: Some problems with write (1) ");
  Serial.println("           No data was written.");
  myfile.close();
  return(0);
}

int verify(){
  File myfile;
  long fsz;
  long blsz;
  byte rdbuf[4096];
  byte rdbuf2[4096];
  char c;
  long dif=0;
  long difla=0;
  long difblo=0;
  if(SD.exists("galiprog_flash_write.bin")){
    Serial.println("  ..Verifying ");
  }else{
    Serial.println("  ..ERROR: Source file 'galiprog_flash_write.bin' was not found.");
    return(0);
  }
  myfile = SD.open("galiprog_flash_write.bin", FILE_READ);
  if(myfile){
    long i,j,m=0;
    fsz=myfile.size();
    if(fsz<8192){
        Serial.print("  ..ERROR: 'galiprog_flash_write.bin' size is very small");
        myfile.close();
        return(0);
    }
    myfile.seek(0);
    Serial.print("  ..'galiprog_flash_write.bin' has ");
    Serial.print(fsz,DEC);
    Serial.println(" bytes.");
    Serial.print("  ..Read data from flash and compare with file: ");
    for(i=0;i<4096*2048;i+=4096){
      if(fsz>4096){blsz=4096;}else{blsz=fsz;}
      dospi_rdbytestr(i,rdbuf,blsz);
      myfile.read(rdbuf2,blsz);
      difblo=0;
      for(j=0;j<blsz;j++){
        if(rdbuf[j]!=rdbuf2[j]){ difblo++; }
      }
      dif+=difblo;   
      if(m>127){
        if(difla!=dif){c='!';}else{c='.';}
        Serial.print(c);m=0;
        difla=dif;
      }
      fsz-=blsz;
      m++;
      if(!fsz) {break;}
    }
    myfile.close();
    Serial.println(" ");
    Serial.print("  ..Found ");
    Serial.print(dif,DEC);
    Serial.println(" different bytes");
    Serial.println(" ");
    Serial.println("  ..Well done!");
  }else{
    Serial.println("  ..Some problems with opening a file :( ");
  }
  return(0);
}


void dumphex(byte* str, long offset, long len){
  long addr=0,tlen=len;
  byte i,k,l;
  byte strout[20];
  while(tlen){
    Serial.print("[");
    if(addr<0x10){Serial.print("00000");}else
    if(addr<0x100){Serial.print("0000");}else
    if(addr<0x1000){Serial.print("000");}else
    if(addr<0x10000){Serial.print("00");}else
    if(addr<0x100000){Serial.print("0");}
    Serial.print(addr+offset,HEX);
    Serial.print("]: ");
    for(k=0;k<16;k++){
      i=str[addr];
      strout[k]=i;
      addr++;
      tlen--;
      if(i<0x10){Serial.print("0");}
      Serial.print(i,HEX);
      if(!tlen) {k++;break;}
      if(k==7){Serial.print("-");}
    }
    Serial.print(" ");
    if(k<8){Serial.print(" ");}
    for(l=0;l<16-k;l++){Serial.print("  ");}
    for(l=0;l<k;l++){
      i=strout[l];
      if(i<0x20){Serial.print(".");}else{Serial.print(char(i));}
    }
    Serial.println(" ");
  }
}

void printDirectory(File dir, int numTabs) {
   long esz;
   int i;
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) { break; }
     for (int i=0; i<numTabs; i++) {
       Serial.print("   ");
     }
     Serial.print(entry.name());
     esz=strlen(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       if(esz<50-numTabs*3){
          for(i=0;i<(50-esz-numTabs*3);i++){Serial.print(" ");}
       }else{Serial.print(" ");}
       Serial.print(entry.size(), DEC);
       Serial.println(" bytes");
     }
   }
}


void loop() {
  switch(state){
    case 0: {
      if(Serial.available() > 0){ state=1; detectBoard();}      
      break;
    }
    case 1: {
      Serial.println(" ");
      Serial.println(" Galiprog, v1.1");
      Serial.println(" Flash tool for Intel Galileo");
      Serial.println(" Copyright (c) 2015 xbolshe, http://www.relvarsoft.com/arduino/galiprog");
      Serial.println(" ");
      Serial.println(" Use the following commands:");
      Serial.println("   i = Identify a flash memory");
      if(SD.exists("pack_1.0.4/platform-data-gen1.ini.base")){
      Serial.println("   m = Create a flash image");
      pack104=1;
      }
      Serial.println("   d = Dump a flash memory data to file");
      Serial.println("   e = Erase a flash memory");
      Serial.println("   w = Write a file to a flash memory");
      Serial.println("   v = Verify a flash memory with a file");
      Serial.println("   s = Show SD card directory");
      Serial.println("   h = Show this help");
      Serial.println(" ");
      for(int i=0;i<Serial.available();i++){Serial.read();}
      state=2;
      break;
    }
    case 2: {
      int i=Serial.available();
      byte c;
      if(i > 0){
        c=Serial.read();
        for(int j=0;j<i-1;j++){Serial.read();}
        if(c == 's') {state=99;}
        if(c == 'h') {state=1;}
        if(c == 'i') {state=90;}
        if(c == 'd') {state=10;}
        if(c == 'e') {state=20;}
        if(c == 'w') {state=30;}
        if(c == 'v') {state=40;}
        if(c == 'm' && pack104 == 1) {state=50;}
      }
      break;
    }
    case 10:{
      long mdevid = 0;
      Serial.println(" ");
      Serial.println(" >> Dump a flash memory to file <<");
      dospi_reset();
      mdevid = dospi_devid();
      Serial.print(" Manufacturer / Device ID=");
      Serial.println(mdevid,HEX);
      if(mdevid==((ManuID_WinbondSPIflash<<16)+DevID_W25Q64FV_SPI)){
        Serial.println("   Detected device: Winbond W25Q64FV SPI");
        state++;
      }else{
        Serial.println(" No Flash memory detected. Check your connections.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;
      }
      break;
    }
    case 11:{
      sd_dump();
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 20:{
      long mdevid = 0;
      Serial.println(" ");
      Serial.println(" >> Erase a flash memory <<");
      dospi_reset();
      mdevid = dospi_devid();
      Serial.print(" Manufacturer / Device ID=");
      Serial.println(mdevid,HEX);
      if(mdevid==((ManuID_WinbondSPIflash<<16)+DevID_W25Q64FV_SPI)){
        Serial.println("   Detected device: Winbond W25Q64FV SPI");
        state++;
      }else{
        Serial.println(" No Flash memory detected. Check your connections.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;
      }
      break;
    }
    case 21:{
      erase();
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 30:{
      long mdevid = 0;
      Serial.println(" ");
      Serial.println(" >> Write a file to a flash memory <<");
      dospi_reset();
      mdevid = dospi_devid();
      Serial.print(" Manufacturer / Device ID=");
      Serial.println(mdevid,HEX);
      if(mdevid==((ManuID_WinbondSPIflash<<16)+DevID_W25Q64FV_SPI)){
        Serial.println("   Detected device: Winbond W25Q64FV SPI");
        state++;
      }else{
        Serial.println(" No Flash memory detected. Check your connections.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;
      }
      break;
    }
    case 31:{
      program();
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 40:{
      long mdevid = 0;
      Serial.println(" ");
      Serial.println(" >> Verify a flash memory with a file <<");
      dospi_reset();
      mdevid = dospi_devid();
      Serial.print(" Manufacturer / Device ID=");
      Serial.println(mdevid,HEX);
      if(mdevid==((ManuID_WinbondSPIflash<<16)+DevID_W25Q64FV_SPI)){
        Serial.println("   Detected device: Winbond W25Q64FV SPI");
        state++;
      }else{
        Serial.println(" No Flash memory detected. Check your connections.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;
      }
      break;
    }
    case 41:{
      verify();
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 50:{
      Serial.println(" ");
      Serial.println(" >> Create a flash image using pack <<");
      Serial.println("    Select target board type:");
      Serial.println("     1 = Intel Galileo Gen 1");
      Serial.println("     2 = Intel Galileo Gen 2");
      for(int i=0;i<Serial.available();i++){Serial.read();}
      state++;
      break;
    }
    case 51:{
      int i=Serial.available();
      byte c;
      if(i > 0){
        c=Serial.read();
        for(int j=0;j<i-1;j++){Serial.read();}
        if(c == '1') {state=52;break;}
        if(c == '2') {state=53;break;}
        Serial.println(" Incorrect selection -> exit");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
      }
      break;
    }
    case 52:{
      platform=1;
      state=55;      
      Serial.println("  ..Galileo Gen1 is selected");
      break;
    }
    case 53:{
      platform=2;
      state=55;      
      Serial.println("  ..Galileo Gen2 is selected");
      break;
    }
    case 55:{
      Serial.println(" ");
      Serial.println("    Enter MAC address defined on a Galileo board's sticker");
      Serial.println("    (like 984f12345678, without '-' and ':')");
      for(int i=0;i<Serial.available();i++){Serial.read();}
      state++;
      break;
    }
    case 56:{
      int i=Serial.available();
      byte c;
      if(i > 0){
        if(i!=12){
          Serial.println("   ERROR: wrong MAC size -> exit");
          Serial.println(" Done.");
          Serial.println(" ");
          state=2;
          break;
        }
        Serial.print("    MAC address: ");
        for (int k=0;k<12;k++){
          c=Serial.read();
          mac[k]=c;
        }
        Serial.println(mac);
        Serial.println(" ");
        state=58;        
      }
      break;
    }
    case 58:{
      int res;
      char cmd[100];
      if(!(SD.exists("pack_1.0.4/platform-data-gen1.ini.base")) ||
        !(SD.exists("pack_1.0.4/platform-data-gen2.ini.base")) ){
        Serial.println("  ..ERROR: The pack is damaged (1). Please reinstall it on SD card.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
        break;          
      }
      if(!(SD.exists("pack_1.0.4/intel_galileo_1.0.4/Flash-missingPDAT_Release-1.0.4.bin"))){
        Serial.println("  ..ERROR: The pack is damaged (2). Please reinstall it on SD card.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
        break;          
      }
      if(!(SD.exists("pack_1.0.4/intel_tools/platform-data/platform-data-patch.py"))){
        Serial.println("  ..ERROR: The pack is damaged (3). Please reinstall it on SD card.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
        break;          
      }
      if(!(SD.exists("pack_1.0.4/intel_tools/platform-data/platform-data-patch.py"))){
        Serial.println("  ..ERROR: The pack is damaged (3). Please reinstall it on SD card.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
        break;          
      }
      if(!(SD.exists("pack_1.0.4/gen_flash_image.sh"))){
        Serial.println("  ..ERROR: The pack is damaged (4). Please reinstall it on SD card.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;        
        break;          
      }
      if(SD.exists("galiprog_flash_write.bin")){
        Serial.println("  ..Delete an existing file 'galiprog_flash_write.bin'");
        SD.remove("galiprog_flash_write.bin");
      }
      if(SD.exists("platform-data.ini")){
        Serial.println("  ..Delete an existing file 'platform-data.ini'");
        SD.remove("platform-data.ini");
      }
      Serial.println("  ..Create 'platform-data.ini'");
      if(platform==1){
        system("cp '/media/mmcblk0p1/pack_1.0.4/platform-data-gen1.ini.base' /media/mmcblk0p1/platform-data.ini");
      }
      if(platform==2){
        system("cp '/media/mmcblk0p1/pack_1.0.4/platform-data-gen2.ini.base' /media/mmcblk0p1/platform-data.ini");
      }
      sprintf(cmd,"echo %s >> /media/mmcblk0p1/platform-data.ini",mac);
      system(cmd);
      Serial.println("  ..Create 'galiprog_flash_write.bin'");
      system("/media/mmcblk0p1/pack_1.0.4/gen_flash_image.sh");
      if(!(SD.exists("galiprog_flash_write.bin"))){
        Serial.println("  ..ERROR: patching flash image was unsuccessful.");
        Serial.println(" Done.");
        Serial.println(" ");
        state=2;
        break;      
      }
      Serial.println("  ..Flash image was created.");
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;      
    }
    case 90:{
      long mdevid = 0;
      dospi_reset();
      mdevid = dospi_devid();
      Serial.println(" ");
      Serial.println(" >> Identify a flash memory <<");
      Serial.print(" Manufacturer / Device ID=");
      Serial.println(mdevid,HEX);
      if(mdevid==((ManuID_WinbondSPIflash<<16)+DevID_W25Q64FV_SPI)){
        Serial.println("   Detected device: Winbond W25Q64FV SPI");
        state=91;
        break;
      }
      Serial.println(" No Flash memory detected. Check your connections.");
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 91:{
      Serial.println(" ");
      Serial.println(" First 80 bytes of Flash memory:");
      dospi_rddump16(0);
      dospi_rddump16(16);
      dospi_rddump16(32);
      dospi_rddump16(48);
      dospi_rddump16(64);
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    case 99:{
      File root;
      Serial.println(" ");
      Serial.println(" >> Show SD card directory <<");
      Serial.println("  ___________________");
      root = SD.open("/");
      printDirectory(root, 0);
      Serial.println("  ___________________");
      Serial.println(" Done.");
      Serial.println(" ");
      state=2;
      break;
    }
    default: {
      Serial.println(" wrong state !");
      break;
    }
  }
  delay(100);
}



