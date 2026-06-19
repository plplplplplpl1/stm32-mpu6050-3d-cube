#include "Menu.h"
#include "OLED.h"
#include "Key.h"
#include "Delay.h"
#include "FontCN.h"
#include <string.h>

extern const uint8_t OLED_F8x16[][16];

typedef struct { const char *ascii; const uint8_t **cn; uint8_t cnLen, avail; } MenuItem;

static const uint8_t *g_m1[]={HZK_52A8,HZK_753B};
static const uint8_t *g_m4[]={HZK_96F7,HZK_8FBE};
static const MenuItem g_main[4]={
	{"3D&2D",NULL,0,1},{NULL,g_m1,2,1},{"OLED",NULL,0,1},{NULL,g_m4,2,1},
};
#define MAIN_N 4
static const MenuItem g_sub[3]={{" 3D",NULL,0,1},{" 2D",NULL,0,1},{"BACK",NULL,0,1},};
#define SUB_N 3
#define ABS(a) ((a)>0?(a):-(a))

/* ═══════════ 像素级文字 ═══════════ */
static uint8_t IW(const MenuItem *it){
	return (it->cnLen==0&&it->ascii)?(uint8_t)strlen(it->ascii)*8:it->cnLen*16;
}
/* 画8x16 ASCII字, Y16可为负 */
static void PC(uint8_t ch, uint8_t cx, int16_t Y16){
	int16_t visBot=Y16+15; if(Y16>63||visBot<0) return;
	uint8_t Y=(uint8_t)(Y16<0?0:Y16), d=Y&7, pg=Y>>3;
	int16_t skip=(Y16<0)?-Y16:0;
	const uint8_t *f=OLED_F8x16[ch]; uint8_t i;
	for(i=0;i<8;i++,cx++){
		uint16_t f16=((uint16_t)f[i])|((uint16_t)f[i+8]<<8);
		f16>>=skip; /* 丢弃屏外行, 可见行紧凑排列 */
		uint8_t b0=(uint8_t)(f16&0xFF), b1=(uint8_t)((f16>>8)&0xFF);
		if(pg<=7)   OLED_GRAM[pg][cx]   |= (b0<<d);
		if(pg+1<=7) OLED_GRAM[pg+1][cx] |= (b0>>(8-d))|(b1<<d);
		if(pg+2<=7) OLED_GRAM[pg+2][cx] |= (b1>>(8-d));
	}
}
static void D1(const MenuItem *it, int16_t Y16, uint8_t ind){
	int16_t visBot=Y16+15; if(Y16>63||visBot<0) return;
	uint8_t w=IW(it), isA=(it->cnLen==0&&it->ascii), c,i,bit,row,rb,mk,sc,cx;
	uint8_t Y=(uint8_t)(Y16<0?0:Y16), d=Y&7, pg=Y>>3;
	int16_t skip=(Y16<0)?-Y16:0;
	if(isA){ uint8_t n=(uint8_t)strlen(it->ascii); sc=(128-w)/2+ind*8;
		for(c=0;c<n;c++) PC(it->ascii[c]-' ',sc+c*8,Y16);
	}else{ uint8_t n=it->cnLen; sc=(128-w)/2+ind*16;
		for(c=0;c<n;c++){ const uint8_t *ft=it->cn[c]; cx=sc+c*16;
			for(i=0;i<16;i++,cx++){ uint8_t ub=0,lb=0;
				for(bit=0;bit<8;bit++){ row=bit; rb=ft[row*2+(i/8)]; mk=0x80>>(i%8); if(rb&mk)ub|=(1<<bit);
					row=bit+8; rb=ft[row*2+(i/8)]; if(rb&mk)lb|=(1<<bit); }
				uint16_t f16=((uint16_t)ub)|((uint16_t)lb<<8);
				f16>>=skip;
				ub=(uint8_t)(f16&0xFF); lb=(uint8_t)((f16>>8)&0xFF);
				if(pg<=7)   OLED_GRAM[pg][cx]   |= (ub<<d);
				if(pg+1<=7) OLED_GRAM[pg+1][cx] |= (ub>>(8-d))|(lb<<d);
				if(pg+2<=7) OLED_GRAM[pg+2][cx] |= (lb>>(8-d));
			}
		}
	}
}

/* ═══════════ 高亮反色条 ═══════════ */
static void Inv(int16_t Y, uint8_t H, uint8_t cL, uint8_t cR){
	if(Y<0||Y>63||H==0) return;
	uint8_t y0=(uint8_t)Y,d=y0&7,pg0=y0>>3,col;
	for(col=cL;col<=cR;col++){ uint8_t pg=pg0, bits=0xFF<<d, remain=H;
		while(remain>0&&pg<=7){ uint8_t need=8-(d&7);
			if(need>remain){bits=((1<<remain)-1)<<(d&7);need=remain;}
			else if(remain<8) bits&=(1<<remain)-1;
			OLED_GRAM[pg][col]^=bits; remain-=need; pg++; bits=0xFF; d=0; }
	}
}

/* ═══════════ 滚动条 ═══════════ */
static void SB(uint8_t sel,uint8_t cnt){
	uint8_t pg,trk=64,th=trk*3/cnt;if(th>trk)th=trk;if(th<8)th=8;
	uint8_t ty=(uint16_t)sel*(trk-th)/(cnt-1);
	for(pg=0;pg<8;pg++){OLED_GRAM[pg][125]|=0x55;OLED_GRAM[pg][126]|=0x55;}
	uint8_t pS=ty/8,bS=ty%8,pE=(ty+th)/8,bE=(ty+th)%8;
	for(pg=pS;pg<=pE&&pg<=7;pg++){uint8_t m=0xFF;
		if(pg==pS)m&=(0xFF<<bS);if(pg==pE)m&=(0xFF>>(8-bE));
		OLED_GRAM[pg][125]|=m;OLED_GRAM[pg][126]|=m;}
}

/* ═══════════ 动画 ═══════════ */
static int16_t g_off=0; /* 三项整体偏移(像素×16定点), UNLINEAR→0 */

static void Rd(const MenuItem *it,uint8_t cnt,uint8_t sel,uint8_t bar){
	int8_t pv=(sel==0)?cnt-1:sel-1, nx=(sel+1)%cnt;
	int16_t off=g_off/16;
	OLED_ClearBuffer();
	D1(&it[pv],(int16_t)0+off,2);
	D1(&it[sel],(int16_t)24+off,0);
	D1(&it[nx],(int16_t)48+off,2);
	Inv(24,20,2,120);
	if(bar) SB(sel,cnt);
}

static uint8_t Lp(const MenuItem *it,uint8_t cnt,uint8_t bar){
	uint8_t sel=0,key; g_off=0; Rd(it,cnt,sel,bar); OLED_Refresh();
	while(1){ key=Key_GetNum();
		if(key==3){ sel=(sel==0)?cnt-1:sel-1; g_off=-384; } /* CCW: items下移(+24px) */
		else if(key==4){ sel=(sel+1)%cnt; g_off=384; }      /* CW: items上移(-24px) */
		else if(key==1){ if(it[sel].avail)return sel;
			const uint8_t *na[]={HZK_5F85,HZK_5F00,HZK_53D1};
			const uint8_t *pk[]={HZK_6309,HZK_952E,HZK_8FD4,HZK_56DE};
			OLED_Clear(); OLED_ShowChineseStr(2,3,na,3); OLED_ShowChineseStr(3,2,pk,4); OLED_Refresh();
			while(Key_GetNum()==0); Delay_ms(50); g_off=0;
		}else if(key==2)return 0xFF;
		if(g_off){ g_off+=(0-g_off)>>3; if(ABS(g_off)<2)g_off=0; }
		Rd(it,cnt,sel,bar); OLED_Refresh(); Delay_ms(10);
	}
}

void Menu_Init(void){}
uint8_t Menu_Show(void){return Lp(g_main,MAIN_N,1);}
uint8_t Menu_Show3D2D(void){uint8_t r=Lp(g_sub,SUB_N,0);return(r==0xFF)?2:r;}
