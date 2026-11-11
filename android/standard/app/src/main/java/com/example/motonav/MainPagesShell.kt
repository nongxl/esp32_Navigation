package com.example.motonav

import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import kotlin.math.cos
import kotlin.math.sin

/** Three independent, lazily created pages; bottom navigation never scrolls away. */
class MainPagesShell(private val activity:AppCompatActivity, saved:Bundle?,
                     private val build:(Int)->View, private val onSelected:(Int)->Unit):LinearLayout(activity) {
    private val S=NavStyle
    private fun dp(v:Int)=S.dpi(activity,v)
    private val titles=listOf("导航","圆屏设置","导航设置")
    private val host=FrameLayout(activity)
    private val pages=arrayOfNulls<ScrollView>(3)
    private val restoreY=saved?.getIntArray("main_page_scroll")?.takeIf{it.size==3} ?: IntArray(3)
    private val tabs=ArrayList<LinearLayout>()
    private val icons=ArrayList<PageIcon>()
    private val labels=ArrayList<TextView>()
    private val indicators=ArrayList<FrameLayout>()
    var selectedPage=0;private set

    init {
        orientation=VERTICAL;setBackgroundColor(S.BG)
        WindowCompat.setDecorFitsSystemWindows(activity.window,false)
        addView(host,LayoutParams(-1,0,1f))
        val navigation=LinearLayout(activity).apply {
            orientation=HORIZONTAL;setPadding(dp(5),dp(5),dp(5),dp(5));setBackgroundColor(S.CARD_SOLID)
            elevation=S.dp(activity,10f)
        }
        titles.forEachIndexed { index,title ->
            val tab=LinearLayout(activity).apply {
                orientation=VERTICAL;gravity=Gravity.CENTER;isFocusable=true
                contentDescription=title;minimumHeight=dp(58)
                setOnClickListener{showPage(index)}
            }
            val indicator=FrameLayout(activity)
            val icon=PageIcon(index)
            indicator.addView(icon,FrameLayout.LayoutParams(dp(23),dp(23),Gravity.CENTER))
            tab.addView(indicator,LayoutParams(dp(56),dp(29)))
            val label=TextView(activity).apply {
                text=title;textSize=12f;setTextColor(S.SUB);gravity=Gravity.CENTER
                isSingleLine=true;includeFontPadding=false;setPadding(0,dp(4),0,0)
            }
            tab.addView(label,LayoutParams(-1,-2))
            navigation.addView(tab,LayoutParams(0,dp(58),1f))
            tabs+=tab;icons+=icon;labels+=label;indicators+=indicator
        }
        addView(navigation,LayoutParams(-1,-2))
        ViewCompat.setOnApplyWindowInsetsListener(this){view,insets->
            val bars=insets.getInsets(WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout())
            val ime=insets.getInsets(WindowInsetsCompat.Type.ime())
            view.setPadding(bars.left,bars.top,bars.right,maxOf(bars.bottom,ime.bottom));insets
        }
    }

    fun showPage(index:Int) {
        if(index !in 0..2)return
        pages[selectedPage]?.visibility=GONE
        selectedPage=index
        val page=pages[index] ?: ScrollView(activity).apply {
            id=View.generateViewId();isFillViewport=true;clipToPadding=true
            addView(build(index),FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.WRAP_CONTENT))
            host.addView(this,FrameLayout.LayoutParams(-1,-1));pages[index]=this
            post{scrollTo(0,restoreY[index])}
        }
        page.visibility=VISIBLE
        tabs.forEachIndexed{i,tab->
            val selected=i==index;tab.isSelected=selected
            labels[i].setTextColor(if(selected)S.PINK else S.SUB)
            labels[i].typeface=if(selected)S.BOLD else S.MEDIUM
            indicators[i].background=if(selected)S.round(activity,S.TABSEL,14f) else null
            icons[i].active=selected
        }
        onSelected(index)
    }

    fun save(out:Bundle) {
        out.putInt("main_page_selected",selectedPage)
        out.putIntArray("main_page_scroll",IntArray(3){pages[it]?.scrollY ?: restoreY[it]})
    }

    private inner class PageIcon(private val index:Int):View(activity) {
        var active=false
            set(value){if(field!=value){field=value;invalidate()}}
        private val paint=Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style=Paint.Style.STROKE;strokeWidth=1.8f;strokeCap=Paint.Cap.ROUND;strokeJoin=Paint.Join.ROUND
        }
        override fun onDraw(canvas:Canvas) {
            super.onDraw(canvas)
            canvas.save();canvas.scale(width/24f,height/24f);paint.color=if(active)S.PINK else S.SUB
            fun line(x1:Float,y1:Float,x2:Float,y2:Float)=canvas.drawLine(x1,y1,x2,y2,paint)
            when(index) {
                0->{ // 导航：指南针/航向罗盘
                    canvas.drawCircle(12f,12f,9f,paint)
                    canvas.drawPath(Path().apply{
                        moveTo(12f,5f);lineTo(15f,12f);lineTo(12f,19f);lineTo(9f,12f);close()
                    },paint)
                    line(12f,3f,12f,5f)
                }
                1->{ // 圆屏设置：圆屏硬件 + 齿轮
                    canvas.drawCircle(12f,12f,6.5f,paint);canvas.drawCircle(12f,12f,2.5f,paint)
                    repeat(8){val a=it*Math.PI/4;line((12+6.5*cos(a)).toFloat(),(12+6.5*sin(a)).toFloat(),(12+9*cos(a)).toFloat(),(12+9*sin(a)).toFloat())}
                }
                2->{ // 导航设置：控制滑块/参数设置
                    line(4f,7f,20f,7f);line(4f,12f,20f,12f);line(4f,17f,20f,17f)
                    canvas.drawCircle(8f,7f,2.2f,paint)
                    canvas.drawCircle(16f,12f,2.2f,paint)
                    canvas.drawCircle(11f,17f,2.2f,paint)
                }
            }
            canvas.restore()
        }
    }
}
