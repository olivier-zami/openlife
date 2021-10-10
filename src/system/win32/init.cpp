//
// Created by olivier on 09/10/2021.
//

#include "src/system/_base/_macro/init_system.h"

#if defined(SYSTEM)&&(SYSTEM==win32)

#include "../_base/init.h"
#include "../_base/log.h"

#define osWin32 system

void openLife::osWin32::init()
{
	openLife::system::Log::notice("Starting Windows version of openLife ...");

	HMODULE hShcore = LoadLibrary( _T( "shcore.dll" ) );

    if( hShcore != NULL ) {
        printf( "shcore.dll loaded successfully\n" );

        typedef enum _PROCESS_DPI_AWARENESS {
            PROCESS_DPI_UNAWARE            = 0,
            PROCESS_SYSTEM_DPI_AWARE       = 1,
            PROCESS_PER_MONITOR_DPI_AWARE  = 2
            } PROCESS_DPI_AWARENESS;

        typedef HRESULT (*SetProcessDpiAwarenessFunc)( PROCESS_DPI_AWARENESS );

        SetProcessDpiAwarenessFunc setAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(
                hShcore,
                "SetProcessDpiAwareness" );

        if( setAwareness ) {
            printf( "Found SetProcessDpiAwareness function in shcore.dll\n" );
            setAwareness( PROCESS_PER_MONITOR_DPI_AWARE );
            }
        else {
            printf( "Could NOT find SetProcessDpiAwareness function in "
                    "Shcore.dll\n" );
            }
        FreeLibrary( hShcore );
        }
    else {
        printf( "shcore.dll NOT loaded successfully... pre Win 8.1 ?\n" );


        printf( "   Trying to load user32.dll instead\n" );


        // backwards compatible code
        // on vista and higher, this will tell Windows that we are DPI aware
        // and that we should not be artificially scaled.
        //
        // Found here:  http://www.rw-designer.com/DPI-aware
        HMODULE hUser32 = LoadLibrary( _T( "user32.dll" ) );

        if( hUser32 != NULL ) {

            typedef BOOL (*SetProcessDPIAwareFunc)();

            SetProcessDPIAwareFunc setDPIAware =
                (SetProcessDPIAwareFunc)GetProcAddress( hUser32,
                                                        "SetProcessDPIAware" );
            if( setDPIAware ) {
                printf( "Found SetProcessDPIAware function in user32.dll\n" );
                setDPIAware();
                }
            else {
                printf( "Could NOT find SetProcessDPIAware function in "
                        "user32.dll\n" );
                }
            FreeLibrary( hUser32 );
            }
        else {
            printf( "Failed to load user32.dll\n" );
            }
        }
}

#undef osWin32

#endif