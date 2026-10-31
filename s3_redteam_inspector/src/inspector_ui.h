#pragma once
// 918 Technologies Evil Inspector Gadget UI. ESP32-S3 N16R8, ST7796S+FT6336U.
// Built on v1.0.7: same proven GPIOs, existing USB/radio diagnostics unchanged.
// Camera stays OFF. All scanner actions remain passive and do not associate.
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <pgmspace.h>
#include <cstring>
#include "display_st7796.h"
#include "touch_ft6336.h"
#include "inspector_asset.h"
#include "dragon_font.h"

#if !M3R_LCD_ENABLED || !M3R_TOUCH_ENABLED || CAMERA_PROFILE != 0
#error "Evil Inspector Gadget requires working LCD + touch and camera disabled"
#endif

namespace redteam_dragon {
constexpr uint16_t W=320, H=480;
constexpr uint16_t DARK=0x0802, PANEL=0x1005, PANEL2=0x18A8, RED=0xF124;
constexpr uint16_t WHITE=0xFFFF, MUTED=0xC659, GREEN=0x27E8, ORANGE=0xFD20;
enum class Page: uint8_t {Home, Wifi, Ble, Camera, Logs, Settings, Status};
static Page page=Page::Home;
static uint16_t *frame=nullptr;
static bool enabled=false;
static bool touchWasDown=false;
static uint32_t lastPoll=0, lastTap=0;
static uint16_t scroll=0;
static uint8_t portraitTouchRotation=0; // 0 uses verified v1.0.7 mapping. 1 flips 180 degrees.
static bool dirty=false;

// These functions are implemented by original main.cpp; no duplicate radio stacks.
void refreshWifi();
void refreshBle();
void sendExport();
int wifiResults();
int bleResults();
String wifiLabel(unsigned index);
String bleLabel(unsigned index);
String wifiDetail(unsigned index);
String bleDetail(unsigned index);
bool filesystemReady();
bool loggingIsEnabled();

inline void fill(uint16_t c) {
  if (!frame) return;
  for (size_t i=0;i<size_t(W)*H;++i) frame[i]=c;
}
inline void pixel(int x,int y,uint16_t c) {
  if(frame && x>=0 && x<W && y>=0 && y<H) frame[size_t(y)*W+x]=c;
}
inline void rect(int x,int y,int w,int h,uint16_t c) {
  if (!frame || w<=0 || h<=0) return;
  int sx=x<0?0:x, sy=y<0?0:y;
  int ex=x+w>W?W:x+w, ey=y+h>H?H:y+h;
  for(int py=sy;py<ey;++py) {
    if (ex>sx) {
      uint16_t *dst=frame+size_t(py)*W+sx;
      for(int px=sx;px<ex;++px) *dst++=c;
    }
  }
}
inline void outline(int x,int y,int w,int h,uint16_t c) {
  rect(x,y,w,1,c); rect(x,y+h-1,w,1,c);
  rect(x,y,1,h,c); rect(x+w-1,y,1,h,c);
}
inline void text(int x,int y,const char* s,uint16_t col=WHITE,int scale=1) {
  if(!frame || !s || scale<1 || scale>3) return;
  int cursor=x;
  for (const unsigned char *p=reinterpret_cast<const unsigned char*>(s); *p; ++p) {
    if(cursor>=W) break;
    unsigned char ch=*p;
    if(ch<32 || ch>126) ch='?';
    const uint8_t *glyph=dragon_font::glyphs[ch-32];
    for(int gy=0;gy<12;++gy) {
      const uint8_t bits=glyph[gy];
      if(bits==0) continue;
      for(int gx=0;gx<7;++gx) {
        if(bits&(1U<<gx)) rect(cursor+gx*scale,y+gy*scale,scale,scale,col);
      }
    }
    cursor+=8*scale;
  }
}
inline void textString(int x,int y,const String& s,uint16_t c=WHITE,int scale=1) {
  text(x,y,s.c_str(),c,scale);
}
inline void copyApprovedDashboard(){
  if(!frame) return;
  memcpy_P(frame,inspector_asset::pixels,sizeof(uint16_t)*size_t(W)*H);
}
inline void header(const char *title){
  rect(0,0,W,64,DARK);
  rect(0,62,W,2,RED);
  text(12,8,"918 TECHNOLOGIES",WHITE,2);
  text(12,38,"EVIL INSPECTOR GADGET",RED,1);
  text(12,74,title,WHITE,2);
  rect(8,101,304,1,RED);
}
inline void footer(const char *hint="BACK        NEXT / SELECT"){
  rect(0,443,W,37,DARK);
  rect(0,442,W,1,RED);
  rect(8,452,70,21,PANEL2); outline(8,452,70,21,RED);
  text(18,456,"BACK",WHITE,1);
  text(93,455,hint,MUTED,1);
}
inline void information(int y,const String& title,const String& detail,uint16_t color=MUTED){
  rect(9,y,302,43,PANEL);
  outline(9,y,302,43,PANEL2);
  textString(17,y+4,title,WHITE,1);
  textString(17,y+20,detail,color,1);
}
inline String shortText(const String& textIn,unsigned limit=34){
  if(textIn.length()<=limit) return textIn;
  return textIn.substring(0,limit-1)+"~";
}
inline void drawHome(){
  copyApprovedDashboard();
}
inline void drawWifi(){
  fill(DARK);header("WI-FI SCAN");
  text(13,111,"PASSIVE / NO ASSOCIATION",GREEN,1);
  textString(13,128,"BSSIDS "+String(wifiResults()),WHITE,1);
  rect(9,157,302,1,PANEL2);
  const int num=wifiResults();
  for(int i=0;i<9;++i){
    const int idx=int(scroll)+i;
    if(idx>=num) break;
    const int y=166+i*30;
    textString(13,y,shortText(wifiLabel(unsigned(idx)),26),WHITE,1);
    textString(13,y+13,shortText(wifiDetail(unsigned(idx)),35),MUTED,1);
    rect(12,y+26,296,1,PANEL2);
  }
  footer("TAP LIST TO SCROLL");
}
inline void drawBle(){
  fill(DARK);header("BLE SCAN");
  text(13,111,"PASSIVE ADVERTISEMENTS",GREEN,1);
  textString(13,128,"DEVICES "+String(bleResults()),WHITE,1);
  rect(9,157,302,1,PANEL2);
  const int num=bleResults();
  for(int i=0;i<9;++i){
    const int idx=int(scroll)+i;
    if(idx>=num) break;
    const int y=166+i*30;
    textString(13,y,shortText(bleLabel(unsigned(idx)),28),WHITE,1);
    textString(13,y+13,shortText(bleDetail(unsigned(idx)),35),MUTED,1);
    rect(12,y+26,296,1,PANEL2);
  }
  footer("TAP LIST TO SCROLL");
}
inline void drawCamera(){
  fill(DARK);header("CAMERA");
  information(132,"CAMERA PROFILE 0","DISABLED / UNVERIFIED PINS",ORANGE);
  information(193,"NO BACKGROUND CAPTURE","USB-ONLY; CAMERA NOT STARTED");
  text(16,290,"S3 GPIO MAP NOT CONFIRMED",MUTED,1);
  text(16,312,"DO NOT ENABLE WITHOUT PIN AUDIT",ORANGE,1);
  footer("CAMERA SAFETY PAGE");
}
inline void drawLogs(){
  fill(DARK);header("LOGS / EXPORT");
  information(130,"FILESYSTEM",filesystemReady()?"MOUNTED":"UNAVAILABLE",filesystemReady()?GREEN:ORANGE);
  information(186,"LOGGING",loggingIsEnabled()?"ENABLED":"DISABLED");
  outline(14,266,290,64,RED);text(26,282,"EXPORT OVER USB",WHITE,2);
  text(20,353,"USE USB: EXPORT / LOG STATUS",MUTED,1);
  footer("TAP EXPORT PANEL");
}
inline void drawSettings(){
  fill(DARK);header("SETTINGS");
  information(125,"DISPLAY","ST7796S 480x320 / PORTRAIT UI",GREEN);
  information(182,"TOUCH","FT6336U / I2C 0x38",GREEN);
  information(239,"CAMERA","DISABLED FOR GPIO SAFETY",ORANGE);
  outline(14,320,291,65,RED);
  text(22,337,"FLIP TOUCH 180",WHITE,2);
  textString(22,363,"MODE "+String(portraitTouchRotation),MUTED,1);
  footer("TAP FLIP IF NEEDED");
}
inline void drawStatus(){
  fill(DARK);header("SYSTEM STATUS");
  information(119,"DEVICE","ESP32-S3 / UART USB",GREEN);
  information(170,"FLASH",String(ESP.getFlashChipSize()/1024/1024)+" MiB");
  information(221,"PSRAM",String(ESP.getPsramSize()/1024/1024)+" MiB");
  information(272,"UPTIME",String(millis()/1000)+" seconds");
  information(323,"SCAN CACHE",String(wifiResults())+" WIFI / "+String(bleResults())+" BLE");
  information(374,"FLIPPER LINK","UART0 TX ONLY / 115200 / GPIO",ORANGE);
  footer("LIVE DEVICE METRICS");
}
// ST7796S is left in the proven 480x320 landscape SPI mode. Rotate the
// 320x480 portrait framebuffer into its physical portrait glass orientation.
// A 960-byte SPI line buffer avoids requiring DMA and keeps PSRAM out of SPI DMA.
inline void flush(){
  if(!enabled || !frame || !mermaid_lcd::initialized) return;
  mermaid_lcd::window(0,0,479,319);
  static uint8_t line[480*2];
  mermaid_lcd::beginTransaction();
  digitalWrite(M3R_LCD_PIN_DC,LOW);
  SPI.transfer(0x2C);
  digitalWrite(M3R_LCD_PIN_DC,HIGH);
  for(int y=0;y<320;++y){
    for(int x=0;x<480;++x){
      const uint16_t value=frame[size_t(479-x)*W+y];
      line[2*x]=uint8_t(value>>8);
      line[2*x+1]=uint8_t(value);
    }
    SPI.writeBytes(line,sizeof(line));
    if((y&15)==15) yield();
  }
  mermaid_lcd::endTransaction();
}
inline void render(){
  if(!enabled) return;
  switch(page){
    case Page::Home:drawHome();break;
    case Page::Wifi:drawWifi();break;
    case Page::Ble:drawBle();break;
    case Page::Camera:drawCamera();break;
    case Page::Logs:drawLogs();break;
    case Page::Settings:drawSettings();break;
    case Page::Status:drawStatus();break;
  }
  flush();
  dirty=false;
}
inline const char* pageName(){
  switch(page){
    case Page::Home:return "HOME";
    case Page::Wifi:return "WIFI";
    case Page::Ble:return "BLE";
    case Page::Camera:return "CAMERA";
    case Page::Logs:return "LOGS";
    case Page::Settings:return "SETTINGS";
    case Page::Status:return "STATUS";
  }
  return "INVALID";
}
inline void navigate(Page target){
  page=target;scroll=0;render();
  Serial.print("INSPECTOR PAGE ");Serial.println(pageName());
}
inline void start(){
  if(!frame){
    frame=reinterpret_cast<uint16_t*>(heap_caps_malloc(size_t(W)*H*sizeof(uint16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!frame){Serial.println("INSPECTOR ERROR NO PSRAM FRAMEBUFFER; BASE USB/RADIO STILL WORKS");return;}
  }
  mermaid_lcd::initialize();
  if(!mermaid_touch::controllerReady)mermaid_touch::init(Serial);
  if(!mermaid_touch::controllerReady){Serial.println("INSPECTOR ERROR TOUCH_UNAVAILABLE; USB COMMANDS AVAILABLE");return;}
  enabled=true;
  touchWasDown=false;
  navigate(Page::Home);
  Serial.println("INSPECTOR START 918 TECHNOLOGIES READY");
}
inline void status(Print& out){
  out.printf("INSPECTOR STATUS ui=%s page=%s framebuffer=%s touch=%s version=1.1.0 camera=OFF\n",
    enabled?"ON":"OFF",pageName(),frame?"PSRAM":"UNAVAILABLE",mermaid_touch::controllerReady?"READY":"NOT_READY");
}
inline void handle(const String&cmd,Print&out){
  if(cmd=="INSPECTOR STATUS"){status(out);return;}
  if(cmd=="INSPECTOR START" || cmd=="INSPECTOR HOME"){start();return;}
  if(cmd=="INSPECTOR REDRAW"){render();out.println("INSPECTOR REDRAW SENT");return;}
  if(cmd=="INSPECTOR OFF"){enabled=false;out.println("INSPECTOR OFF; LCD COMMANDS AND SCANNERS ACTIVE");return;}
  if(!enabled){out.println("INSPECTOR ERROR UI_OFF; USE INSPECTOR START");return;}
  if(cmd=="INSPECTOR WIFI"){refreshWifi();navigate(Page::Wifi);return;}
  if(cmd=="INSPECTOR BLE"){refreshBle();navigate(Page::Ble);return;}
  if(cmd=="INSPECTOR CAMERA"){navigate(Page::Camera);return;}
  if(cmd=="INSPECTOR LOGS"){navigate(Page::Logs);return;}
  if(cmd=="INSPECTOR SETTINGS"){navigate(Page::Settings);return;}
  if(cmd=="INSPECTOR SYSTEM"){navigate(Page::Status);return;}
  if(cmd=="INSPECTOR EXPORT"){sendExport();return;}
  if(cmd=="INSPECTOR TOUCH FLIP"){
    portraitTouchRotation=1-portraitTouchRotation;
    out.printf("INSPECTOR TOUCH FLIP mode=%u\n",portraitTouchRotation);render();return;
  }
  out.println("INSPECTOR ERROR UNKNOWN COMMAND; USE HELP");
}
inline void tap(uint16_t px,uint16_t py){
  if(page==Page::Home){
    if(py>=187&&py<270){
      if(px<172){refreshWifi();navigate(Page::Wifi);}else{refreshBle();navigate(Page::Ble);}return;
    }
    if(py>=272&&py<350){navigate(px<172?Page::Camera:Page::Logs);return;}
    if(py>=352&&py<431){navigate(px<172?Page::Settings:Page::Status);return;}
    return;
  }
  if(py>=442&&px<84){navigate(Page::Home);return;}
  if(page==Page::Wifi || page==Page::Ble){
    if(py>=160&&py<430){
      const int count=page==Page::Wifi?wifiResults():bleResults();
      const unsigned next=unsigned(scroll)+7U;
      scroll=count<=9?0:(next<static_cast<unsigned>(count)?static_cast<uint16_t>(next):0);
      render();
    }
    return;
  }
  if(page==Page::Logs && py>=265&&py<335){sendExport();return;}
  if(page==Page::Settings && py>=320&&py<388){
    portraitTouchRotation=1-portraitTouchRotation;
    Serial.printf("INSPECTOR TOUCH FLIP mode=%u\n",portraitTouchRotation);
    render();return;
  }
}
inline void service(){
  if(!enabled || !mermaid_touch::controllerReady || mermaid_touch::testActive)return;
  const uint32_t now=millis();
  if(now-lastPoll<42)return;
  lastPoll=now;
  mermaid_touch::Point point;
  if(!mermaid_touch::sample(point))return;
  if(!point.down){touchWasDown=false;return;}
  if(touchWasDown || now-lastTap<230)return;
  touchWasDown=true;lastTap=now;
  // Mapping for the approved portrait asset; the existing LCD is 480x320.
  int x=int(point.screenY);
  int y=479-int(point.screenX);
  if(portraitTouchRotation){x=319-x;y=479-y;}
  if(x<0||x>=W||y<0||y>=H)return;
  Serial.printf("INSPECTOR TAP x=%d y=%d page=%s\n",x,y,pageName());
  tap(uint16_t(x),uint16_t(y));
}
}
