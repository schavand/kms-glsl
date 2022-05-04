/*** libcamera_wrap.h ***/
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
    enum libcameraMsgType
	{
		RequestComplete,
		Quit
	};

   	typedef enum OptionList {awb_gain_r,awb_gain_b} OptionList;

    typedef struct libcameraAppHandle libcameraAppHandle;
    typedef struct libcameraAppMsg libcameraAppMsg;
    typedef struct libcameraOptions libcameraOptions;
    typedef struct libcameraStream libcameraStream;
    typedef struct libcameraStreamInfo libcameraStreamInfo; 
    typedef struct libcameraCompletedRequestPtr libcameraCompletedRequestPtr;

    libcameraAppHandle *createLibcameraApp();
    void OpenCamera(libcameraAppHandle *h);
    void CloseCamera(libcameraAppHandle *h);
    libcameraOptions *GetOptions(libcameraAppHandle *h);
    bool ParseOptions(libcameraOptions *o, int argc, char *argv[]);
    void ConfigureViewfinder(libcameraAppHandle *h);
    void ConfigureStill(libcameraAppHandle *h, unsigned int flags);
    void ConfigureVideo(libcameraAppHandle *h, unsigned int flags);
    void Teardown(libcameraAppHandle *h);
    void StartCamera(libcameraAppHandle *h);
    void StopCamera(libcameraAppHandle *h);
    void libcameraWait(libcameraAppHandle *h, libcameraAppMsg *m);
    libcameraAppMsg *createAppMsg();
    int getLibcameraMessageType(libcameraAppMsg *m);
    void *getLibcameraMessagePayload(libcameraAppMsg *m);
    void ShowPreview(libcameraAppHandle *h, libcameraCompletedRequestPtr *completed_request, libcameraStream *stream);
    uint8_t *getFrame(libcameraAppHandle *h, libcameraAppMsg *msg, libcameraStream *s);
    libcameraCompletedRequestPtr *getCompletedRequestPtr(libcameraAppMsg *m);
    libcameraStream *getViewfinderStream(libcameraAppHandle *h, libcameraStreamInfo *info);
    libcameraStream *getRawStream(libcameraAppHandle *h, libcameraStreamInfo *info);
    libcameraStream *getVideoStream(libcameraAppHandle *h, libcameraStreamInfo *info);
    libcameraStream *getLoresStream(libcameraAppHandle *h, libcameraStreamInfo *info);
    libcameraStream *getStillStream(libcameraAppHandle *h, libcameraStreamInfo *info);
    libcameraStreamInfo *createStreamInfo();
    uint32_t getStreamInfoWidth(libcameraStreamInfo *info);
    uint32_t getStreamInfoHeight(libcameraStreamInfo *info);
    uint32_t getStreamInfoStride(libcameraStreamInfo *info);
    uint32_t getStreamInfoPixel(libcameraStreamInfo *info);
    void freeStreamInfo(libcameraStreamInfo *s);
    void freeLibcameraApp(libcameraAppHandle *h);
    void optionSetValue(libcameraOptions *o, OptionList ol, void *value);

#ifdef __cplusplus
}
#endif