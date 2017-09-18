//============ Copyright (c) Valve Corporation, All rights reserved. ============

#include <openvr_driver.h>
#include "driverlog.h"

#include <vector>
#include <thread>
#include <chrono>
#include <cstring>

#if defined( _WINDOWS )
#include <windows.h>
#endif

#include <openhmd.h>

#include <math.h>

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
		// for the other platforms, just send one every five seconds
		std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
		vr::VRWatchdogHost()->WatchdogWakeUp();
#endif
	}
}

EVRInitError CWatchdogDriver_OpenHMD::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_WATCHDOG_DRIVER_CONTEXT( pDriverContext );
	InitDriverLog( vr::VRDriverLog() );

	// Watchdog mode on Windows starts a thread that listens for the 'Y' key on the keyboard to 
	// be pressed. A real driver should wait for a system button event or something else from the 
	// the hardware that signals that the VR system should start up.
	g_bExiting = false;
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


class COpenHMDDeviceDriverController : public vr::ITrackedDeviceServerDriver, public vr::IVRControllerComponent {
public:

    int index;
    COpenHMDDeviceDriverController(int index) : index(index) {

    }
    virtual EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId )
    {
        m_unObjectId = unObjectId;
        return VRInitError_None;
    }

    virtual void Deactivate()
    {
        m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    }

    virtual void EnterStandby()
    {
    }

    void *GetComponent( const char *pchComponentNameAndVersion )
    {

        if (!strcmp(pchComponentNameAndVersion, vr::IVRControllerComponent_Version))
        {
            return (vr::IVRControllerComponent*)this;
        }

        return NULL;
    }

    /** debug request from a client */
    virtual void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize )
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

        //TODO: Wait until openhmd controller api https://github.com/OpenHMD/OpenHMD/pull/93 gets merged
        /*
        ohmd_ctx_update(ctx);

        //TODO: why inverted?
        float quat[4];
        ohmd_device_getf(hmd, OHMD_ROTATION_QUAT, quat);
        pose.qRotation.x = quat[0];
        pose.qRotation.y = quat[1];
        pose.qRotation.z = quat[2];
        pose.qRotation.w = quat[3];

        float pos[3];
        ohmd_device_getf(hmd, OHMD_POSITION_VECTOR, pos);
        pose.vecPosition[0] = pos[0];
        pose.vecPosition[1] = pos[1];
        pose.vecPosition[2] = pos[2];

        //printf("ohmd rotation quat %f %f %f %f\n", quat[0], quat[1], quat[2], quat[3]);
        */

        pose.qWorldFromDriverRotation = identityquat;
        pose.qDriverFromHeadRotation = identityquat;

        return pose;
    }

    VRControllerState_t controllerstate;
    VRControllerState_t GetControllerState() {
        return controllerstate;

        controllerstate.unPacketNum = controllerstate.unPacketNum + 1;
        /* //TODO: buttons
         *   if (ohmd_button_state) {
         *       state.ulButtonPressed |= vr::ButtonMaskFromId(k_EButton_Button1);
    }
    // other buttons ...
    */

        //TODO: nolo says when a button was pressed a button was also touched. is that so?
        controllerstate.ulButtonTouched |= controllerstate.ulButtonPressed;

        uint64_t ulChangedTouched = controllerstate.ulButtonTouched ^ controllerstate.ulButtonTouched;
        uint64_t ulChangedPressed = controllerstate.ulButtonPressed ^ controllerstate.ulButtonPressed;

        /*
         *   if (controllerstate.rAxis[0].x != openhmd.... || controllerstate.rAxis[0].y != )
         *       controllerstate->TrackedDeviceAxisUpdated(???, 0, NewState.rAxis[0]);
    }

        controllerstate.rAxis[0].x = openhmd...
        controllerstate.rAxis[0].y =
        controllerstate.rAxis[1].x =
        controllerstate.rAxis[1].y =
        */
        return controllerstate;
    }

    bool TriggerHapticPulse( uint32_t unAxisId, uint16_t usPulseDurationMicroseconds ) {
        return false;
    }

    std::string GetSerialNumber() const { return m_sSerialNumber; }

    bool exists() {
        //TODO: return false when there's no controller with the given index
        return false;
    }

private:
    vr::TrackedDeviceIndex_t m_unObjectId;

    std::string m_sSerialNumber = "Controller serial number " + std::to_string(index);
    std::string m_sModelNumber = "Controller model number " + std::to_string(index);
};

class COpenHMDDeviceDriver : public vr::ITrackedDeviceServerDriver, public vr::IVRDisplayComponent
{
public:
    ohmd_context* ctx;
    ohmd_device* hmd;
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

                // Open default device (0)
                hmd = ohmd_list_open_device(ctx, 0);

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

		char buf[1024];
                strcpy(buf, ohmd_list_gets(ctx, 0, OHMD_PRODUCT)); //whatever
                strcat(buf, ": ");
                strcat(buf, ohmd_list_gets(ctx, 0, OHMD_PATH));
		m_sSerialNumber = buf;

                strcpy(buf, "OpenHMD: ");
                strcat(buf, ohmd_list_gets(ctx, 0, OHMD_PRODUCT));
                m_sModelNumber = buf;

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

	virtual ~COpenHMDDeviceDriver()
	{
	}


	virtual EVRInitError Activate( vr::TrackedDeviceIndex_t unObjectId ) 
	{
		m_unObjectId = unObjectId;
		m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer( m_unObjectId );

		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_ModelNumber_String, m_sModelNumber.c_str() );
		vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, Prop_RenderModelName_String, m_sModelNumber.c_str() );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserIpdMeters_Float, m_flIPD );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_UserHeadToEyeDepthMeters_Float, 0.f );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_DisplayFrequency_Float, m_flDisplayFrequency );
		vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_SecondsFromVsyncToPhotons_Float, m_flSecondsFromVsyncToPhotons );

                float sep;
                //ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);
                //DriverLog("sep %f\n", sep);
                //vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_LensCenterRightU_Float, -sep);
                /*
                float left_lens_center[2];
                float right_lens_center[2];
                float sep;
                ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(left_lens_center[1]));
                ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(right_lens_center[1]));
                ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);

                //vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_LensCenterLeftU_Float, left_lens_center[0]);
                vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_LensCenterLeftV_Float, left_lens_center[1]);

                //vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_LensCenterRightU_Float, right_lens_center[0]);
                vr::VRProperties()->SetFloatProperty( m_ulPropertyContainer, Prop_LensCenterRightV_Float, right_lens_center[1]);

                DriverLog("Prop_LensCenterLeftU_Float %f %f %f %f %f %f",
                          vr::VRProperties()->GetFloatProperty(m_ulPropertyContainer, Prop_LensCenterLeftU_Float),
                          vr::VRProperties()->GetFloatProperty(m_ulPropertyContainer, Prop_LensCenterLeftV_Float),
                          vr::VRProperties()->GetFloatProperty(m_ulPropertyContainer, Prop_LensCenterRightU_Float),
                          vr::VRProperties()->GetFloatProperty(m_ulPropertyContainer, Prop_LensCenterRightV_Float),
                          left_lens_center[0],
                          left_lens_center[1]
                         );
                */

		// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
		vr::VRProperties()->SetUint64Property( m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2 );

		// avoid "not fullscreen" warnings from vrmonitor
		//vr::VRProperties()->SetBoolProperty( m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false );

		// Icons can be configured in code or automatically configured by an external file "drivername\resources\driver.vrresources".
		// Icon properties NOT configured in code (post Activate) are then auto-configured by the optional presence of a driver's "drivername\resources\driver.vrresources".
		// In this manner a driver can configure their icons in a flexible data driven fashion by using an external file.
		//
		// The structure of the driver.vrresources file allows a driver to specialize their icons based on their HW.
		// Keys matching the value in "Prop_ModelNumber_String" are considered first, since the driver may have model specific icons.
		// An absence of a matching "Prop_ModelNumber_String" then considers the ETrackedDeviceClass ("HMD", "Controller", "GenericTracker", "TrackingReference")
		// since the driver may have specialized icons based on those device class names.
		//
		// An absence of either then falls back to the "system.vrresources" where generic device class icons are then supplied.
		//
		// Please refer to "bin\drivers\sample\resources\driver.vrresources" which contains this sample configuration.
		//
		// "Alias" is a reserved key and specifies chaining to another json block.
		//
		// In this sample configuration file (overly complex FOR EXAMPLE PURPOSES ONLY)....
		//
		// "Model-v2.0" chains through the alias to "Model-v1.0" which chains through the alias to "Model-v Defaults".
		//
		// Keys NOT found in "Model-v2.0" would then chase through the "Alias" to be resolved in "Model-v1.0" and either resolve their or continue through the alias.
		// Thus "Prop_NamedIconPathDeviceAlertLow_String" in each model's block represent a specialization specific for that "model".
		// Keys in "Model-v Defaults" are an example of mapping to the same states, and here all map to "Prop_NamedIconPathDeviceOff_String".
		//

                // if we want to set our own icons
                /*
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceOff_String, "{openhmd}/icons/headset_sample_status_off.png" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearching_String, "{openhmd}/icons/headset_sample_status_searching.gif" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{openhmd}/icons/headset_sample_status_searching_alert.gif" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReady_String, "{openhmd}/icons/headset_sample_status_ready.png" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{openhmd}/icons/headset_sample_status_ready_alert.png" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceNotReady_String, "{openhmd}/icons/headset_sample_status_error.png" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceStandby_String, "{openhmd}/icons/headset_sample_status_standby.png" );
                vr::VRProperties()->SetStringProperty( m_ulPropertyContainer, vr::Prop_NamedIconPathDeviceAlertLow_String, "{openhmd}/icons/headset_sample_status_ready_low.png" );
                */
		return VRInitError_None;
	}

	virtual void Deactivate() 
	{
		m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	}

	virtual void EnterStandby()
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

	virtual void PowerOff() 
	{
	}

	virtual void DebugRequest( const char *pchRequest, char *pchResponseBuffer, uint32_t unResponseBufferSize ) 
	{
		if( unResponseBufferSize >= 1 )
			pchResponseBuffer[0] = 0;
	}

	virtual void GetWindowBounds( int32_t *pnX, int32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight ) 
	{
		*pnX = m_nWindowX;
		*pnY = m_nWindowY;
		*pnWidth = m_nWindowWidth;
		*pnHeight = m_nWindowHeight;
	}

	virtual bool IsDisplayOnDesktop() 
	{
		return true;
	}

	virtual bool IsDisplayRealDisplay() 
	{
		return true;
	}

	virtual void GetRecommendedRenderTargetSize( uint32_t *pnWidth, uint32_t *pnHeight ) 
	{
		*pnWidth = m_nRenderWidth;
		*pnHeight = m_nRenderHeight;
	}

	virtual void GetEyeOutputViewport( EVREye eEye, uint32_t *pnX, uint32_t *pnY, uint32_t *pnWidth, uint32_t *pnHeight ) 
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

	virtual void GetProjectionRaw( EVREye eEye, float *pfLeft, float *pfRight, float *pfTop, float *pfBottom ) 
	{
            float sep;
            ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep); //mabye use this
            if (eEye == Eye_Left) {
                *pfLeft = -1.0;
                *pfRight = 1.0;
                *pfTop = -1.0;
                *pfBottom = 1.0;
            } else {
                *pfLeft = -1.0;
                *pfRight = 1.0;
                *pfTop = -1.0;
                *pfBottom = 1.0;
            }
	}

	virtual DistortionCoordinates_t ComputeDistortion( EVREye eEye, float fU, float fV ) 
	{

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

                ohmd_ctx_update(ctx);

                float quat[4];
                ohmd_device_getf(hmd, OHMD_ROTATION_QUAT, quat);
                pose.qRotation.x = quat[0];
                pose.qRotation.y = quat[1];
                pose.qRotation.z = quat[2];
                pose.qRotation.w = quat[3];

                float pos[3];
                ohmd_device_getf(hmd, OHMD_POSITION_VECTOR, pos);
                pose.vecPosition[0] = pos[0];
                pose.vecPosition[1] = pos[1];
                pose.vecPosition[2] = pos[2];

                //printf("ohmd rotation quat %f %f %f %f\n", quat[0], quat[1], quat[2], quat[3]);

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
		}
	}

	std::string GetSerialNumber() const { return m_sSerialNumber; }

private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

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
        COpenHMDDeviceDriverController *m_OpenHMDDeviceDriverControllerL;
        COpenHMDDeviceDriverController *m_OpenHMDDeviceDriverControllerR;
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

	if( pReturnCode )
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}
