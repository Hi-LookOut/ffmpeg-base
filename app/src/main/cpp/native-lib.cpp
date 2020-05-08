#include <jni.h>
#include <string>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);
#define MAX_AUDIO_FRME_SIZE 48000 * 4


extern "C"
{
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//缩放
#include "libswscale/swscale.h"
//像素处理
#include "libavutil/avutil.h"
//native
#include "android/native_window.h"
#include "android/native_window_jni.h"
//libyuv
#include "libyuv/libyuv.h"

#include "libavutil/imgutils.h"

//重采样
#include "libswresample/swresample.h"
#include <pthread.h>
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_ffmpeg_1demo_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}


JavaVM *javaVM;

//动态库加载时会执行
//兼容Android SDK 2.2之后，2.2没有这个函数
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved){
    LOGI("%s","JNI_OnLoad");
    javaVM = vm;
    return JNI_VERSION_1_4;
}

void* th_fun(void* arg){
    JNIEnv* env = NULL;
    //通过JavaVM关联当前线程，获取当前线程的JNIEnv
    //(*javaVM)->AttachCurrentThread(javaVM,&env,NULL);
    //(*javaVM)->GetEnv(javaVM,);
    jclass uuidutils_jcls = env->FindClass("com/example/ffmpeg_demo/UUIDUtils");
    //jclass uuidutils_jcls = (*env)->FindClass(env,"java/lang/String");
    if(uuidutils_jcls == NULL){
        LOGI("%s","NULL");
    }else{
        LOGI("%s","Not NULL");
    }
    jmethodID get_mid = env->GetStaticMethodID(uuidutils_jcls,"get","()Ljava/lang/String;");
    char* no = (char*)arg;
    int i;
    for (i = 0; i < 5; ++i) {
        LOGI("thread %s, i:%d",no,i);
        jobject uuid = env->CallStaticObjectMethod(uuidutils_jcls,get_mid);
        char* uuid_cstr = (char *) env->GetStringUTFChars((jstring) uuid, NULL);
        LOGI("%s",uuid_cstr);
        if(i == 4){
            pthread_exit((void*)0);
        }
        env->ReleaseStringUTFChars((jstring) uuid, uuid_cstr);
        sleep(1);
    }

    //解除关联
    javaVM->DetachCurrentThread();
}



//JavaVM 代表的是Java虚拟机，所有的工作都是从JavaVM开始
//可以通过JavaVM获取到每个线程关联的JNIEnv


//如何获取JavaVM？
//1.在JNI_OnLoad函数中获取
//2.(*env)->GetJavaVM(env,&javaVM);


//每个线程都有独立的JNIEnv

JNIEXPORT void JNICALL Java_com_example_ffmpeg_1demo_MainActivity_pthread
        (JNIEnv *env, jobject jobj){
    env->GetJavaVM(&javaVM);

    //创建多线程
    pthread_t tid;
    pthread_create(&tid, NULL,th_fun,(void*)"NO1");

}




//For video, it should typically contain one compressed frame. For audio it may contain several compressed frames.
// 对于视频来说，它包含一个压缩帧；对于音频它可能包含多个压缩帧。

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpeg_1demo_MainActivity_sound(
        JNIEnv *env, jobject jthiz, jstring input_jstr, jstring output_jstr) {
      const char* input_cstr=env->GetStringUTFChars(input_jstr,NULL);
      const char* output_cstr=env->GetStringUTFChars(output_jstr,NULL);
      LOGE("音频转码");
      av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if(avformat_open_input(&pFormatCtx,input_cstr,NULL,NULL) != 0){
        LOGI("%s","无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
        LOGI("%s","无法获取输入文件信息");
        return;
    }
    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for(; i < pFormatCtx->nb_streams;i++){
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx = i;
            break;
        }
    }

    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[audio_stream_idx]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGE("%s", "无法解码");
        return;
    }

    //获取解码器
    AVCodecContext *codecCtx = avcodec_alloc_context3(pCodec);
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    if(codec == NULL){
        LOGI("%s","无法获取解码器");
        return;
    }
    //打开解码器
    if(avcodec_open2(codecCtx,codec,NULL) < 0){
        LOGI("%s","无法打开解码器");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();

    //重采样设置参数-------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = codecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;
    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    //av_get_default_channel_layout(codecCtx->channels);
    uint64_t in_ch_layout = codecCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
                       out_ch_layout,out_sample_fmt,out_sample_rate,
                       in_ch_layout,in_sample_fmt,in_sample_rate,
                       0, NULL);
    swr_init(swrCtx);

    //输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //重采样设置参数-------------end

    //JNI begin------------------
    //JasonPlayer
    jclass player_class = env->GetObjectClass(jthiz);

    //AudioTrack对象
    //类似于java反射
    jclass  audioPlayer_Class=env->FindClass("com/example/ffmpeg_demo/AudioPlayer");
    jmethodID create_audio_track_mid = env->GetMethodID(audioPlayer_Class,"createAudioTrack","(II)Landroid/media/AudioTrack;");
    jobject audio_track = env->CallObjectMethod(jthiz,create_audio_track_mid,out_sample_rate,out_channel_nb);

    //调用AudioTrack.play方法
    jclass audio_track_class = env->GetObjectClass(audio_track);
    jmethodID audio_track_play_mid = env->GetMethodID(audio_track_class,"play","()V");
    env->CallVoidMethod(audio_track,audio_track_play_mid);

    //AudioTrack.write
    jmethodID audio_track_write_mid = env->GetMethodID(audio_track_class,"write","([BII)I");

    //JNI end------------------
    FILE *fp_pcm = fopen(output_cstr,"wb");

    //16bit 44100 PCM 数据
    uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRME_SIZE);
    int index = 0;
    //不断读取压缩数据
    while(av_read_frame(pFormatCtx,packet) >= 0){
        // 解码音频类型的Packet
        if((packet->stream_index == audio_stream_idx)&&avcodec_send_packet(codecCtx,packet)==0){
            // 解码packet----一个AVPacket通常包含一个压缩的Frame，而音频（Audio）则有可能包含多个压缩的Frame
            //获得解压后的一帧数据
            while (avcodec_receive_frame(codecCtx,frame)==0) {
                //解码
                LOGI("解码：%d",index++);
                swr_convert(swrCtx, &out_buffer, MAX_AUDIO_FRME_SIZE,(const uint8_t **)frame->data,frame->nb_samples);
                //获取sample的size
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples, out_sample_fmt, 1);
                fwrite(out_buffer,1,out_buffer_size,fp_pcm);

                //out_buffer缓冲区数据，转成byte数组
                jbyteArray audio_sample_array = env->NewByteArray(out_buffer_size);
                jbyte* sample_bytep = env->GetByteArrayElements(audio_sample_array,NULL);
                //out_buffer的数据复制到sampe_bytep
                memcpy(sample_bytep,out_buffer,out_buffer_size);
                //同步
                env->ReleaseByteArrayElements(audio_sample_array,sample_bytep,0);

                //AudioTrack.write PCM数据
                env->CallIntMethod(audio_track,audio_track_write_mid,
                                   audio_sample_array,0,out_buffer_size);
                //释放局部引用
                env->DeleteLocalRef(audio_sample_array);
                usleep(1000 * 16);
            }
        }

        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_free(out_buffer);

    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);

    env->ReleaseStringUTFChars(input_jstr,input_cstr);
    env->ReleaseStringUTFChars(output_jstr,output_cstr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ffmpeg_1demo_MainActivity_decodeYUV420toWINDOW(
        JNIEnv *env, jclass jcls,jstring input_jstr,jobject surface) {
    LOGE("%s", "render----开始");
    const char *input_cstr = env->GetStringUTFChars(input_jstr, NULL);
    //1.注册组件
    av_register_all();
//    LOGE("开始：%s", input_jstr);
//    LOGE("开始：%s", input_cstr);
    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    int ret=0;
    if ((ret=avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL)) != 0) {
        LOGE("%s，code:%d", "打开输入视频文件失败,",ret);
        return;
    }

    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }
    AVDictionaryEntry *tag = NULL;
    tag = av_dict_get(pFormatCtx->metadata, "rotate", tag, 0);

//    if (tag != NULL)
//    {
//        LOGI("视频旋转角度为：%s",tag);
//        av_dict_set(&pFormatCtx->metadata, "rotate", tag->value, 0);
//    } else{
//        LOGI("视频旋转角度为：%s","NULL");
//        av_dict_set(&pFormatCtx->metadata, "rotate", "90", 0);
//    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int video_index = -1;
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //找到视频流开始帧数
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    }
    AVCodec *pCodec = avcodec_find_decoder(pFormatCtx->streams[video_index]->codecpar->codec_id);
    if (pCodec == NULL) {
        LOGE("%s", "无法解码");
        return;
    }
    //4.获取视频解码器
    AVCodecContext *pCodeCtx = avcodec_alloc_context3(pCodec);
    if (pCodeCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return ;
    }
    //将视频信息传入解码器中
    avcodec_parameters_to_context(pCodeCtx, pFormatCtx->streams[video_index]->codecpar);


    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return;
    }

    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //像素数据（解码数据）
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();
    //获取native window窗口
    ANativeWindow* nativeWindow=ANativeWindow_fromSurface(env,surface);
    if(nativeWindow==0){
        LOGE("nativewindow取到失败");
        return;
    }
    //绘制时的缓冲区
    ANativeWindow_Buffer outBuffer;
    struct SwsContext *sws_ctx = sws_getContext(pCodeCtx->width,
                                                pCodeCtx->height,
                                                pCodeCtx->pix_fmt,
                                                pCodeCtx->width,
                                                pCodeCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BICUBIC,
                                                NULL,
                                                NULL,
                                                NULL);

    LOGI("AVCodecContext->time_base：num:%d,den:%d",pCodeCtx->time_base.num,pCodeCtx->time_base.den);
    //记录一共多少解压了帧，并打印
    int framecount = 0;
    int index=0;
    //6.一帧一帧的读取压缩的视频数据AVPacket
    while (av_read_frame(pFormatCtx,packet)>=0){

//       每个packet的 packet->stream_index都在0，1徘徊
//        LOGI("每帧%d",packet->stream_index)
        if(packet->stream_index!=video_index){
           LOGE("这帧不是视频");
            av_packet_unref(packet);
            continue;
        }
        index=packet->stream_index;
        LOGI("stream->视频帧：%d,pts:%ld,dts:%ld,duration:%ld",framecount,packet->pts,packet->dts,packet->duration);
        LOGI("stream->时间单位：%d,%d,%d,%d",pFormatCtx->streams[index]->time_base.num,pFormatCtx->streams[index]->time_base.den,pFormatCtx->streams[index]->avg_frame_rate.num,pFormatCtx->streams[index]->avg_frame_rate.den);
        double play=(packet->pts)*av_q2d(pFormatCtx->streams[index]->time_base);
        LOGI("streams->显示时间：%ld",play);

        //解码packet----一个AVPacket通常包含一个压缩的Frame，而音频（Audio）则有可能包含多个压缩的Frame
        if(avcodec_send_packet(pCodeCtx,packet)==0){
            //获得解压后的一帧数据
            while (avcodec_receive_frame(pCodeCtx,yuv_frame)==0){


                sws_scale(sws_ctx, (uint8_t const *const *) yuv_frame->data,
                          yuv_frame->linesize, 0,pCodeCtx ->height,
                          rgb_frame->data, rgb_frame->linesize);
                //设置缓存属性
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width, pCodeCtx->height,WINDOW_FORMAT_RGBA_8888);
//                LOGI("width:%d,height:%d", pCodeCtx->width, pCodeCtx->height)
                LOGI("width:%d,height:%d", yuv_frame->width, yuv_frame->height)
                //lock window;
                if ( ANativeWindow_lock(nativeWindow,&outBuffer,NULL) != 0) {
                    LOGE("ANativeWindow_lock error----native-lib.cpp(142)");
                    return;
                }
                //设置rgb_frame的属性（像素格式、宽高）和缓冲区
                av_image_fill_arrays( rgb_frame->data, rgb_frame->linesize,
                                      (const uint8_t *) outBuffer.bits, AV_PIX_FMT_RGBA, pCodeCtx->width, pCodeCtx->height, 1);
                //YUV->RGBA_8888
//                libyuv::I420ToARGB(yuv_frame->data[0],yuv_frame-aud>linesize[0],
//                                   yuv_frame->data[1],yuv_frame->linesize[1],
//                                   yuv_frame->data[2],yuv_frame->linesize[2],
//                                   rgb_frame->data[0], rgb_frame->linesize[0],
//                                   pCodeCtx->width,pCodeCtx->height);

                //unlock
                ANativeWindow_unlockAndPost(nativeWindow);
                LOGI("render解码%d帧", framecount++);
            }

        }
        av_packet_unref(packet);
        usleep(1000 * 16);
    }

    ANativeWindow_release(nativeWindow);
    av_frame_free(&yuv_frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pFormatCtx);
    env->ReleaseStringUTFChars(input_jstr, input_cstr);
    LOGE("%s", "结束");
}

extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpeg_1demo_MainActivity_decodeYUV420toRGB
        ( JNIEnv *env, jclass jcls,jstring input_jstr,jobject surface) {
    const char *input_cstr = env->GetStringUTFChars(input_jstr, NULL);
    //1.注册组件
    av_register_all();

    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if(avformat_open_input(&pFormatCtx,input_cstr,NULL,NULL) != 0){
        LOGE("%s","打开输入视频文件失败");
        return;
    }
    //3.获取视频信息
    if(avformat_find_stream_info(pFormatCtx,NULL) < 0){
        LOGE("%s","获取视频信息失败");
        return;
    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int video_stream_idx = -1;
    int i = 0;
    for(; i < pFormatCtx->nb_streams;i++){
        //根据类型判断，是否是视频流
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_idx = i;
            break;
        }
    }

    //4.获取视频解码器
    AVCodecContext *pCodeCtx = pFormatCtx->streams[video_stream_idx]->codec;
    AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);
    if(pCodec == NULL){
        LOGE("%s","无法解码");
        return;
    }

    //5.打开解码器
    if(avcodec_open2(pCodeCtx,pCodec,NULL) < 0){
        LOGE("%s","解码器无法打开");
        return;
    }

    //编码数据
    AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    //像素数据（解码数据）
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    //native绘制
    //窗体
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);
    //绘制时的缓冲区
    ANativeWindow_Buffer outBuffer;


    int len ,got_frame, framecount = 0;
    //6.一阵一阵读取压缩的视频数据AVPacket
    while(av_read_frame(pFormatCtx,packet) >= 0){
        //解码AVPacket->AVFrame
        len = avcodec_decode_video2(pCodeCtx, yuv_frame, &got_frame, packet);

        //Zero if no frame could be decompressed
        //非零，正在解码
        if(got_frame){

            LOGI("解码%d帧",framecount++);

            //lock
            //设置缓冲区的属性（宽、高、像素格式）
            ANativeWindow_setBuffersGeometry(nativeWindow, pCodeCtx->width, pCodeCtx->height,WINDOW_FORMAT_RGBA_8888);
            ANativeWindow_lock(nativeWindow,&outBuffer,NULL);
            LOGI("width:%d,height:%d", pCodeCtx->width, pCodeCtx->height)
            //设置rgb_frame的属性（像素格式、宽高）和缓冲区
            //rgb_frame缓冲区与outBuffer.bits是同一块内存
            avpicture_fill((AVPicture *)rgb_frame, (const uint8_t *) outBuffer.bits, AV_PIX_FMT_RGBA, pCodeCtx->width, pCodeCtx->height);

            //YUV->RGBA_8888
            libyuv::I420ToARGB(yuv_frame->data[0],yuv_frame->linesize[0],
                               yuv_frame->data[1],yuv_frame->linesize[1],
                               yuv_frame->data[2],yuv_frame->linesize[2],
                               rgb_frame->data[0], rgb_frame->linesize[0],
                               pCodeCtx->width,pCodeCtx->height);

            //unlock
            ANativeWindow_unlockAndPost(nativeWindow);

            usleep(1000 * 16);

        }

        av_free_packet(packet);
    }

    ANativeWindow_release(nativeWindow);
    av_frame_free(&yuv_frame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pFormatCtx);

    env->ReleaseStringUTFChars(input_jstr,input_cstr);
}


extern "C"
JNIEXPORT void JNICALL Java_com_example_ffmpeg_1demo_MainActivity_decodeYUV420
        (JNIEnv *env, jclass jcls, jstring input_jstr, jstring output_jstr) {
    LOGE("%s", "开始");
    const char *input_cstr = env->GetStringUTFChars(input_jstr, NULL);
    const char *output_cstr = env->GetStringUTFChars(output_jstr, NULL);

    //1.注册组件
    av_register_all();

    //封装格式上下文
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
        return;
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }

    //视频解码，需要找到视频对应的AVStream所在pFormatCtx->streams的索引位置
    int video_stream_idx = -1;
    int i = 0;
    for (; i < pFormatCtx->nb_streams; i++) {
        //根据类型判断，是否是视频流

        //过时用法
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            break;
        }
    }

    //4.获取视频解码器
    //过时用法：
    AVCodecContext *pCodeCtx = pFormatCtx->streams[video_stream_idx]->codec;
    LOGI("pCodeCtx-----width:%d,height:%d",pCodeCtx->width,pCodeCtx->height);
    if (pCodeCtx == NULL)
    {
        printf("Could not allocate AVCodecContext\n");
        return ;
    }
    //将视频信息传入解码器中
    avcodec_parameters_to_context(pCodeCtx, pFormatCtx->streams[video_stream_idx]->codecpar);

    AVCodec *pCodec = avcodec_find_decoder(pCodeCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("%s", "无法解码");
        return;
    }

    //5.打开解码器
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return;
    }

    //编码数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //像素数据（解码数据）
    AVFrame *frame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();

    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, pCodeCtx->width, pCodeCtx->height));


    //初始化缓冲区
    avpicture_fill((AVPicture *) yuvFrame, out_buffer, AV_PIX_FMT_YUV420P, pCodeCtx->width,
                   pCodeCtx->height);

    //输出文件
    FILE *fp_yuv = fopen(output_cstr, "wb");

    //用于像素格式转换或者缩放
    struct SwsContext *sws_ctx = sws_getContext(
            pCodeCtx->width, pCodeCtx->height, pCodeCtx->pix_fmt,
            pCodeCtx->width, pCodeCtx->height, AV_PIX_FMT_YUV420P,
            SWS_BILINEAR, NULL, NULL, NULL);

    //记录一共多少解压了帧，并打印
    int framecount = 0;
    int len, got_frame=0;
    //6.一阵一阵读取压缩的视频数据AVPacket
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //解码AVPacket->AVFrame
         avcodec_decode_video2(pCodeCtx, frame, &got_frame, packet);

        //Zero if no frame could be decompressed
        //非零，正在解码
        if (got_frame) {
            //frame->yuvFrame (YUV420P)
            //转为指定的YUV420P像素帧
            sws_scale(sws_ctx, (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height,
                      yuvFrame->data, yuvFrame->linesize);

            //向YUV文件保存解码之后的帧数据
            //AVFrame->YUV
            //一个像素包含一个Y
            int y_size = pCodeCtx->width * pCodeCtx->height;
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
            fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);
            LOGI("width:%d,height:%d",frame->height,frame->width);
            LOGI("解码%d帧", framecount++);
        }

        av_free_packet(packet);
    }


    fclose(fp_yuv);

    av_frame_free(&frame);
    avcodec_close(pCodeCtx);
    avformat_free_context(pFormatCtx);

    env->ReleaseStringUTFChars(input_jstr, input_cstr);
    env->ReleaseStringUTFChars(output_jstr, output_cstr);
    LOGE("%s", "结束");
}



//音视频同步
JNIEXPORT void JNICALL Java_com_example_ffmpeg_1demo_MainActivity_playAll
        (JNIEnv *env, jobject jobj, jstring input_jstr, jobject surface){


}