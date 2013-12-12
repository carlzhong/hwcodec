#pragma once

#ifndef _QCOM_OMX_PUBLIC_INTERFACE_H
#define _QCOM_OMX_PUBLIC_INTERFACE_H


#define BUILD_FOR_GINGERBREAD 0

// Change this define to 1 if building on a Honeycomb or up tree
#define BUILD_FOR_HONEYCOMB_AND_UP 1 //carlzhong 0

#if !BUILD_FOR_HONEYCOMB_AND_UP

#define ENABLE_OMX_RENDERER 01

#endif



#define LOG_VERBOSE 01


//
#define USE_QCOM_PICTURE_ORDER  0

#include <jni.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum {
	kQcomOmxInterfaceErrorSuccess,
	kQcomOmxInterfaceErrorCouldNotAcquireMediaPlayerService,
	kQcomOmxInterfaceErrorCouldNotCreateQcomOmxInterface,
	kQcomOmxInterfaceErrorCouldNotAcquireiOmxInterface,
	kQcomOmxInterfaceErrorCouldNotAllocateOmxComponent,
	kQcomOmxInterfaceErrorCouldNotCreateQcOmxCodecObserver,
	kQcomOmxInterfaceErrorOmxComponentNotFound,
	kQcomOmxInterfaceErrorBuffersNotRegistered,

	kQcomOmxInterfaceErrorFillingBuffer,
	kQcomOmxInterfaceErrorPortSettingChangeFail,

	kQcomOmxInterfaceErrorInvalidState,
	kQcomOmxInterfaceErrorIncorrectStateReached,
	kQcomOmxInterfaceErrorOmxStateNotExecuting,

	kQcomOmxInterfaceErrorCouldNotGetPortDefinition,
	kQcomOmxInterfaceErrorCouldNotSetPortDefinition,
	kQcomOmxInterfaceErrorCouldNotAllocateMemory,
	kQcomOmxInterfaceErrorCouldNotRequestIFrame,
	kQcomOmxInterfaceErrorCouldNotGetBitRateParameters,
	kQcomOmxInterfaceErrorCouldNotSetBitRateParameters,
	kQcomOmxInterfaceErrorCouldNotSetCodecConfiguration,

	kQcomOmxInterfaceErrorCouldNotSendCommandState,
	kQcomOmxInterfaceErrorCouldNotSetExecutionState,

	kQcomOmxInterfaceErrorCouldNotCreateEventThread,
	kQcomOmxInterfaceErrorCouldNotCreateOutputThread,

	kQcomOmxInterfaceErrorBadBufferFound,
	kQcomOmxInterfaceErrorBufferAlreadyOwned,
	kQcomOmxInterfaceErrorBufferNotEmptied,
	kQcomOmxInterfaceErrorOmxEventError,

	kQcomOmxInterfaceErrorInvalidEncodeParameters,
	kQcomOmxInterfaceErrorCouldNotSetupRenderer,

	kQcomOmxInterfaceErrorSettingDecoderPictureOrder,
	kQcomOmxInterfaceErrorSettingFramePackingFormat,

	kQcomOmxInterfaceErrorSeeLogs

} QcomOmxInterfaceError;

typedef enum {
       kDecoderPictureOrderDisplay = 0,
       kDecoderPictureOrderDecode
} DecoderPictureOrder;


typedef enum {
       kFramePackingFormatUnspecified = 0,
       kFramePackingFormatOneCompleteFrame,
       kFramePackingFormatArbitrary
} FramePackingFormat;


typedef enum {
       kVideoCompressionFormatAVC = 0,
       kVideoCompressionFormatMPEG4,
       kVideoCompressionFormatH263,
       kVideoCompressionFormatWMV
} VideoCompressionFormat;

const char *resultDescription(int result);

struct encoderParams {
	int frameWidth;
	int frameHeight;
 	int frameRate;
	int rateControl;
	int bitRate;
	const char *codecString;
};


typedef enum {
	kHardwareBaseUnknown,
	kHardwareBaseUncatalogued,
	kHardwareBase7x30,
	kHardwareBase8x55,
	kHardwareBase8x60
} HardwareBase;


typedef enum {
	kOMXOperationTypeDecode = 0,
	kOMXOperationTypeEncode
} OMXOperationType;

const char *resultDescription(int result);



typedef int (*_QcomOmxInputCallback)(void *obj, void *userData);


typedef int (*_QcomOmxOutputCallback)(void *obj, void *buffer, size_t bufferSize, void *iomxBuffer, void *userData);

extern "C" { 
	bool omx_component_is_available(const char *hardwareCodecString);


	void *encoder_create(int *errorCode, encoderParams *params);

	void *decoder_create(int *errorCode, const char *codecString = NULL);

	int   omx_interface_init(void *obj);

	int   omx_interface_deinit(void *obj);


	int   omx_interface_destroy(void *obj);

	int   omx_interface_error(void *obj);

	int   omx_interface_register_input_callback(void *obj, _QcomOmxInputCallback function, void *userData = NULL);
	int   omx_interface_register_output_callback(void *obj, _QcomOmxOutputCallback function, void *userData = NULL);


	int   omx_interface_send_input_data(void *obj, void *buffer, size_t size, int timeStampLfile);

	int   omx_interface_send_end_of_input_flag(void *obj, int timeStampLfile);
	           
	int	  omx_interface_reserve_input_buffer(void *obj, void **buffer, void **memPtr);

	int   omx_interface_send_input_buffer(void *obj, void *buffer, int size, long timeStampFile);

	int   omx_interface_send_final_buffer(void *obj, void *buffer, long timeStampFile);

	int   getHardwareBaseVersion();

	int decode_file(const char *fileFrom, const char *fileTo, const char *codecString = NULL);


	int encode_file(const char *fileFrom, const char *fileTo, encoderParams *params);

	int omx_setup_input_semaphore(void *omx);

	int omx_teardown_input_semaphore();

	int omx_send_data_frame_to_encoder(void *omx, void *frameBytes, int width, int height, long timeStamp);

	int	  omx_interface_decode_from_file(void *obj, const char *fileFrom);

//	int video_encoder_test(int argc, char *argv[]);//carlzhong

}
extern int video_encoder_test(int argc, char *argv[]); //carlzhong
#endif
