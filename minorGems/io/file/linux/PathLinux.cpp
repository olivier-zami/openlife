/*
 * Modification History
 *
 * 2001-February-12		Jason Rohrer
 * Created. 
 *
 * 2001-August-1   Jason Rohrer
 * Added missing length return value.
 *
 * 2003-June-2   Jason Rohrer
 * Added support for new path checking functions.
 *
 * 2010-May-18    Jason Rohrer
 * String parameters as const to fix warnings.
 */

#if defined __linux__
	#include "minorGems/io/file/Path.h"
	#define Linux system
#else
#endif

#include "minorGems/util/stringUtils.h"



/*
 * Linux-specific path implementation.  May be compatible
 * with other posix-complient systems.
 */ 



char openLife::Linux::Path::getDelimeter() {
	return '/';
	}
	
		
		
char *openLife::Linux::Path::getAbsoluteRoot( int *outLength ) {
	char *returnString = new char[1];
	returnString[0] = '/';

    *outLength = 1;
    
	return returnString;
	}



char openLife::Linux::Path::isAbsolute( const char *inPathString ) {
    if( inPathString[0] == '/' ) {
        return true;
        }
    else {
        return false;
        }
    }



char *openLife::Linux::Path::extractRoot( const char *inPathString ) {
    if( isAbsolute( inPathString )  ){
        return stringDuplicate( "/" );
        }
    else {
        return NULL;
        }
    }



char openLife::Linux::Path::isRoot( const char *inPathString ) {
    if( strcmp( inPathString, "/" ) == 0 ) {
        return true;
        }
    else {
        return false;
        }
    }


