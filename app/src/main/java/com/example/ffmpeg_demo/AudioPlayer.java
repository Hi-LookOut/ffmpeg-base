package com.example.ffmpeg_demo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Created by hi on 2019.8.7.
 * 播放音频
 */

public class AudioPlayer {


    /**
     * 创建一个AudioTrac对象，用于播放
     * @param nb_channels
     * @return
     */
    public AudioTrack createAudioTrack(int sampleRateInHz, int nb_channels){
        //固定格式的音频码流
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        Log.i("音频", "nb_channels:"+nb_channels);
        //声道布局
        int channelConfig;
        if(nb_channels == 1){
            channelConfig = android.media.AudioFormat.CHANNEL_OUT_MONO;
        }else if(nb_channels == 2){
            channelConfig = android.media.AudioFormat.CHANNEL_OUT_STEREO;
        }else{
            channelConfig = android.media.AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);

        AudioTrack audioTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRateInHz, channelConfig,
                audioFormat,
                bufferSizeInBytes, AudioTrack.MODE_STREAM);
        //播放
        //audioTrack.play();
        //写入PCM
        //audioTrack.write(audioData, offsetInBytes, sizeInBytes);
        audioTrack.play();
        return audioTrack;
    }
}
