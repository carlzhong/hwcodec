if [ "$1" = "vfp" ];then
ConfigMode=arm
MODE=armvfp
FLAGS='-mfpu=vfp'
elif [ "$1" = "neon" ];then
ConfigMode=arm
MODE=neon
FLAGS='-mfpu=neon'
elif [ "$1" = "x86" ];then
ConfigMode=x86
elif [ "$1" = "--help" ];then
echo "Please input option \"vfp\" , \"neon\" or \"x86\""
else
echo "Unknown option \"$1\""
echo "See ./config.sh --help for available options."
fi

export ARM_ROOT=/E/code/qiyi/forwindows/android-ndk-r8b
export ARM_PRE=arm-linux-androideabi
TOOLCHAIN=/E/code/qiyi/forwindows/android-ndk-r8b/toolchains/arm-linux-androideabi-4.6/prebuilt/windows
export PATH=$TOOLCHAIN/bin:$PATH
export ANDROID_SOURCE=/E/code/qiyi/android/android4.0
export ANDROID_LIBS=/E/code/qiyi/android/android4.0/lib
export SYSROOT=/E/code/qiyi/forwindows/android-ndk-r8b/platforms/android-8/arch-arm
FLAGS="$FLAGS  --sysroot=$SYSROOT -I$ANDROID_SOURCE/frameworks/base/include -I$ANDROID_SOURCE/system/core/include"
FLAGS="$FLAGS -I$ANDROID_SOURCE/frameworks/base/media/libstagefright -I$ANDROID_SOURCE/hardware/libhardware/include"
FLAGS="$FLAGS -I$ANDROID_SOURCE/frameworks/base/include/media/stagefright/openmax"
FLAGS="$FLAGS -I$ARM_ROOT/sources/cxx-stl/system/include -I$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/include"
FLAGS="$FLAGS -I$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/include -I$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/include/tr2"
FLAGS="$FLAGS -I$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a/include"
EXTRA_CXXFLAGS="-Wno-multichar -fno-exceptions -fno-rtti -lstdc++ -lstlport -lcutils -lstagefright -lbinder -lc -lm -lz -ldl -llog"

if [ "$ConfigMode" = "arm" ];then
./configure \
--enable-cross-compile \
--target-os=linux \
--arch=arm \
--cross-prefix=/E/code/qiyi/forwindows/android-ndk-r8b/toolchains/arm-linux-androideabi-4.6/prebuilt/windows/bin/${ARM_PRE}- \
--extra-cflags="${FLAGS} -mfloat-abi=softfp -fPIC" \
--disable-logging \
--enable-gpl \
--enable-version3 \
--enable-nonfree \
--disable-doc \
--disable-ffplay \
--disable-ffprobe \
--disable-ffserver \
--disable-avdevice \
--disable-postproc \
--enable-small \
--enable-fastdiv \
--disable-encoders \
--disable-decoders \
--enable-decoder=png \
--enable-decoder=h264 \
--enable-decoder=h263 \
--enable-decoder=h263i \
--enable-decoder=mpeg4 \
--enable-decoder=mjpeg \
--enable-decoder=jpeg2000 \
--enable-decoder=pcm_s16le \
--enable-decoder=zlib \
--enable-decoder=aac \
--enable-decoder=aac_latm \
--enable-decoder=aasc \
--enable-decoder=ac3 \
--enable-decoder=mp3 \
--enable-decoder=amrnb \
--enable-decoder=amrwb \
--enable-encoder=aac \
--enable-encoder=ac3 \
--enable-encoder=h263 \
--enable-encoder=pcm_s16le \
--enable-encoder=mjpeg \
--enable-encoder=jpeg2000 \
--enable-encoder=png \
--enable-encoder=zlib \
--disable-hwaccels \
--disable-muxers \
--disable-demuxers \
--enable-demuxer=mov \
--enable-demuxer=applehttp \
--enable-demuxer=h263 \
--enable-demuxer=h264 \
--enable-demuxer=mpegts \
--enable-demuxer=concat \
--enable-demuxer=aac \
--enable-demuxer=ac3 \
--enable-demuxer=amr \
--enable-demuxer=mp3 \
--enable-demuxer=image2 \
--enable-demuxer=image2pipe \
--enable-demuxer=mjpeg \
--enable-demuxer=pcm_s16le \
--enable-muxer=ac3 \
--enable-muxer=adts \
--enable-muxer=h264 \
--enable-muxer=image2 \
--enable-muxer=mjpeg \
--enable-muxer=mp4 \
--enable-muxer=mpegts \
--enable-muxer=mov \
--enable-muxer=tgp \
--enable-muxer=pcm_s16le \
--enable-muxer=rawvideo \
--disable-parsers \
--enable-parser=h264 \
--enable-parser=aac \
--enable-parser=aac_latm \
--enable-parser=ac3 \
--enable-parser=h263 \
--enable-parser=mjpeg \
--enable-parser=mpeg4video \
--enable-parser=mpegvideo \
--enable-parser=mpegaudio \
--disable-protocols \
--enable-protocol=applehttp \
--enable-protocol=http \
--enable-protocol=tcp \
--enable-protocol=udp \
--enable-protocol=concat \
--enable-protocol=file \
--enable-protocol=pipe \
--disable-filters \
--enable-filter=abuffer \
--enable-filter=abuffersink \
--enable-filter=aconvert \
--enable-filter=aformat \
--enable-filter=amerge \
--enable-filter=amovie \
--enable-filter=aresample \
--enable-filter=color \
--enable-filter=copy \
--enable-filter=crop \
--enable-filter=fade \
--enable-filter=fifo \
--enable-filter=format \
--enable-filter=lutrgb \
--enable-filter=movie \
--enable-filter=negate \
--enable-filter=overlay \
--enable-filter=pad \
--enable-filter=scale \
--enable-filter=select \
--enable-filter=thumbnail \
--enable-filter=transpose \
--enable-filter=unsharp \
--enable-filter=volume \
--disable-decoder=vp8 \
--disable-devices \
--disable-debug \
--disable-stripping \
--disable-bzlib \
--enable-asm \
--enable-sram \
--enable-pthreads \
--extra-ldflags="-L/E/code/qiyi/forwindows/android-ndk-r8b/platforms/android-8/arch-arm/usr/lib \
$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a/libsupc++.a  \
$SYSROOT/usr/lib/libstdc++.a -nostartfiles  \
$TOOLCHAIN/lib/gcc/arm-linux-androideabi/4.6.x-google/libgcc.a -lstdc++ \
-L$ANDROID_LIBS -Wl,-rpath-link,$ANDROID_LIBS -L$ARM_ROOT/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a" \
--extra-cxxflags="$EXTRA_CXXFLAGS -lstdc++ -lz" \
--enable-${MODE}


echo "update config.h for arm-$1"

elif [ "$ConfigMode" = "x86" ];then
./configure \
--disable-logging \
--enable-gpl \
--enable-version3 \
--enable-nonfree \
--disable-doc \
--disable-ffplay \
--disable-ffprobe \
--disable-ffserver \
--disable-avdevice \
--disable-postproc \
--enable-fastdiv \
--disable-decoder=vp8 \
--disable-devices \
--disable-debug \
--disable-stripping \
--disable-zlib \
--disable-bzlib \
--disable-altivec \
--disable-amd3dnow \
--disable-amd3dnowext \
--disable-mmx \
--disable-mmx2 \
--enable-sse \
--enable-ssse3 \
--disable-avx \
--disable-armv5te \
--disable-armv6 \
--disable-armv6t2 \
--disable-armvfp \
--disable-iwmmxt \
--disable-mmi \
--disable-neon \
--disable-vis \
--enable-yasm \
--enable-pic \
--enable-sram

echo "update config.h for $1"
fi
sed -i "s/restrict restrict/restrict/g" config.h
sed -i "s/HAVE_LOG2 1/HAVE_LOG2 0/g" config.h
sed -i "s/HAVE_LOG2F 1/HAVE_LOG2F 0/g" config.h
sed -i "s/HAVE_POSIX_MEMALIGN 1/HAVE_POSIX_MEMALIGN 0/g" config.h

sed -i 's/HAVE_LRINT 0/HAVE_LRINT 1/g' config.h
sed -i 's/HAVE_LRINTF 0/HAVE_LRINTF 1/g' config.h
sed -i 's/HAVE_ROUND 0/HAVE_ROUND 1/g' config.h
sed -i 's/HAVE_ROUNDF 0/HAVE_ROUNDF 1/g' config.h
sed -i 's/HAVE_TRUNC 0/HAVE_TRUNC 1/g' config.h
sed -i 's/HAVE_TRUNCF 0/HAVE_TRUNCF 1/g' config.h
#gcc -I. -E libavcodec/avcodec.h | libavcodec/codec_names.sh config.h libavcodec/codec_names.h
