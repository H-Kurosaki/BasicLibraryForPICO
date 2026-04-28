#include "MyDS18B20_pico.h"


//温度測定コマンドを発行してから測定が完了するまで750msかかるので、偶数秒に測定コマンドを発行し、奇数秒に読み取る
//したがって、温度の更新は2秒間隔になる
double DS18B20::autoRead1Sec(unsigned char ch)
{
if(dssts[ch].measurecount>10){dssts[ch].measurecount=10;}
if(dssts[ch].errorcount>10){dssts[ch].errorcount=10;}
  
  switch (dssts[ch].count)
    {
    case 0:
	   	sensor0.requestTemp();
      break;

    case 1:
		 if (sensor0.readTemp())
				{
				//読み出し成功の場合
				dssts[ch].errorcount=0;
				dssts[ch].enabled=true;
				dssts[ch].TempTemp=sensor0.getTemp();
				dssts[ch].measurecount++;
				}
			  else//エラーの場合
				{
				dssts[ch].measurecount=0;
				dssts[ch].enabled=false;
				dssts[ch].errorcount++;
				}
      break;
    }
  
  dssts[ch].count++;
  if(dssts[ch].count>1){dssts[ch].count=0;}
  
 if(!dssts[ch].enabled ||dssts[ch].measurecount<3)
    {return -999.9;}
    
  return dssts[ch].TempTemp;
  
}

