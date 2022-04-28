/*** libcamera_wrap.h ***/
#ifdef __cplusplus
extern "C" {
#endif
    typedef void * libcameraAppHandle;
    typedef void * libcameraAppMsg;

    libcameraAppHandle createLibcameraApp();
    void OpenCamera(libcameraAppHandle h);
    void CloseCamera(libcameraAppHandle h);
    void ConfigureViewfinder(libcameraAppHandle h);
    void ConfigureStill(libcameraAppHandle h, unsigned int flags);
    void ConfigureVideo(libcameraAppHandle h, unsigned int flags);
    void Teardown(libcameraAppHandle h);
    void StartCamera(libcameraAppHandle h);
    void StopCamera(libcameraAppHandle h);
    void Wait(libcameraAppHandle h, libcameraAppMsg m);
    libcameraAppMsg createLibcameraAppMsg();
    void freeLibcameraAppMsg(libcameraAppMsg m);
    int getLibcameraMessageType(libcameraAppMsg m);
    void *getLibcameraMessagePayload(libcameraAppMsg m);
    void ShowPreview(libcameraAppHandle h, void* completed_request, void *stream);
    void    freeLibcameraApp(libcameraAppHandle h);
#ifdef __cplusplus
}
#endif