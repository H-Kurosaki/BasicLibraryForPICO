//2025/11/4 綛田氏修正

#include "MyGPS2.h"

bool GPS::GPSRead()
{

static int writepoint=0;
int r;

//GPSからの文字列を受信する
//Receive character string from GPS
while (Serial1.available())
{
  int c=Serial1.read();
  //文字列の開始点
  if(c=='$' || writepoint>SERIAL_BUFFSIZE-1)
    {
    for(int j=0;j<SERIAL_BUFFSIZE;j++){gps_txt[j]=0;}
    writepoint=0;
    }
    
  gps_txt[writepoint]=c;
  writepoint++;
  //文字列の終了点
  if(c=='\n')
    {
    DecodeGPS();
    }

  }

return true;
}
//--------------------------------------------------------------

void GPS::begin()
{
Serial1.begin(9600);
GPSSts=false;//GPSが使えるか
TimeSts=false;//時間情報が有効化どうか
PositionSts=false;//位置情報が有効かどうか
Satellites=0;

}

//太陽仰角、太陽方位角を計算する
void GPS::CalcSun(double longitude_deg,double latitude_deg)
{
//明石：東経134度59分51秒 緯度：北緯34度38分48秒
double latitude=latitude_deg*TO_RAD;//北緯(rad)
double longitude=longitude_deg*TO_RAD;//東経(rad)
double longitude_d=longitude_deg;//東経(degree)

double cDays=DayOfTheYear;//元旦からの通し日
double cHours=HourOfTheToday;//本日の通し時間
double rr=(cDays-1)/365.0*2.0*PAI;//θ(rad)
double rA=0.006918-0.399912*cos(rr)+0.070257*sin(rr)-0.006758*cos(2*rr)+0.000907*sin(2.0*rr)-0.002697*cos(3.0*rr)+0.00148*sin(3.0*rr);//太陽赤緯(rad)
double alpha=rA/PAI*180.0;//太陽赤緯(degree)
double eq=0.000075+0.001868*cos(rr)-0.032077*sin(rr)-0.014615*cos(2*rr)-0.040849*sin(2*rr);//均時差Eq(rad)
double hourAngle=(cHours-12.0)/12.0*PAI+(longitude_d-135.0)/180.0*PAI+eq;//時角h(rad)
double sunHeight=asin(sin(latitude)*sin(rA)+cos(latitude)*cos(rA)*cos(hourAngle));//高度α(rad)
double sunDir=atan(cos(latitude)*cos(rA)*sin(hourAngle)/(sin(latitude)*sin(sunHeight)-sin(rA)));//太陽方位(rad)
//P3=IF(AND(hourAngle<0, sunDir/PAI*180.0>0), sunDir/PAI*180.0-180.0, IF(AND(hourAngle>0, sunDir/PAI*180<0), sunDir/PAI*180+180,sunDir/PAI*180));//太陽方位(degree)
double sunDir_deg;
if(hourAngle<0 && sunDir>0){sunDir_deg=sunDir/PAI*180.0-180.0;}
else if(hourAngle>0 && sunDir<0){sunDir_deg=sunDir/PAI*180.0+180.0;}
else {sunDir_deg=sunDir/PAI*180.0;}
SolarAziDeg=sunDir_deg;
SolarAltDeg=sunHeight*180/PAI;
}

  
//GPSのメッセージを解読する
//Decipher GPS message
int GPS::DecodeGPS()
{

  //for debug
  /*
    IPAddress ip(255,255,255,255);
    UECS_UDP16520.beginPacket(ip, 16520);
    UECS_UDP16520.write(gps_txt);
    UECS_UDP16520.endPacket();
  */
  
int i;
int sumpos = -1;  // 初期化
unsigned int sum=0;
//チェックサム計算
//Checksum calculation
for(i=1;i<SERIAL_BUFFSIZE;i++)
{
if(gps_txt[i]=='*'){sumpos=i+1;break;}
sum^=gps_txt[i];
}

char sumtxtL[8];
sprintf(sumtxtL, "%02X\r\n", sum);//照合用チェックサム文字列
//チェックサム照合
//Checksum verification
if(strcmp((char *)(gps_txt+sumpos),sumtxtL)==0)
  {
    //OK
  }
  else
  {
    return GPS_SERIAL_NG;
    //NG
  }

if(strstr(gps_txt,"RMC,")!=NULL){GPSSts=true;RMCSts=true;DecodeRMC();return GPS_SERIAL_RMC;}
else if(!RMCSts && strstr(gps_txt,"ZDA,")!=NULL){GPSSts=true;DecodeZDA();return GPS_SERIAL_ZDA;}
else if(!RMCSts && strstr(gps_txt,"GLL,")!=NULL){DecodeGLL();return GPS_SERIAL_GLL;}
else if(strstr(gps_txt,"GSV,")!=NULL){DecodeGSV();return GPS_SERIAL_GSV;}
return GPS_SERIAL_OTHER;
}

//-------------------------------------
void GPS::DecodeRMC()
{
//RMCセンテンスは位置情報と日付時刻を簡潔に示す
//得られるのは世界標準時なので日本標準時に変換している
//例:
//$GNRMC,072134.00,A,3601.47407,N,14005.91166,E,0.000,,100723,,,A*67
//$GNRMC,051320.00,V,,,,,,,120723,,,N*63
//どの衛星のデータかは以下のTalker IDによって決定。
//GP=GPS/SBAS、GL=GLONASS、GA=Galileo、GB=BeiDou、GQ=QZSS、GN=任意の衛星の組み合わせ

double toLat;
double toLon;

TimeSts=false;
PositionSts=false;
int i=0;
int ii=0;
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
long HHMMSS=atol((char *)(gps_txt+i));

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
if(gps_txt[i]=='A')//座標データは有効
  {
    //60進法表記なので10進表記に変換する
    i=SkipComma(i);
    if(i==SERIAL_BUFFSIZE){return;}
    long LatH=atol((char *)(gps_txt+i))/100;
    long LatM=atol((char *)(gps_txt+i))-LatH*100;
    i=SkipDot(i);
    ii=i;
    if(i==SERIAL_BUFFSIZE){return;}
    double LatL=atol((char *)(gps_txt+i));
    
    i=SkipComma(i);
    if(i==SERIAL_BUFFSIZE){return;}
    //本来はここでN：北緯、S：南緯の判定が入る
    
    //出力される小数部の桁数を求めて除数を決める
    //(GPSモジュールによって桁数が違うため)
    double pw=1;
    for(int j=0;j<i-ii-1;j++)
      {pw=pw*10;}
    toLat=LatH+(LatM+LatL/pw)/60;
    
    i=SkipComma(i);
    if(i==SERIAL_BUFFSIZE){return;}
    long LonH=atol((char *)(gps_txt+i))/100;
    long LonM=atol((char *)(gps_txt+i))-LonH*100;
    
    //60進法表記なので10進表記に変換する
    i=SkipDot(i);
    if(i==SERIAL_BUFFSIZE){return;}
    double LonL=atol((char *)(gps_txt+i));
    toLon=LonH+(LonM+LonL/pw)/60;//
    
    LatitudeDeg=toLat;
    LongitudeDeg=toLon;    
    PositionSts=true;

    i=SkipComma(i);
    i=SkipComma(i);
    //東経西経判定が入る
    i=SkipComma(i);
    //移動速度
    i=SkipComma(i);
  }
else if (gps_txt[i]=='V')//座標無効
  {
    i=SkipComma(i);
    i=SkipComma(i);
    i=SkipComma(i);
    i=SkipComma(i);
    i=SkipComma(i);
    i=SkipComma(i);
    i=SkipComma(i);
  }

if(i==SERIAL_BUFFSIZE){return;}
long DDMMYY=atol((char *)(gps_txt+i));

//時刻の退避
lastHour=Hour;
lastMin=Min;
lastSec=Sec;
lastMSec=cpuMSec;
//時間の取得(国際標準時)
Hour=HHMMSS/10000;
Min=HHMMSS/100-Hour*100;
Sec=HHMMSS%100;

//日付の取得
Day=DDMMYY/10000;
Mon=DDMMYY/100-Day*100;
Year=DDMMYY%100+2000;
cpuMSec=millis();
//-----------------------------------日本標準時に変換する
GMTtoJST(JST);
TimeSts=CheckTimeSts();


}
//------------------------------------------
void GPS::DecodeZDA()
{
//ZDAセンテンスは日付時刻を含んでいる
//これはGPSユニットが独自に管理しており、衛星を補足していれば誤差は常に3秒以内に保たれる
//一度キャリブレーションすれば衛星を補足できなくても日付時刻は得られるが徐々に精度が低下する
//得られるのは世界標準時なので日本標準時に変換している

/*
The ZDA sentence includes the date and time.
This is managed independently by the GPS unit, and errors are always kept within 3 seconds if supplemented with satellites.
Even if it can not complement the satellite once it is calibrated, the date and time can be obtained but the accuracy gradually decreases.
Since it is world standard time to obtain it is converted to Japan Standard Time.
*/


TimeSts=false;
int i=0;

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
long UHMS=atol((char *)(gps_txt+i));

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
Day=atol((char *)(gps_txt+i));
if(Day==0){return;}

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
Mon=atol((char *)(gps_txt+i));
if(Mon==0){return;}


i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
Year=atol((char *)(gps_txt+i));
if(Year==0){return;}

//時刻の退避
lastHour=Hour;
lastMin=Min;
lastSec=Sec;
lastMSec=cpuMSec;
//------------------------------------
Hour=UHMS/10000;
Min=UHMS/100-Hour*100;
Sec=UHMS-Hour*10000-Min*100;
cpuMSec=millis();
//-----------------------------------日本標準時に変換する
GMTtoJST(JST);

TimeSts=CheckTimeSts();

}

//--------------------------------------------------
void GPS::DecodeGLL()
{

//GLLセンテンスはGPSから得られた緯度経度を含んでいる
//ただし、表記方法が60進法表記なのでGoogleMapなどで使われる10進角度表記に変換する
//起動直後などで捕捉している衛星数が少ないと誤差が大きい
//例：
//$GNGLL,3601.46383,N,14005.91332,E,061306.00,A,A*71
/*
The GLL sentence contains latitude and longitude obtained from GPS.
However, since the notation method is notation in hexadecimal notation, 
it is converted to decimal angle notation used in GoogleMap and others.
If the number of satellites captured just after startup is small, the error is large.
*/
//Serial.println(gps_txt);
  PositionSts=false;
int i=0;
int ii=0;
double toLat;
double toLon;

//60進法表記なので10進表記に変換する
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
long LatH=atol((char *)(gps_txt+i))/100;
long LatM=atol((char *)(gps_txt+i))-LatH*100;
i=SkipDot(i);
ii=i;
if(i==SERIAL_BUFFSIZE){return;}
double LatL=atol((char *)(gps_txt+i));

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
//本来はここでN：北緯、S：南緯の判定が入る

//出力される小数部の桁数を求めて除数を決める
//(GPSモジュールによって桁数が違うため)
double pw=1;
for(int j=0;j<i-ii-1;j++)
  {pw=pw*10;}
toLat=LatH+(LatM+LatL/pw)/60;

i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
long LonH=atol((char *)(gps_txt+i))/100;
long LonM=atol((char *)(gps_txt+i))-LonH*100;

//60進法表記なので10進表記に変換する
i=SkipDot(i);
if(i==SERIAL_BUFFSIZE){return;}
double LonL=atol((char *)(gps_txt+i));
toLon=LonH+(LonM+LonL/pw)/60;//

LatitudeDeg=toLat;
LongitudeDeg=toLon;

PositionSts=true;
}

//-------------------------------------------
void GPS::DecodeGSV()
{
//GSVセンテンスには補足可能な衛星のリストが書かれている
//3番目に書かれている衛星数は理想的な条件での最大値で当てにならない
//AE-GPSではＳＮ比が書かれている衛星だけが受信できているのでこの数を数える
//GSVセンテンスは複数に分割されるので先頭のメッセージを受信したときだけカウンタを初期化している

/*
The GSV sentence has a list of supplementary satellites.
The number of satellites written third is unreliable with the maximum value under ideal conditions.
Since AE-GPS can receive only satellites with SNR, this number is counted.
Since the GSV sentence is divided into a plurality of parts, 
the counter is initialized only when the head message is received.
*/

int i=0;
int j;

i=SkipComma(i);
i=SkipComma(i);
if(atol((char *)(gps_txt+i))==1){Satellites=0;}

for(j=0;j<5;j++)
{
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
}
if(atol((char *)(gps_txt+i))){Satellites++;}

for(j=0;j<4;j++)
{
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
}

if(atol((char *)(gps_txt+i))){Satellites++;}

for(j=0;j<4;j++)
{
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
}

if(atol((char *)(gps_txt+i))){Satellites++;}

for(j=0;j<4;j++)
{
i=SkipComma(i);
if(i==SERIAL_BUFFSIZE){return;}
}

if(atol((char *)(gps_txt+i))){Satellites++;}

}
//------------------------------------------
int GPS::SkipComma(int i)
{
for(i;i<SERIAL_BUFFSIZE;i++)
	{
	if(gps_txt[i]==','){return i+1;}
	}
return SERIAL_BUFFSIZE;
}
//------------------------------------------
int GPS::SkipDot(int i)
{
for(i;i<SERIAL_BUFFSIZE;i++)
	{
	if(gps_txt[i]=='.'){return i+1;}
	}
return SERIAL_BUFFSIZE;
}
//-------------------------------------------
void GPS::GMTtoJST(int timeDiff)
{
int i;  
int monthdayMax=monthday[Mon];
//うるう年判定
if(Year % 4 == 0 && Year % 100 != 0 || Year % 400 == 0)
  {monthdayMax=monthday_l[Mon];}

Hour=Hour+JST;
if(Hour>=24)
  {
  Hour-=24;
  Day++;
  
  if(Day>monthdayMax)
      {
      Day=1;
      Mon++;
      
      if(Mon>12)
        {
        Mon=1;
        Year++;
        }
      
      }
  
  }


  UECSDate=(Year-2000)*10000+Mon*100+Day;
  UECSTime=Hour*10000+Min*100+Sec;

//To Excel serial time
// double exTime=((double)Sec+(double)Min*60.0+(double)Hour*3600.0)/864*1000000;
//  U_ccmList[CCMID_ExTime].value=exTime;

HourOfTheToday=(double)Sec/3600.0+(double)Min/60.0+(double)Hour;

/*
//To Excel date
U_ccmList[CCMID_ExDate].value=36525;// y2k
for(i=2000;i<Year;i++)
  {
  if(i % 4 == 0 && i % 100 != 0 || i % 400 == 0)
    {
      U_ccmList[CCMID_ExDate].value+=366;
      }
   else
      {
      U_ccmList[CCMID_ExDate].value+=365;
      } 
  }  
for(i=1;i<Mon;i++)
  {
  if(Year % 4 == 0 && Year % 100 != 0 || Year % 400 == 0)
    {
      U_ccmList[CCMID_ExDate].value+=monthday_l[i];
      }
   else
      {
      U_ccmList[CCMID_ExDate].value+=monthday[i];
      } 
  } 
  U_ccmList[CCMID_ExDate].value+=Day;
  */

  DayOfTheYear=0;
  for(i=1;i<Mon;i++)
  {
  if(Year % 4 == 0 && Year % 100 != 0 || Year % 400 == 0)
    {
      DayOfTheYear+=monthday_l[i];
      }
   else
      {
      DayOfTheYear+=monthday[i];
      } 
  } 
  DayOfTheYear+=Day;

}
//----------------------------------------------------------------
bool GPS::CheckTimeSts()
{
static unsigned count=0;
  if(Year<2020){count=0;return false;}
  if(Mon<1 || Mon>12){count=0;return false;}
  if(Day<1 || Day>31){count=0;return false;}
  
long lastDaySec=lastHour*60*60+lastMin*60+lastSec;
long nowDaySec=Hour*60*60+Min*60+Sec;

if((nowDaySec-lastDaySec)==1){count++;}
else if(lastDaySec==86399 && nowDaySec==0){count++;}//日が変わるとき
else {count=0;}

if(count>5)
  {count=5;return true;}
  
return false;
}
//-------------------------------------------
/*
GPSの送信文字列の意味(一部)

ＺＤＡセンテンス （最大３６バイト）
    $GPZDA,hhmmss,dd,mm,yyyy,xhh,mm,*cc[CR][LF]
    現在時刻、日付等を出力します。
    $GPZDA     （固定)
    hhmmss     現在時刻（ＵＴＣ、時、分、秒）
    dd         日（01～31）
    mm         月（01～12）
    yyyy       年（1997～2040）
    xhh        時差の時（-13～+00～+13）
    mm         時差の分（00～59）
    *cc        最初の１文字（＊）に続いて
               ２桁のチェックサムを示します（00～FF）
               チェックサムは＄から＊の間をXOR演算

ＧＬＬセンテンス （最大４７バイト）
    $GPGLL,ddmm.mmmm,N,dddmm.mmmm,E,hhmmss,x,a*cc[CR][LF]
    位置、測位時刻等を出力します。
    $GPGLL     （固定）
    ddmm.mmmm  緯度（度、分、１／１００００分）
    N          N：北緯、S：南緯
    dddmm.mmmm 経度（度、分、１／１００００分）
    E          E：東経、W：西経
    hhmmss     測位時刻（ＵＴＣ、時、分、秒）
    x          測位状態
               A：単独測位中またはＤＧＰＳ測位中
               V：未測位
    a          測位モード
               N：未測位
               A：単独測位中
               D：ＤＧＰＳ測位中
    *cc        最初の１文字（＊）に続いて
               ２桁のチェックサムを示します（00～FF）
               チェックサムは＄から＊の間をXOR演算


ＧＳＶセンテンス （最大７０バイト）
    $GPGSV,m,n,mm,nn,dd,ddd,zz,nn,dd,ddd,zz,nn,dd,ddd,zz,nn,dd,ddd,zz,*cc[CR][LF]
    衛星情報を出力します。
    ＧＳＶセンテンスは、１つのメッセージで、最大４つの衛星情報を出力します。
    それ以上の衛星情報がある場合は、複数のメッセージに分けて出力されます。
    その場合、２番目の項目として、メッセージの総数が、３番目の項目として、メッセージ番号が出力されます。

    $GPGSV     （固定）
    m          メッセージ総数（1～3）
    n          メッセージ番号（1～3）
    mm         仰角が５度以上の衛星数（1～12）
    nn         １番目の衛星番号（01～32）
    dd         １番目の衛星仰角（05～90、単位：度）
    ddd        １番目の衛星方位角（000～359、単位：度）
    zz         １番目の衛星ＳＮ比（信号のＣ／Ｎｏ）
               （00～99、単位：ｄＢＨｚ）
               ：
    ［以降、２番目の衛星情報～４番目の衛星情報まで繰り返します］
               ：
    *cc        最初の１文字（＊）に続いて
               ２桁のチェックサムを示します（00～FF）
               チェックサムは＄から＊の間をXOR演算            
*/
