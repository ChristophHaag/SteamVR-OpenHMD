//============ Copyright (c) Valve Corporation, All rights reserved. ============

#include "ohmd_config.h" // which hmds and trackers to use

#include <openvr_driver.h>
#include "driverlog.h"

#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>

#if defined( _WINDOWS )
#include <windows.h>
#endif

#include <openhmd.h>

#include <math.h>
#include <stdio.h>

using namespace vr;


#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#elif defined(__GNUC__) || defined(COMPILER_GCC) || defined(__APPLE__)
#define HMD_DLL_EXPORT extern "C" __attribute__((visibility("default")))
#define HMD_DLL_IMPORT extern "C" 
#else
#error "Unsupported Platform."
#endif

ohmd_context* ctx;
ohmd_device* hmd = NULL;
ohmd_device* hmdtracker = NULL;
ohmd_device* lcontroller = NULL;
ohmd_device* rcontroller = NULL;


class COpenHMDDeviceDriverController;
COpenHMDDeviceDriverController *m_OpenHMDDeviceDriverControllerL;
COpenHMDDeviceDriverController *m_OpenHMDDeviceDriverControllerR;


// gets float values from the device and prints them
void print_infof(ohmd_device* hmd, const char* name, int len, ohmd_float_value val)
{
    float f[len];
    ohmd_device_getf(hmd, val, f);
    printf("%-25s", name);
    for(int i = 0; i < len; i++)
        printf("%f ", f[i]);
    printf("\n");
}

// gets int values from the device and prints them
void print_infoi(ohmd_device* hmd, const char* name, int len, ohmd_int_value val)
{
    int iv[len];
    ohmd_device_geti(hmd, val, iv);
    printf("%-25s", name);
    for(int i = 0; i < len; i++)
        printf("%d ", iv[i]);
    printf("\n");
}

// keys for use with the settings API
static const char * const k_pch_Sample_Section = "driver_openhmd";
static const char * const k_pch_Sample_SecondsFromVsyncToPhotons_Float = "secondsFromVsyncToPhotons";
static const char * const k_pch_Sample_DisplayFrequency_Float = "displayFrequency";

HmdQuaternion_t identityquat{ 1, 0, 0, 0};
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

class CWatchdogDriver_OpenHMD : public IVRWatchdogProvider
{
public:
    CWatchdogDriver_OpenHMD()
    {
        DriverLog("Created watchdog object\n");
        m_pWatchdogThread = nullptr;
    }

    virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
    virtual void Cleanup() ;

private:
	std::thread *m_pWatchdogThread;
};

CWatchdogDriver_OpenHMD g_watchdogDriverOpenHMD;


bool g_bExiting = false;

void WatchdogThreadFunction(  )
{
    while ( !g_bExiting )
    {
#if defined( _WINDOWS )
        // on windows send the event when the Y key is pressed.
        if ( (0x01 & GetAsyncKeyState( 'Y' )) != 0 )
        {
            // Y key was pressed.
            vr::VRWatchdogHost()->WatchdogWakeUp();
        }
        std::this_thread::sleep_for( std::chrono::microseconds( 500 ) );
#else
        DriverLog("Watchdog wakeup\n");
        // for the other platforms, just send one every five seconds
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        vr::VRWatchdogHost()->WatchdogWakeUp();
#endif
    }
    DriverLog("Watchdog exit\n");
}

EVRInitError CWatchdogDriver_OpenHMD::Init( vr::IVRDriverContext *pDriverContext )
{
    VR_INIT_WATCHDOG_DRIVER_CONTEXT( pDriverContext );
    InitDriverLog( vr::VRDriverLog() );

    // Watchdog mode on Windows starts a thread that listens for the 'Y' key on the keyboard to
    // be pressed. A real driver should wait for a system button event or something else from the
    // the hardware that signals that the VR system should start up.
    g_bExiting = false;
    
    DriverLog("starting watchdog thread\n");

    m_pWatchdogThread = new std::thread( WatchdogThreadFunction );
    if ( !m_pWatchdogThread )
    {
        DriverLog( "Unable to create watchdog thread\n");
        return VRInitError_Driver_Failed;
    }

    return VRInitError_None;
    }


    void CWatchdogDriver_OpenHMD::Cleanup()
    {
    g_bExiting = true;
    if ( m_pWatchdogThread )
    {
        m_pWatchdogThread->join();
        delete m_pWatchdogThread;
        m_pWatchdogThread = nullptr;
    }

    CleanupDriverLog();
}

vr::TrackedDeviceIndex_t lcindex;
vr::TrackedDeviceIndex_t rcindex;
class COpenHMDDeviceDriverController : public vr::ITrackedDeviceServerDriver /*, public vr::IVRControllerComponent */ {
public:
    int index;
    COpenHMDDeviceDriverController(int index) : index(index) {
        DriverLog("construct controller object %d\n", index);
        this->index = index;
    }
    EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId )
    {
        DriverLog("activate controller %d: %d\n", index, unObjectId);
        if (index == 0) {
            lcindex = unObjectId;
        } else {
            rcindex = unObjectId;
        }
        return VRInitError_None;
    }

    void Deactivate()
    {
        DriverLog("deactivate controller\n");
        //m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    }

    void EnterStandby()
    {
                DriverLog("standby controller\n");
    }

    void *GetComponent( const char *pchComponentNameAndVersion )
    {
        DriverLog("get controller component %s | %s ", pchComponentNameAndVersion, /*vr::IVRControllerComponent_Version*/ "<nothing>");
        if (!strcmp(pchComponentNameAndVersion, /*vr::IVRControllerComponent_Version*/ "<nothing>"))
        {
            DriverLog(": yes\n");
            return NULL;//(vr::IVRControllerComponent*)this;

        }

        DriverLog(": no\n");
        return NULL;
    }

    /** debug request from a client */
    void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
    {
        if( unResponseBufferSize >= 1 )
            pchResponseBuffer[0] = 0;
    }

    DriverPose_t GetPose()
    {
        DriverPose_t pose = { 0 };
        pose.poseIsValid = true;
        pose.result = TrackingResult_Running_OK;
        pose.deviceIsConnected = true;

        ohmd_device* d = index == 0 ? lcontroller : rcontroller;
            
        ohmd_ctx_update(ctx);

        float quat[4];
        ohmd_device_getf(d, OHMD_ROTATION_QUAT, quat);
        pose.qRotation.x = quat[0];
        pose.qRotation.y = quat[1];
        pose.qRotation.z = quat[2];
        pose.qRotation.w = quat[3];

        float pos[3];
        ohmd_device_getf(d, OHMD_POSITION_VECTOR, pos);
        pose.vecPosition[0] = pos[0];
        pose.vecPosition[1] = pos[1];
        pose.vecPosition[2] = pos[2];

        //DriverLog("get controller %d pose %f %f %f %f, %f %f %f\n", index, quat[0], quat[1], quat[2], quat[3], pos[0], pos[1], pos[2]);

        pose.qWorldFromDriverRotation = identityquat;
        pose.qDriverFromHeadRotation = identityquat;

        return pose;
    }

    VRControllerState_t controllerstate;
    VRControllerState_t GetControllerState() {
    DriverLog("get controller state\n");
    //return controllerstate;

    controllerstate.unPacketNum = controllerstate.unPacketNum + 1;
    //TODO: buttons

    //TODO: nolo says when a button was pressed a button was also touched. is that so?
    controllerstate.ulButtonTouched |= controllerstate.ulButtonPressed;

    //uint64_t ulChangedTouched = controllerstate.ulButtonTouched ^ controllerstate.ulButtonTouched;
    //uint64_t ulChangedPressed = controllerstate.ulButtonPressed ^ controllerstate.ulButtonPressed;
    return controllerstate;
    }

    bool TriggerHapticPulse( uint32_t unAxisId, uint16_t usPulseDurationMicroseconds ) {
        return false;
    }

    std::string GetSerialNumber() const { 
        DriverLog("get controller serial number %s\n", m_sSerialNumber.c_str());
        return m_sSerialNumber;
    }

    bool exists() {
        return index == 0 ? lcontroller != NULL : rcontroller != NULL;
    }

private:
    std::string m_sSerialNumber = "Controller serial number " + std::to_string(index);
    std::string m_sModelNumber = "Controller model number " + std::to_string(index);
};

class COpenHMDDeviceDriver final : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
    COpenHMDDeviceDriver(  )
    {
        ctx = ohmd_ctx_create();
        int num_devices = ohmd_ctx_probe(ctx);
        if(num_devices < 0){
            DriverLog("failed to probe devices: %s\n", ohmd_ctx_get_error(ctx));
        }

        for(int i = 0; i < num_devices; i++){
            DriverLog("device %d\n", i);
            DriverLog("  vendor:  %s\n", ohmd_list_gets(ctx, i, OHMD_VENDOR));
            DriverLog("  product: %s\n", ohmd_list_gets(ctx, i, OHMD_PRODUCT));
            DriverLog("  path:    %s\n\n", ohmd_list_gets(ctx, i, OHMD_PATH));
        }

        int hmddisplay = get_configvalues()[0];
        int hmdtrackerindex = get_configvalues()[1];
        int leftcontroller = get_configvalues()[2];
        int rightcontroller = get_configvalues()[3];

        DriverLog("Using HMD Display %d, HMD Tracker %d, Left Controller %d, Right Controller %d\n", hmddisplay, hmdtrackerindex, leftcontroller, rightcontroller);
        hmd = ohmd_list_open_device(ctx, hmddisplay);

        if (hmdtrackerindex != -1 && hmdtrackerindex != hmddisplay) hmdtracker = ohmd_list_open_device(ctx, hmdtrackerindex);
        if (leftcontroller != -1) lcontroller = ohmd_list_open_device(ctx, leftcontroller);
        if (rightcontroller != -1) rcontroller = ohmd_list_open_device(ctx, rightcontroller);

        if(!hmd){
            DriverLog("failed to open device: %s\n", ohmd_ctx_get_error(ctx));
        }

        int ivals[2];
        ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, ivals);
        ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, ivals + 1);
        //DriverLog("resolution:              %i x %i\n", ivals[0], ivals[1]);

        /*
        print_infof(hmd, "hsize:",            1, OHMD_SCREEN_HORIZONTAL_SIZE);
        print_infof(hmd, "vsize:",            1, OHMD_SCREEN_VERTICAL_SIZE);
        print_infof(hmd, "lens separation:",  1, OHMD_LENS_HORIZONTAL_SEPARATION);
        print_infof(hmd, "lens vcenter:",     1, OHMD_LENS_VERTICAL_POSITION);
        print_infof(hmd, "left eye fov:",     1, OHMD_LEFT_EYE_FOV);
        print_infof(hmd, "right eye fov:",    1, OHMD_RIGHT_EYE_FOV);
        print_infof(hmd, "left eye aspect:",  1, OHMD_LEFT_EYE_ASPECT_RATIO);
        print_infof(hmd, "right eye aspect:", 1, OHMD_RIGHT_EYE_ASPECT_RATIO);
        print_infof(hmd, "distortion k:",     6, OHMD_DISTORTION_K);

        print_infoi(hmd, "digital button count:", 1, OHMD_BUTTON_COUNT);
        */

        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
        m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

        DriverLog( "Using settings values\n" );
        ohmd_device_getf(hmd, OHMD_EYE_IPD, &m_flIPD);
        
        {
            std::stringstream buf;
            buf << ohmd_list_gets(ctx, 0, OHMD_PRODUCT);
            buf << ": ";
            buf << ohmd_list_gets(ctx, 0, OHMD_PATH);
            m_sSerialNumber = buf.str();
        }

        {
            std::stringstream buf;
            buf << "OpenHMD: ";
            buf << ohmd_list_gets(ctx, 0, OHMD_PRODUCT);
            m_sModelNumber = buf.str();
        }
        
        // Important to pass vendor through. Gaze cursor is only available for "Oculus". So grab the first word.
        char const* vendor_override = getenv("OHMD_VENDOR_OVERRIDE");
        if (vendor_override) {
            m_sVendor = vendor_override;
        } else {
            m_sVendor = ohmd_list_gets(ctx, 0, OHMD_VENDOR);
            if (m_sVendor.find(' ') != std::string::npos) {
                m_sVendor = m_sVendor.substr(0, m_sVendor.find(' '));
            }
        }

        m_nWindowX = 1920; //TODO: real window offset
        m_nWindowY = 0;
        ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &m_nWindowWidth);
        ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &m_nWindowHeight );
        ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &m_nRenderWidth);
        ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &m_nRenderHeight );
        //m_nRenderWidth /= 2;
        //m_nRenderHeight /= 2;

        m_flSecondsFromVsyncToPhotons = vr::VRSettings()->GetFloat( k_pch_Sample_Section, k_pch_Sample_SecondsFromVsyncToPhotons_Float );
        //TODO: find actual frequency somehow (from openhmd?)
        m_flDisplayFrequency = vr::VRSettings()->GetFloat( k_pch_Sample_Section, k_pch_Sample_DisplayFrequency_Float );

        DriverLog( "driver_openhmd: Vendor: %s\n", m_sVendor.c_str() );
        DriverLog( "driver_openhmd: Serial Number: %s\n", m_sSerialNumber.c_str() );
        DriverLog( "driver_openhmd: Model Number: %s\n", m_sModelNumber.c_str() );
        DriverLog( "driver_openhmd: Window: %d %d %d %d\n", m_nWindowX, m_nWindowY, m_nWindowWidth, m_nWindowHeight );
        DriverLog( "driver_openhmd: Render Target: %d %d\n", m_nRenderWidth, m_nRenderHeight );
        DriverLog( "driver_openhmd: Seconds from Vsync to Photons: %f\n", m_flSecondsFromVsyncToPhotons );
                                                DriverLog( "driver_openhmd: Display Frequency: %f\n", m_flDisplayFrequency );
        DriverLog( "driver_openhmd: IPD: %f\n", m_flIPD );

        float distortion_coeffs[4];
        ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(distortion_coeffs[0]));
        DriverLog("driver_openhmd: Distortion values a=%f b=%f c=%f d=%f\n", distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3]);
    }

    ~COpenHMDDeviceDriver()
    {
    }


    EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId )
    {
        m_unObjectId = unObjectId;
        m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer( m_unObjectId );

        vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ManufacturerName_String, m_sVendor.c_str());
        vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str() );
        vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str() );
        vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD );
        vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f );
        vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency );
        vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons );

        //float sep;

        // return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
        vr::VRProperties()->SetUint64Property( m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2 );

        return VRInitError_None;
    }

    void Deactivate()
    {
        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    }

    void EnterStandby()
    {
    }

    void *GetComponent( const char *pchComponentNameAndVersion )
    {
        if ( !strcmp( pchComponentNameAndVersion, vr::IVRDisplayComponent_Version ) )
        {
            return (vr::IVRDisplayComponent*)this;
        }
        return NULL;
    }

    void PowerOff()
    {
    }

    void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
    {
        if( unResponseBufferSize >= 1 )
            pchResponseBuffer[0] = 0;
    }

    void GetWindowBounds( int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight )
    {
        *pnX = m_nWindowX;
        *pnY = m_nWindowY;
        *pnWidth = m_nWindowWidth;
        *pnHeight = m_nWindowHeight;
    }

    bool IsDisplayOnDesktop()
    {
        return true;
    }

    bool IsDisplayRealDisplay()
    {
        return true;
    }

    void GetRecommendedRenderTargetSize( uint32_t *pnWidth, uint32_t *pnHeight )
    {
        *pnWidth = m_nRenderWidth;
        *pnHeight = m_nRenderHeight;
    }

    void GetEyeOutputViewport( EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight )
    {
        *pnY = 0;
        *pnWidth = m_nWindowWidth / 2;
        *pnHeight = m_nWindowHeight;

        if ( eEye == Eye_Left )
        {
            *pnX = 0;
        }
        else
        {
            *pnX = m_nWindowWidth / 2;
        }
    }

    // flatten 2D indices in a 4x4 matrix explicit so it's easy to see what's happening:
    int f1(int i, int j)
    {
        if (i == 0 && j == 0) return 0;
        if (i == 0 && j == 1) return 1;
        if (i == 0 && j == 2) return 2;
        if (i == 0 && j == 3) return 3;

        if (i == 1 && j == 0) return 4;
        if (i == 1 && j == 1) return 5;
        if (i == 1 && j == 2) return 6;
        if (i == 1 && j == 3) return 7;

        if (i == 2 && j == 0) return 8;
        if (i == 2 && j == 1) return 9;
        if (i == 2 && j == 2) return 10;
        if (i == 2 && j == 3) return 11;

        if (i == 3 && j == 0) return 12;
        if (i == 3 && j == 1) return 13;
        if (i == 3 && j == 2) return 14;
        if (i == 3 && j == 3) return 15;

        return -1;
    }

    int f2(int i, int j)
    {
        if (i == 0 && j == 0) return 0;
        if (i == 0 && j == 1) return 4;
        if (i == 0 && j == 2) return 8;
        if (i == 0 && j == 3) return 12;

        if (i == 1 && j == 0) return 1;
        if (i == 1 && j == 1) return 5;
        if (i == 1 && j == 2) return 9;
        if (i == 1 && j == 3) return 13;

        if (i == 2 && j == 0) return 2;
        if (i == 2 && j == 1) return 6;
        if (i == 2 && j == 2) return 10;
        if (i == 2 && j == 3) return 14;

        if (i == 3 && j == 0) return 3;
        if (i == 3 && j == 1) return 7;
        if (i == 3 && j == 2) return 11;
        if (i == 3 && j == 3) return 15;

        return -1;
    }

    void columnMatrixToAngles(float *yaw, float *pitch, float *roll, float colMatrix[4][4] ) {
        double sinPitch, cosPitch, sinRoll, cosRoll, sinYaw, cosYaw;

        sinPitch = -colMatrix[2][0];
        cosPitch = sqrt(1 - sinPitch*sinPitch);

        if ( abs(cosPitch) > 0.1) 
        {
            sinRoll = colMatrix[2][1] / cosPitch;
            cosRoll = colMatrix[2][2] / cosPitch;
            sinYaw = colMatrix[1][0] / cosPitch;
            cosYaw = colMatrix[0][0] / cosPitch;
        } 
        else 
        {
            sinRoll = -colMatrix[1][2];
            cosRoll = colMatrix[1][1];
            sinYaw = 0;
            cosYaw = 1;
        }

        *yaw   = atan2(sinYaw, cosYaw) * 180 / M_PI;
        *pitch = atan2(sinPitch, cosPitch) * 180 / M_PI;
        *roll  = atan2(sinRoll, cosRoll) * 180 / M_PI;
    } 
    
    typedef union {
        float m[4][4];
        float arr[16];
    } mat4x4f;

    void omat4x4f_mult(const mat4x4f* l, const mat4x4f* r, mat4x4f *o) {
        for(int i = 0; i < 4; i++){
            float a0 = l->m[i][0], a1 = l->m[i][1], a2 = l->m[i][2], a3 = l->m[i][3];
            o->m[i][0] = a0 * r->m[0][0] + a1 * r->m[1][0] + a2 * r->m[2][0] + a3 * r->m[3][0];
            o->m[i][1] = a0 * r->m[0][1] + a1 * r->m[1][1] + a2 * r->m[2][1] + a3 * r->m[3][1];
            o->m[i][2] = a0 * r->m[0][2] + a1 * r->m[1][2] + a2 * r->m[2][2] + a3 * r->m[3][2];
            o->m[i][3] = a0 * r->m[0][3] + a1 * r->m[1][3] + a2 * r->m[2][3] + a3 * r->m[3][3];
        }
    }
    
    void createUnRotation(float angle, mat4x4f *m) {
        memset(m, 0, sizeof(*m));
        m->m[0][0] = 1.0f;
        m->m[1][1] = 1.0f;
        m->m[2][2] = 1.0f;
        m->m[3][3] = 1.0f;
        
        DriverLog("Unrotating for angle %f\n", angle);
        
        if (angle > -5 && angle < 5) {
            return;
        }
        
        else if (angle > 85 && angle < 95) {
            m->m[0][0] = 0.0f; m->m[0][1] = -1.0f; m->m[1][0] = 1.0f; m->m[1][1] = 0.0f;
            m->m[0][1] = 1.0f; m->m[1][0] = -1.0f;
        }

        else if (angle > -95 && angle < -85) {
            m->m[0][0] = 0.0f; m->m[0][1] = -1.0f; m->m[1][0] = 1.0f; m->m[1][1] = 0.0f;
        }
        
        else {
            DriverLog("UNIMPLEMENTED ROTATION!!!\n");
        }
    }

    void GetProjectionRaw( EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom )
    {
        mat4x4f ohmdprojection;
        if (eEye == Eye_Left) {
            ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_PROJECTION_MATRIX, ohmdprojection.arr);
        } else {
            ohmd_device_getf(hmd, OHMD_RIGHT_EYE_GL_PROJECTION_MATRIX, ohmdprojection.arr);
        }

        float yaw, pitch, roll;
        float p[4][4];
        memcpy(p, ohmdprojection.arr, 16 * sizeof(float));
        columnMatrixToAngles(&yaw, &pitch, &roll, p);
        
        if (eEye == Eye_Left) {
            rotation_left = yaw;
        } else {
            rotation_right = yaw;
        }
        
        mat4x4f unrotation;
        createUnRotation(yaw, &unrotation);
 
        DriverLog("unrotation\n%f %f %f %f\n%f %f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
            unrotation.arr[0], unrotation.arr[1], unrotation.arr[2], unrotation.arr[3],
            unrotation.arr[4], unrotation.arr[5], unrotation.arr[6], unrotation.arr[7],
            unrotation.arr[8], unrotation.arr[9], unrotation.arr[10], unrotation.arr[11],
            unrotation.arr[12], unrotation.arr[13], unrotation.arr[14], unrotation.arr[15]);
            
        omat4x4f_mult(&ohmdprojection, &unrotation, &ohmdprojection);
        
        // http://stackoverflow.com/questions/10830293/ddg#12926655
        // get projection matrix from openhmd, convert it into lrtb + near,far with SO formula
        // then divide by near plane distance to get the tangents of the angles from the center plane (tan = opposite side = these values divided by adjacent side = near plane distance)
        // but negate top and bottom. who knows why. there are 3 or so issues for it on github

        // f2 switches row-major and column-major
        float m00 = ohmdprojection.arr[f2(0,0)];
        //float m03 = ohmdprojection[f2(0,3)];
        //float m10 = ohmdprojection[f2(1,3)];
        float m11 = ohmdprojection.arr[f2(1,1)];
        //float m13 = ohmdprojection[f2(1,3)];
        float m23 = ohmdprojection.arr[f2(2,3)];
        float m22 = ohmdprojection.arr[f2(2,2)];
        float m12 = ohmdprojection.arr[f2(1,2)];
        float m02 = ohmdprojection.arr[f2(0,2)];

        float near   = m23/(m22-1);
        float far    = m23/(m22+1);
        *pfBottom = -   (m12-1)/m11;
        *pfTop    = -   (m12+1)/m11;
        *pfLeft   =     (m02-1)/m00;
        *pfRight  =     (m02+1)/m00;
        
        DriverLog("m 00 %f, 11 %f, 22 %f, 12 %f, 02 %f\n", m00, m11, m23, m22, m12, m02);

        DriverLog("ohmd projection\n%f %f %f %f\n%f %f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
            ohmdprojection.arr[0], ohmdprojection.arr[1], ohmdprojection.arr[2], ohmdprojection.arr[3],
            ohmdprojection.arr[4], ohmdprojection.arr[5], ohmdprojection.arr[6], ohmdprojection.arr[7],
            ohmdprojection.arr[8], ohmdprojection.arr[9], ohmdprojection.arr[10], ohmdprojection.arr[11],
            ohmdprojection.arr[12], ohmdprojection.arr[13], ohmdprojection.arr[14], ohmdprojection.arr[15]
        );
        
        DriverLog("projectionraw values lrtb, near far: %f %f %f %f | %f %f\n", *pfLeft, *pfRight, *pfTop, *pfBottom, near, far);
        
        //DriverLog("angles %f %f %f\n", yaw, pitch, roll);
    }

    DistortionCoordinates_t ComputeDistortion( EVREye eEye, float fU, float fV )
    {
        float angle = (eEye == Eye_Left ? rotation_left : rotation_right);
        
        //DriverLog("Eye %d before: %f %f\n", eEye, fU, fV);
                
        if (angle > -5 && angle < 5) {
        } else if (angle > 85 && angle < 95) {
            float tmp = fV;
            fV = fU;
            fU = tmp;
        } else if (angle > -95 && angle < -85) {
            float tmp = fV;
            fV = fU;
            fU = 1.f - tmp;
        } else {
            float x = 0 * fU + -1 * fV;
            float y = -1 * fU + 0 * 0 * fV;
            fU = x;
            fV = y;
        }
        
        //DriverLog("Eye %d after: %f %f\n", eEye, fU, fV);
        
        int hmd_w;
        int hmd_h;
        ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
        ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);
        float ipd;
        ohmd_device_getf(hmd, OHMD_EYE_IPD, &ipd);
        float viewport_scale[2];
        float distortion_coeffs[4];
        float aberr_scale[3];
        float sep;
        float left_lens_center[2];
        float right_lens_center[2];
        //viewport is half the screen
        ohmd_device_getf(hmd, OHMD_SCREEN_HORIZONTAL_SIZE, &(viewport_scale[0]));
        viewport_scale[0] /= 2.0f;
        ohmd_device_getf(hmd, OHMD_SCREEN_VERTICAL_SIZE, &(viewport_scale[1]));
        //distortion coefficients
        ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(distortion_coeffs[0]));
        ohmd_device_getf(hmd, OHMD_UNIVERSAL_ABERRATION_K, &(aberr_scale[0]));
        //calculate lens centers (assuming the eye separation is the distance betweenteh lense centers)
        ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);
        ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(left_lens_center[1]));
        ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(right_lens_center[1]));
        left_lens_center[0] = viewport_scale[0] - sep/2.0f;
        right_lens_center[0] = sep/2.0f;
        //asume calibration was for lens view to which ever edge of screen is further away from lens center
        float warp_scale = (left_lens_center[0] > right_lens_center[0]) ? left_lens_center[0] : right_lens_center[0];

        float lens_center[2];
        lens_center[0] = (eEye == Eye_Left ? left_lens_center[0] : right_lens_center[0]);
        lens_center[1] = (eEye == Eye_Left ? left_lens_center[1] : right_lens_center[1]);

        float r[2];
        r[0] = fU * viewport_scale[0] - lens_center[0];
        r[1] = fV * viewport_scale[1] - lens_center[1];

        r[0] /= warp_scale;
        r[1] /= warp_scale;

        float r_mag = sqrt(r[0] * r[0] + r[1] * r[1]);


        float r_displaced[2];
        r_displaced[0] = r[0] * (distortion_coeffs[3] + distortion_coeffs[2] * r_mag + distortion_coeffs[1] * r_mag * r_mag + distortion_coeffs[0] * r_mag * r_mag * r_mag);

        r_displaced[1] = r[1] * (distortion_coeffs[3] + distortion_coeffs[2] * r_mag + distortion_coeffs[1] * r_mag * r_mag + distortion_coeffs[0] * r_mag * r_mag * r_mag);

        r_displaced[0] *= warp_scale;
        r_displaced[1] *= warp_scale;

        float tc_r[2];
        tc_r[0] = (lens_center[0] + aberr_scale[0] * r_displaced[0]) / viewport_scale[0];
        tc_r[1] = (lens_center[1] + aberr_scale[0] * r_displaced[1]) / viewport_scale[1];

        float tc_g[2];
        tc_g[0] = (lens_center[0] + aberr_scale[1] * r_displaced[0]) / viewport_scale[0];
        tc_g[1] = (lens_center[1] + aberr_scale[1] * r_displaced[1]) / viewport_scale[1];

        float tc_b[2];
        tc_b[0] = (lens_center[0] + aberr_scale[2] * r_displaced[0]) / viewport_scale[0];
        tc_b[1] = (lens_center[1] + aberr_scale[2] * r_displaced[1]) / viewport_scale[1];

        //DriverLog("Distort %f %f -> %f %f; %f %f %f %f\n", fU, fV, tc_b[0], tc_b[1], distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3]);

        DistortionCoordinates_t coordinates;
        coordinates.rfBlue[0] = tc_b[0];
        coordinates.rfBlue[1] = tc_b[1];
        coordinates.rfGreen[0] = tc_g[0];
        coordinates.rfGreen[1] = tc_g[1];
        coordinates.rfRed[0] = tc_r[0];
        coordinates.rfRed[1] = tc_r[1];
        return coordinates;
    }

    DriverPose_t GetPose()
    {
        DriverPose_t pose = { 0 };
        pose.poseIsValid = true;
        pose.result = TrackingResult_Running_OK;
        pose.deviceIsConnected = true;

        ohmd_device* d = hmdtracker ? hmdtracker : hmd;
        ohmd_ctx_update(ctx);

        float quat[4];
        ohmd_device_getf(d, OHMD_ROTATION_QUAT, quat);
        pose.qRotation.x = quat[0];
        pose.qRotation.y = quat[1];
        pose.qRotation.z = quat[2];
        pose.qRotation.w = quat[3];

        float pos[3];
        ohmd_device_getf(d, OHMD_POSITION_VECTOR, pos);
        pose.vecPosition[0] = pos[0];
        pose.vecPosition[1] = pos[1];
        pose.vecPosition[2] = pos[2];

        //printf("%f %f %f %f  %f %f %f\n", quat[0], quat[1], quat[2], quat[3], pos[0], pos[1], pos[2]);
        //fflush(stdout);
        //DriverLog("get hmd pose %f %f %f %f, %f %f %f\n", quat[0], quat[1], quat[2], quat[3], pos[0], pos[1], pos[2]);

        pose.qWorldFromDriverRotation = identityquat;
        pose.qDriverFromHeadRotation = identityquat;

        return pose;
    }


    void RunFrame()
    {
        // In a real driver, this should happen from some pose tracking thread.
        // The RunFrame interval is unspecified and can be very irregular if some other
        // driver blocks it for some periodic task.
        if ( m_unObjectId != vr::k_unTrackedDeviceIndexInvalid )
        {
            vr::VRServerDriverHost()->TrackedDevicePoseUpdated( m_unObjectId, GetPose(), sizeof( DriverPose_t ) );
            if (m_OpenHMDDeviceDriverControllerL->exists())
                vr::VRServerDriverHost()->TrackedDevicePoseUpdated( lcindex, m_OpenHMDDeviceDriverControllerL->GetPose(), sizeof( DriverPose_t ) );
            if (m_OpenHMDDeviceDriverControllerR->exists())
                vr::VRServerDriverHost()->TrackedDevicePoseUpdated( rcindex, m_OpenHMDDeviceDriverControllerR->GetPose(), sizeof( DriverPose_t ) );
        }
    }

    std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
    vr::TrackedDeviceIndex_t m_unObjectId;
    vr::PropertyContainerHandle_t m_ulPropertyContainer;

    std::string m_sVendor;
    std::string m_sSerialNumber;
    std::string m_sModelNumber;

    int32_t m_nWindowX;
    int32_t m_nWindowY;
    int32_t m_nWindowWidth;
    int32_t m_nWindowHeight;
    int32_t m_nRenderWidth;
    int32_t m_nRenderHeight;
    float m_flSecondsFromVsyncToPhotons;
    float m_flDisplayFrequency;
    float m_flIPD;
    
    float rotation_left = 0.0;
    float rotation_right = 0.0;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CServerDriver_OpenHMD: public IServerTrackedDeviceProvider
{
public:
    CServerDriver_OpenHMD()
        : m_OpenHMDDeviceDriver( NULL )
    {
    }

    virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
    virtual void Cleanup() ;
    virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
    virtual void RunFrame() ;
    virtual bool ShouldBlockStandbyMode()  { return false; }
    virtual void EnterStandby()  {}
    virtual void LeaveStandby()  {}

private:
    COpenHMDDeviceDriver *m_OpenHMDDeviceDriver;
};

CServerDriver_OpenHMD g_serverDriverOpenHMD;

EVRInitError CServerDriver_OpenHMD::Init( vr::IVRDriverContext *pDriverContext )
{
    VR_INIT_SERVER_DRIVER_CONTEXT( pDriverContext );
    InitDriverLog( vr::VRDriverLog() );

    m_OpenHMDDeviceDriver = new COpenHMDDeviceDriver();
    vr::VRServerDriverHost()->TrackedDeviceAdded( m_OpenHMDDeviceDriver->GetSerialNumber().c_str(), vr::TrackedDeviceClass_HMD, m_OpenHMDDeviceDriver );

    m_OpenHMDDeviceDriverControllerL = new COpenHMDDeviceDriverController(0);
    m_OpenHMDDeviceDriverControllerR = new COpenHMDDeviceDriverController(1);
    if (m_OpenHMDDeviceDriverControllerL->exists()) {
        vr::VRServerDriverHost()->TrackedDeviceAdded( m_OpenHMDDeviceDriverControllerL->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, m_OpenHMDDeviceDriverControllerL );
    }
    if (m_OpenHMDDeviceDriverControllerR->exists()) {
        vr::VRServerDriverHost()->TrackedDeviceAdded(  m_OpenHMDDeviceDriverControllerR->GetSerialNumber().c_str(), vr::TrackedDeviceClass_Controller, m_OpenHMDDeviceDriverControllerR );
    }

    return VRInitError_None;
}

void CServerDriver_OpenHMD::Cleanup()
{
    CleanupDriverLog();
    delete m_OpenHMDDeviceDriver;
    m_OpenHMDDeviceDriver = NULL;
}


void CServerDriver_OpenHMD::RunFrame()
{
    if ( m_OpenHMDDeviceDriver )
    {
        m_OpenHMDDeviceDriver->RunFrame();
    }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
HMD_DLL_EXPORT void *HmdDriverFactory( const char *pInterfaceName, int *pReturnCode )
{
    if( 0 == strcmp( IServerTrackedDeviceProvider_Version, pInterfaceName ) )
    {
        return &g_serverDriverOpenHMD;
    }
    if( 0 == strcmp( IVRWatchdogProvider_Version, pInterfaceName ) )
    {
        return &g_watchdogDriverOpenHMD;
    }

    DriverLog("no interface %s\n", pInterfaceName);

    if( pReturnCode )
        *pReturnCode = VRInitError_Init_InterfaceNotFound;

    return NULL;
}
