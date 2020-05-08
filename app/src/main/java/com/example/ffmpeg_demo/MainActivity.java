package com.example.ffmpeg_demo;

import android.Manifest;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import java.io.File;

import utils.PermissionsUtils;

public class MainActivity extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
    //视频地址
    String input;
    String input5;
    String input7;
    String input4;
    String input6;
    String output;
    //音频地址
    String audio_input;
    String audio_output;
    int spinnerPosition=0;
    AudioPlayer ap;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
        System.loadLibrary("postproc-54");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
        System.loadLibrary("yuv");
    }

    private videoView vv;
    private Spinner sp_video;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        init();

        //多种格式的视频列表
        String[] videoArray = getResources().getStringArray(R.array.video_list);
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_list_item_1,
                android.R.id.text1, videoArray);
        sp_video.setAdapter(adapter);
        sp_video.setOnItemSelectedListener(this);

        input = new File(Environment.getExternalStorageDirectory(), "DCIM/Camera/input2.mp4").getAbsolutePath();
        //ffmpeg -i transpose=1 ----顺时针旋转iuput2.mp4 90度生成的视频input5.mp4
        input5 = new File(Environment.getExternalStorageDirectory(), "Tencent/QQfile_recv/input5.mp4").getAbsolutePath();
        //ffmpeg -i transpose=2 ----顺时针旋转iuput2.mp4 90度生成的视频input4.mp4
        input4= new File(Environment.getExternalStorageDirectory(), "Tencent/QQfile_recv/input4.mp4").getAbsolutePath();
        //ffmpeg -i transpose=3 ----顺时针旋转iuput2.mp4 90度生成的视频input7.mp4
        input7= new File(Environment.getExternalStorageDirectory(), "Tencent/QQfile_recv/input7.mp4").getAbsolutePath();
        //ffmpeg -i transpose=1在转一次transpose=1 ----顺时针旋转iuput2.mp4 90度生成的视频input6.mp4
        input6= new File(Environment.getExternalStorageDirectory(), "Tencent/QQfile_recv/input6.mp4").getAbsolutePath();
        output = new File(Environment.getExternalStorageDirectory(), "DCIM/Camera/output2_1280x720_yuv420p.yuv").getAbsolutePath();
        //动态授权
        PermissionsUtils pu = PermissionsUtils.getInstance();
        String[] permissions = new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE, Manifest.permission.READ_EXTERNAL_STORAGE,};
        pu.grantPermissions(MainActivity.this, permissions);
    }

    private void init(){
        vv = findViewById(R.id.video_view);
        sp_video = findViewById(R.id.sp_video);
        ap=new AudioPlayer();
    }

    /**
     * 原生native绘制
     * 1.lock window
     * 2.设置缓冲区
     * 3.unlock window
     *
     * @param view
     */
    public void mPlay(View view) {
        Log.i("spinnerPosition：", ""+spinnerPosition);
        switch (spinnerPosition){
            case 0:
                System.out.println(input);
//                input="http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
                decodeYUV420toWINDOW(input, vv.getHolder().getSurface());
                break;
            case 1:
                decodeYUV420toRGB(input, vv.getHolder().getSurface());
                break;
            case 2:
                decodeYUV420(input, output);
                break;
            case 3:
                 //音频
                audio_input=new File(Environment.getExternalStorageDirectory(), "DCIM/Camera/input2.mp4").getAbsolutePath();
                audio_output = new File(Environment.getExternalStorageDirectory(),"DCIM/Music/p1.pcm").getAbsolutePath();
                sound(audio_input,audio_output);
                break;
            case 4:
                //多线程
                pthread();
                Log.d("jason", UUIDUtils.get());
                break;
        }

    }


    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
        Log.i("Spinner选中项：",""+(position));
          spinnerPosition=position;
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    //将mp4转换为yuv420
    public native void decodeYUV420(String input, String output);
    //YUV420展示在surface
    public native void decodeYUV420toWINDOW(String input, Surface surface);
    //YUV420转为RGB 展示在surface
    public native void decodeYUV420toRGB(String input, Surface surface);
    //播放音频
    public native void sound(String input,String output);
    //线程开发
    public native void pthread();

    //音视频同步
    public native void playAll();
}
