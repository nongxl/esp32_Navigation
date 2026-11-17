#!/usr/bin/env python3
"""主页表盘预览 v8 —— 笔画化字库（数字/符号用几何笔画，字母用 5x7 点阵）。

几何全部来自参考图（经 cmp4 左panel 量得，466 空间，圆心 238 / 外环 R=200）：
  顶行 y 115..153 | 上分隔线 y 165..168 | 时间块 y 187..288 | 下分隔线 y 308..311 | 底行 y 323..360
  竖线 x 88..388；时间字符列 104-136 / 160-214 / 231-244 / 260-315 / 327-382
  灰度：外环 ~95、分隔线 ~82、时间 ~172、顶/底行 ~146

字库网格：5 列 × 7 行。数字笔画用中心线 + 厚度：
  H = 水平笔画（厚度单位=行）、V = 垂直笔画（厚度单位=列）、D = 斜笔画、R = 圆角环
本文件是固件 WaitingScreen.h 主页字库的唯一真源，参数 1:1 对应。
"""
from PIL import Image, ImageDraw
import numpy as np

W = H = 466
CX = CY = 233

def r565(v):
    r=(v>>11)&0x1F; g=(v>>5)&0x3F; b=v&0x1F
    return (r*255//31, g*255//63, b*255//31)

BG   = r565(0x0000)   # 纯黑
RING = r565(0x5AEB)   # 外环 ~95
LINE = r565(0x528A)   # 分隔线 ~82
TXT  = r565(0xAD55)   # 时间 ~172
SUB  = r565(0x9492)   # 顶/底行 ~146

# ------------------------------------------------------------ 数字笔画库
# ('H', x0,y0, x1,y1, thick_row)   水平笔画，y 为中心行
# ('V', x0,y0, x1,y1, thick_col)   垂直笔画，x 为中心列
# ('D', x0,y0, x1,y1, thick_row)   斜笔画
# ('R', x0,y0, x1,y1, tx_col, ty_row, rad)  圆角矩形环
NUM = {
 '0': [('R',0,0, 5,7, 1.15, 0.85, 0)],
 '1': [('V',2.45,0.15, 2.45,6.9, 1.10), ('D',0.15,1.70, 2.45,0.15, 0.95)],
 '2': [('H',0.65,0.6, 4.35,0.6, 0.90), ('D',4.35,1.05, 3.45,2.95, 0.90),
       ('H',1.60,3.10, 3.60,3.10, 0.85), ('D',1.75,3.15, 0.75,6.10, 0.90),
       ('H',0.65,6.40, 4.45,6.40, 0.90)],
 '3': [('H',0.70,0.60, 4.30,0.60, 0.90), ('D',4.30,1.05, 3.40,2.95, 0.90),
       ('H',2.00,3.10, 3.60,3.10, 0.85), ('D',3.50,3.15, 4.35,6.05, 0.90),
       ('H',0.70,6.40, 4.30,6.40, 0.90)],
 '4': [('D',3.35,0.05, 0.75,4.55, 0.95), ('H',0.75,4.70, 4.50,4.70, 0.90),
       ('V',3.35,0.05, 3.35,6.90, 1.05)],
 '5': [('H',0.80,0.60, 4.30,0.60, 0.90), ('V',0.80,0.60, 0.80,3.15, 1.05),
       ('H',0.80,3.25, 3.70,3.25, 0.85), ('D',3.70,3.30, 4.35,6.05, 0.90),
       ('H',0.80,6.40, 4.30,6.40, 0.90)],
 '6': [('D',0.85,2.20, 2.50,0.55, 0.90), ('H',2.50,0.55, 4.30,0.55, 0.90),
       ('V',0.85,2.20, 0.85,6.40, 1.05), ('H',0.85,3.40, 3.60,3.40, 0.85),
       ('D',3.60,3.45, 4.35,6.05, 0.90), ('H',0.85,6.40, 4.30,6.40, 0.90)],
 '7': [('H',0.45,0.60, 4.60,0.60, 0.90), ('D',4.05,0.95, 1.85,6.90, 0.95)],
 '8': [('R',0,0, 5,3.50, 1.15, 0.80, 0), ('R',0,3.50, 5,7, 1.15, 0.80, 0)],
 '9': [('R',0,0, 5,3.35, 1.15, 0.80, 0), ('V',4.25,0.05, 4.25,6.15, 1.05)],
 ':': [('B',0,1.90, 1.30,2.85), ('B',0,4.15, 1.30,5.10)],
 '%': [('P',0,0, 1.50,2.20), ('P',3.50,4.80, 5.0,7.0), ('D',0.30,7.0, 4.70,0.0, 0.55)],
 '-': [('H',0.60,3.60, 4.40,3.60, 0.80)],
}

# ------------------------------------------------------------ 数字连续折线字库
# 每个数字由 1~2 条连续折线构成，转角补圆接头，从根本上消除离散笔画的断口。
# 与固件 HomeFace.h 的 HOME_GLYPH 一一对应。
STROKE_R = 0.55   # 笔画半宽（列单位）
GLYPH = {
 '0': [([2.50,4.15,4.15,2.50,0.85,0.85], [0.40,1.10,5.90,6.60,5.90,1.10], True)],
 '1': [([1.55,2.50,2.50], [1.65,0.40,6.60], False)],
 '2': [([0.85,1.85,4.15,4.15,0.85,0.85,4.15], [1.15,0.40,0.40,3.25,3.25,6.60,6.60], False)],
 '3': [([0.85,4.15,4.15,2.40,4.15,4.15,0.85], [0.40,0.40,3.25,3.25,3.25,6.60,6.60], False)],
 '4': [([4.35,0.80,3.35,3.35], [4.65,4.65,0.40,6.60], False)],
 '5': [([4.15,0.85,0.85,3.45,4.15,0.85], [0.40,0.40,3.25,3.25,6.60,6.60], False)],
 '6': [([4.15,2.45,0.85,0.85,4.15,4.15,0.85], [0.55,0.55,2.10,6.60,6.60,3.40,3.40], False)],
 '7': [([0.60,4.40,1.85], [0.40,0.40,6.60], False)],
 '8': [([0.85,1.55,3.45,4.15,4.15,3.45,1.55,0.85], [1.10,0.40,0.40,1.10,2.80,3.50,3.50,2.80], True),
       ([0.85,1.55,3.45,4.15,4.15,3.45,1.55,0.85], [4.20,3.50,3.50,4.20,5.90,6.60,6.60,5.90], True)],
 '9': [([0.85,1.55,3.45,4.15,4.15,3.45,1.55,0.85], [1.10,0.40,0.40,1.10,2.80,3.50,3.50,2.80], True),
       ([4.15,4.15], [3.50,6.60], False)],
}

# 七段数码管段定义（与固件 spoSegChar / HomeFace.h homeSeg7Digit 一致）
SEG7 = {
 '0':'ABCDEF', '1':'BC', '2':'ABGED', '3':'ABGCD', '4':'FGBC',
 '5':'AFGCD', '6':'AFGECD', '7':'ABC', '8':'ABCDEFG', '9':'ABCDFG',
}

def draw_seg7(d, ch, x, y, w, h, t, col):
    segs = SEG7.get(ch, '')
    half, r, ht = h//2, t//2, h//2 - t//2
    if 'A' in segs: d.rounded_rectangle([x+t/2, y, x+w-t/2, y+t], radius=r, fill=col)
    if 'G' in segs: d.rounded_rectangle([x+t/2, y+half-t/2, x+w-t/2, y+half+t/2], radius=r, fill=col)
    if 'D' in segs: d.rounded_rectangle([x+t/2, y+h-t, x+w-t/2, y+h], radius=r, fill=col)
    if 'F' in segs: d.rounded_rectangle([x, y+t/2, x+t, y+t/2+ht], radius=r, fill=col)
    if 'B' in segs: d.rounded_rectangle([x+w-t, y+t/2, x+w, y+t/2+ht], radius=r, fill=col)
    if 'E' in segs: d.rounded_rectangle([x, y+half, x+t, y+half+ht], radius=r, fill=col)
    if 'C' in segs: d.rounded_rectangle([x+w-t, y+half, x+w, y+half+ht], radius=r, fill=col)

# ------------------------------------------------------------ 字母点阵 5x7
LET = {
 'A':[0x7C,0x12,0x11,0x12,0x7C],'B':[0x7F,0x49,0x49,0x49,0x36],'C':[0x3E,0x41,0x41,0x41,0x22],
 'D':[0x7F,0x41,0x41,0x22,0x1C],'E':[0x7F,0x49,0x49,0x49,0x41],'F':[0x7F,0x09,0x09,0x09,0x01],
 'G':[0x3E,0x41,0x49,0x49,0x7A],'H':[0x7F,0x08,0x08,0x08,0x7F],'I':[0x00,0x41,0x7F,0x41,0x00],
 'J':[0x20,0x40,0x41,0x3F,0x01],'K':[0x7F,0x08,0x14,0x22,0x41],'L':[0x7F,0x40,0x40,0x40,0x40],
 'M':[0x7F,0x02,0x0C,0x02,0x7F],'N':[0x7F,0x04,0x08,0x10,0x7F],'O':[0x3E,0x41,0x41,0x41,0x3E],
 'P':[0x7F,0x09,0x09,0x09,0x06],'Q':[0x3E,0x41,0x51,0x21,0x5E],'R':[0x7F,0x09,0x19,0x29,0x46],
 'S':[0x46,0x49,0x49,0x49,0x31],'T':[0x01,0x01,0x7F,0x01,0x01],'U':[0x3F,0x40,0x40,0x40,0x3F],
 'V':[0x1F,0x20,0x40,0x20,0x1F],'W':[0x7F,0x20,0x18,0x20,0x7F],'X':[0x63,0x14,0x08,0x14,0x63],
 'Y':[0x07,0x08,0x70,0x08,0x07],'Z':[0x61,0x51,0x49,0x45,0x43],
}

def is_num(c):
    return c in NUM

# ------------------------------------------------------------ 渲染原语
def cap(d, x0,y0,x1,y1,col):
    """小圆角矩形段（半径上限 3px，保证相邻段拼接连续）"""
    w=x1-x0; h=y1-y0
    if w<=0 or h<=0: return
    r=min(min(w,h)//4,3)
    if r<=0: d.rectangle([x0,y0,x1,y1],fill=col); return
    d.rounded_rectangle([x0,y0,x1,y1],radius=r,fill=col)

def draw_num(d, ch, x, y, cw, chh, col, bg):
    """绘制数字/符号笔画，x,y 为网格左上角"""
    if ch in SEG7:
        draw_seg7(d, ch, x, y, round(4.6*cw), round(7*chh), round(1.06*cw), col)
        return
    if ch==':':
        bw,bh=round(1.10*cw),round(0.90*chh)
        for yy in (y+1.90*chh, y+4.15*chh):
            cap(d, x, yy, x+bw, yy+bh, col)
        return
    for s in NUM[ch]:
        k = s[0]
        if k=='H':
            _,x0,y0,x1,y1,t = s
            yy=y+y0*chh
            cap(d, x+x0*cw, yy-t*chh/2, x+x1*cw, yy+t*chh/2, col)
        elif k=='V':
            _,x0,y0,x1,y1,t = s
            xx=x+x0*cw
            cap(d, xx-t*cw/2, y+y0*chh, xx+t*cw/2, y+y1*chh, col)
        elif k=='D':
            _,x0,y0,x1,y1,t = s
            ax,ay=x+x0*cw, y+y0*chh
            bx,by=x+x1*cw, y+y1*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx,ny=-dy/L,dx/L
            hx,hy=t*cw/2,t*chh/2
            d.polygon([(ax+nx*hx,ay+ny*hy),(bx+nx*hx,by+ny*hy),
                       (bx-nx*hx,by-ny*hy),(ax-nx*hx,ay-ny*hy)],fill=col)
        elif k=='B':
            _,x0,y0,x1,y1 = s
            cap(d, x+x0*cw, y+y0*chh, x+x1*cw, y+y1*chh, col)
        elif k=='P':
            _,x0,y0,x1,y1 = s
            cap(d, x+x0*cw, y+y0*chh, x+x1*cw, y+y1*chh, col)
        elif k=='S':
            _,x0,y0,x1,y1,t = s
            ax,ay=x+x0*cw, y+y0*chh
            bx,by=x+x1*cw, y+y1*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx,ny=-dy/L,dx/L
            hx,hy=t*cw/2,t*chh/2
            d.polygon([(ax+nx*hx,ay+ny*hy),(bx+nx*hx,by+ny*hy),
                       (bx-nx*hx,by-ny*hy),(ax-nx*hx,ay-ny*hy)],fill=col)
        elif k=='R':
            _,gx0,gy0,gx1,gy1,tx,ty,rad = s
            w=(gx1-gx0)*cw; h=(gy1-gy0)*chh
            r = rad*cw if rad>0 else min(w,h)*0.22
            d.rounded_rectangle([x+gx0*cw, y+gy0*chh, x+gx0*cw+w, y+gy0*chh+h],
                                radius=r, fill=col)
            d.rounded_rectangle([x+gx0*cw+tx*cw, y+gy0*chh+ty*chh,
                                 x+gx0*cw+w-tx*cw, y+gy0*chh+h-ty*chh],
                                radius=max(1,r-tx*cw), fill=bg)

def num_w(ch, cw, chh):
    """数字墨迹宽度（像素）：按真实几何求各笔画的水平包络"""
    if ch in SEG7: return 4.6*cw
    if ch==':': return 1.30*cw
    xs=[]
    for s in NUM[ch]:
        k=s[0]
        if k=='H':  xs += [s[1]*cw, s[3]*cw]
        elif k=='V': xs += [s[1]*cw-s[5]*cw/2, s[1]*cw+s[5]*cw/2]
        elif k=='D':
            ax,ay=s[1]*cw, s[2]*chh
            bx,by=s[3]*cw, s[4]*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx,ny=-dy/L,dx/L
            hx,hy=s[5]*cw/2, s[5]*chh/2
            xs += [min(ax+nx*hx,ax-nx*hx,bx+nx*hx,bx-nx*hx),
                   max(ax+nx*hx,ax-nx*hx,bx+nx*hx,bx-nx*hx)]
        elif k=='B' or k=='P': xs += [s[1]*cw, s[3]*cw]
        elif k=='S':
            ax,ay=s[1]*cw, s[2]*chh
            bx,by=s[3]*cw, s[4]*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx,ny=-dy/L,dx/L
            hx,hy=s[5]*cw/2, s[5]*chh/2
            xs += [min(ax+nx*hx,ax-nx*hx,bx+nx*hx,bx-nx*hx),
                   max(ax+nx*hx,ax-nx*hx,bx+nx*hx,bx-nx*hx)]
        elif k=='R': xs += [s[1]*cw, s[3]*cw]
    return (max(xs)-min(xs)) if xs else 0

def num_left(ch, cw, chh):
    """数字墨迹左缘相对网格原点的偏移（像素）"""
    if ch in SEG7 or ch==':': return 0
    mn=1e9
    for s in NUM[ch]:
        k=s[0]
        if k=='H':  mn=min(mn, s[1]*cw)
        elif k=='V': mn=min(mn, s[1]*cw-s[5]*cw/2)
        elif k=='D':
            ax,ay=s[1]*cw, s[2]*chh
            bx,by=s[3]*cw, s[4]*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx=-dy/L
            hx=s[5]*cw/2
            mn=min(mn, ax+nx*hx, ax-nx*hx, bx+nx*hx, bx-nx*hx)
        elif k=='B' or k=='P': mn=min(mn, s[1]*cw)
        elif k=='S':
            ax,ay=s[1]*cw, s[2]*chh
            bx,by=s[3]*cw, s[4]*chh
            dx,dy=bx-ax,by-ay
            L=max(1e-6,(dx*dx+dy*dy)**0.5)
            nx=-dy/L
            hx=s[5]*cw/2
            mn=min(mn, ax+nx*hx, ax-nx*hx, bx+nx*hx, bx-nx*hx)
        elif k=='R': mn=min(mn, s[1]*cw)
    return mn if mn<1e8 else 0

def let_w(c, cw, fw):
    g=LET[c]
    cs=[i for i in range(5) if g[i]]
    return (max(cs)-min(cs))*cw+cw+fw if cs else 0

def let_left(c, cw, fw):
    """字母墨迹左缘相对网格原点的偏移（像素）"""
    g=LET[c]
    cs=[i for i in range(5) if g[i]]
    if not cs: return 0
    return min(cs)*cw - fw//2

def draw_let(d, c, x, y, cw, chh, fw, col):
    """字母：5x7 点阵行合并 + 列合并 → 实心连续笔画"""
    g=LET[c]
    for r in range(7):
        cc=0
        while cc<5:
            if not ((g[cc]>>r)&1): cc+=1; continue
            c0=cc
            while cc<5 and ((g[cc]>>r)&1): cc+=1
            cap(d, x+c0*cw, y+r*chh, x+cc*cw-1, y+r*chh+chh-1, col)
    for cc in range(5):
        r=0
        while r<7:
            if not ((g[cc]>>r)&1): r+=1; continue
            r0=r
            while r<7 and ((g[cc]>>r)&1): r+=1
            cap(d, x+cc*cw-fw//2, y+r0*chh, x+cc*cw+cw-1+fw//2, y+(r-1)*chh+chh-1, col)

def text_w(s, cw, chh, gap, fw=4):
    tot=0; first=True
    for c in s:
        if c==' ':
            continue
        w = num_w(c,cw,chh) if is_num(c) else let_w(c,cw,fw)
        if w<=0: continue
        if not first: tot+=gap
        tot+=w; first=False
    return tot

def draw_text(d, x, y, s, cw, chh, gap, col, bg, fw=4):
    """按墨迹左缘精确对齐逐字排版"""
    cur=x
    for c in s:
        if c==' ': continue
        if is_num(c):
            left = num_left(c,cw,chh)
            draw_num(d, c, cur-left, y, cw, chh, col, bg)
            cur += num_w(c,cw,chh)+gap
        else:
            left = let_left(c,cw,fw)
            draw_let(d, c, cur-left, y, cw, chh, fw, col)
            cur += let_w(c,cw,fw)+gap

def star(d,cx,cy,R,rin,col):
    pts=[]
    for i in range(10):
        a=np.deg2rad(-90+i*36); rr=R if i%2==0 else rin
        pts.append((cx+rr*np.cos(a), cy+rr*np.sin(a)))
    d.polygon(pts,fill=col)

def render(hhmm="20:07", date="11-17", wd="TUE", out="/tmp/home_preview.png", pct=85, charging=False):
    im=Image.new("RGB",(W,H),BG); d=ImageDraw.Draw(im)

    for r in range(196,201):
        d.ellipse([CX-r,CY-r,CX+r,CY+r], outline=RING)

    # ---- 顶行（参考 y 115..153）----
    TY=129
    bx,by,bw,bh = 104,110,60,38
    d.rounded_rectangle([bx,by,bx+bw,by+bh],radius=8,outline=SUB,width=3)
    d.rounded_rectangle([bx+bw,by+bh//2-7,bx+bw+8,by+bh//2+7],radius=2,fill=SUB)
    pad=4; iw=bw-2*pad; ih=bh-2*pad
    fw=max(1,min(iw,round(iw*max(0,min(100,pct))/100)))
    if pct>0:
        d.rounded_rectangle([bx+pad,by+pad,bx+pad+fw,by+pad+ih],radius=min(4,fw//2),fill=SUB)
    if charging:
        fx,fy=bx+bw//2,TY
        d.polygon([(fx+5,fy-14),(fx-7,fy+2),(fx+1,fy+2),(fx-4,fy+14),(fx+8,fy-4),(fx+1,fy-4)],fill=TXT)

    pct_txt=f"{pct}%"; pcw,pch,pgap = 4.6,4.9,5
    pw=text_w(pct_txt,pcw,pch,pgap,fw=2)
    draw_text(d, 256-pw//2, TY-pch*7//2, pct_txt, pcw,pch,pgap, SUB, BG, fw=2)
    star(d, 335, TY, 18, 7.5, SUB)

    d.rectangle([83,160,383,163], fill=LINE)

    # ---- 大号时间（严格居中于圆心）----
    cw,ch,gap = 11.0,14.4,12
    tw=text_w(hhmm,cw,ch,gap,fw=5)
    draw_text(d, CX-tw//2, 183, hhmm, cw,ch,gap, TXT, BG, fw=5)

    d.rectangle([83,303,383,306], fill=LINE)

    # ---- 底行 ----
    dw=text_w(date,4.4,5.4,8,fw=2); ww=text_w(wd,4.0,5.0,7,fw=2); gapx=32
    x0=CX-(dw+gapx+ww)//2
    draw_text(d,x0,318,date,4.4,5.4,8,SUB,BG,fw=2)
    draw_text(d,x0+dw+gapx,318,wd,4.0,5.0,7,SUB,BG,fw=2)

    im.save(out)
    print(f"saved {out} time_w={tw} time_x0={CX-tw//2} pct_w={pw} bottom_x0={x0}")
    return im

if __name__=="__main__":
    render()
