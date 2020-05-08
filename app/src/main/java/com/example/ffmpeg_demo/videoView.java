package com.example.ffmpeg_demo;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * Created by hi on 2019.8.1.
 *将视频绘制的画布
 */

public class videoView extends SurfaceView {
    public videoView(Context context) {
        super(context);
        init();
    }

    public videoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public videoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    //初始化SurfaceView绘制的像素格式
    private void init(){
        SurfaceHolder holder=getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }



}
