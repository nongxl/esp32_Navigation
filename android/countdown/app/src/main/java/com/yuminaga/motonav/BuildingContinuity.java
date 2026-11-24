package com.yuminaga.motonav;

import java.util.Arrays;

/** 换地图时按真实坐标保留已取到的楼块；普通图携带旧楼块不等于新楼块已完成。 */
public final class BuildingContinuity {
    private static final int SIZE=1200;
    private BuildingContinuity() {}
    public static final class Result {
        public final byte[] bytes;
        public final boolean complete;
        public final int retained;
        Result(byte[] bytes,boolean complete,int retained) {
            this.bytes=bytes;this.complete=complete;this.retained=retained;
        }
    }
    private static int u16(byte[] b,int p) { return (b[p]&255)|((b[p+1]&255)<<8); }
    private static int i32(byte[] b,int p) { return u16(b,p)|(u16(b,p+2)<<16); }
    private static boolean valid(byte[] b) {
        return b!=null && b.length>=17 && RasterPacket.valid(b,u16(b,0),i32(b,2),i32(b,6));
    }
    private static byte[] decode(byte[] b) {
        byte[] pixels=new byte[SIZE*SIZE];
        for(int pos=16,k=0;pos<b.length;) {
            int v=b[pos++]&255,run=(v>>>4)+1;
            if(run==16)run+=b[pos++]&255;
            Arrays.fill(pixels,k,k+run,(byte)(v&15));k+=run;
        }
        return pixels;
    }
    public static Result merge(byte[] incoming,byte[] known) {
        boolean complete=valid(incoming) && RasterPacket.isBuilding(incoming);
        Result original=new Result(incoming,complete,0);
        if(!valid(incoming) || !valid(known) || !RasterPacket.isBuilding(known) || u16(incoming,0)!=u16(known,0))return original;
        int inMm=RasterPacket.decodeMm(u16(incoming,14));
        int knownMm=RasterPacket.decodeMm(u16(known,14));
        if(inMm<=0 || knownMm<=0)return original;
        long inX0=i32(incoming,2),inY0=i32(incoming,6);
        long knX0=i32(known,2),knY0=i32(known,6);
        byte[] current=decode(incoming),previous=decode(known);int retained=0;
        if(inMm==knownMm) {
            long stepM=inMm/1000L;
            if(stepM<=0)return original;
            long dx=inX0-knX0,dy=knY0-inY0;
            long ox=Math.floorDiv(dx+(stepM/2),stepM),oy=Math.floorDiv(dy+(stepM/2),stepM);
            if(ox<=-SIZE || ox>=SIZE || oy<=-SIZE || oy>=SIZE)return original;
            int xStart=(int)Math.max(0,-ox),xEnd=(int)Math.min(SIZE,SIZE-ox);
            int yStart=(int)Math.max(0,-oy),yEnd=(int)Math.min(SIZE,SIZE-oy);
            for(int y=yStart;y<yEnd;y++) {
                int a=y*SIZE+xStart,b=(int)((y+oy)*SIZE+xStart+ox);
                for(int x=xStart;x<xEnd;x++,a++,b++) {
                    int ground=current[a]&255,roof=previous[b]&255;
                    // 只补已确认的屋顶；新道路、水面及新取得的屋顶都优先。
                    if((ground==0 || ground==2) && roof>=4 && roof<=7) {
                        current[a]=(byte)(4|(roof&1)|(ground&2));retained++;
                    }
                }
            }
        } else {
            // 跨分辨率继承（如 2m/px 精细图 -> 6m/px 宏观图，或反向），确保缩放切换瞬间楼块不闪烁消失
            long dxMm=(inX0-knX0)*1000L,dyMm=(knY0-inY0)*1000L;
            byte[] carried=new byte[SIZE*SIZE];
            int scale=Math.max(1,(inMm+knownMm/2)/knownMm);
            int minHits=scale>=3?2:1;
            for(int y=0;y<SIZE;y++) {
                long ky0=Math.floorDiv(dyMm+(long)y*inMm,knownMm);
                if(ky0+scale<=0 || ky0>=SIZE)continue;
                int y0=(int)Math.max(0,ky0),y1=(int)Math.min(SIZE,ky0+scale);
                int rowOff=y*SIZE;
                for(int x=0;x<SIZE;x++) {
                    int ground=current[rowOff+x]&255;
                    if(ground!=0 && ground!=2)continue;
                    long kx0=Math.floorDiv(dxMm+(long)x*inMm,knownMm);
                    if(kx0+scale<=0 || kx0>=SIZE)continue;
                    int x0=(int)Math.max(0,kx0),x1=(int)Math.min(SIZE,kx0+scale);
                    int hits=0;
                    for(int ky=y0;ky<y1;ky++) {
                        int kRow=ky*SIZE;
                        for(int kx=x0;kx<x1;kx++) {
                            int r=previous[kRow+kx]&255;
                            if(r>=4 && r<=7) hits++;
                        }
                    }
                    if(hits>=minHits) { carried[rowOff+x]=1; retained++; }
                }
            }
            if(retained>0) {
                for(int y=0;y<SIZE;y++) {
                    int rowOff=y*SIZE;
                    for(int x=0;x<SIZE;x++) {
                        int i=rowOff+x;
                        if(carried[i]==0)continue;
                        boolean edge=x==0 || x+1==SIZE || y==0 || y+1==SIZE ||
                            carried[i-1]==0 || carried[i+1]==0 || carried[i-SIZE]==0 || carried[i+SIZE]==0;
                        int ground=current[i]&255;
                        current[i]=(byte)((edge?AmapBuildings.OUTLINE:AmapBuildings.ROOF)|(ground&2));
                    }
                }
            }
        }
        if(retained==0)return original;
        byte[] packet=MapGeo.INSTANCE.encodeRaster(u16(incoming,0),i32(incoming,2),i32(incoming,6),SIZE,SIZE,inMm|AmapBuildings.FLAG,current,null);
        if(!valid(packet))return original;
        return new Result(packet,complete,retained);
    }
}
