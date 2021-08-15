//
// Created by olivier on 14/08/2021.
//

#include "File.h"

File::File( openLife::system::Path *inPath, const char *inName, int inNameLength  )
: mPath( inPath ), mNameLength( inNameLength ) {

	if( inNameLength == -1 ) {
		inNameLength = strlen( inName );
		mNameLength = inNameLength;
	}

	// copy name internally
	mName = stringDuplicate( inName );

}



File::~File() {
	delete [] mName;

	if( mPath != NULL ) {
		delete mPath;
	}
}



long File::getLength() {
	struct stat fileInfo;

	// get full file name
	int length;
	char *stringName = getFullFileName( &length );

	int statError = stat( stringName, &fileInfo );

	delete [] stringName;

	if( statError == 0 ) {
		return fileInfo.st_size;
	}
	else {
		// file does not exist
		return 0;
	}
}



char File::isDirectory() {
	struct stat fileInfo;

	// get full file name
	int length;
	char *stringName = getFullFileName( &length );

	int statError = stat( stringName, &fileInfo );

	delete [] stringName;

	if( statError == -1 ) {
		return false;
	}
	else {
		return S_ISDIR( fileInfo.st_mode );
	}
}



File **File::getChildFiles( int *outNumFiles ) {

	int length;
	char *stringName = getFullFileName( &length );

	DIR *directory = opendir( stringName );

	if( directory != NULL ) {

		SimpleVector< File* > *fileVector = new SimpleVector< File* >();

		struct dirent *entry = readdir( directory );

		if( entry == NULL ) {
			delete fileVector;

			closedir( directory );

			delete [] stringName;

			*outNumFiles = 0;
			return NULL;
		}


		while( entry != NULL ) {
			// skip parentdir and thisdir files, if they occur
			if( strcmp( entry->d_name, "." ) &&
			strcmp( entry->d_name, ".." ) ) {

				openLife::system::Path *newPath;

				if( mPath != NULL ) {
					newPath = mPath->append( mName );
				}
				else {

					if( openLife::system::Path::isRoot( mName ) ) {
						// use name as a string path
						newPath = new openLife::system::Path( mName );
					}
					else {
						char **folderPathArray = new char*[1];
						folderPathArray[0] = mName;

						// a non-absolute path to this directory's contents
						int numSteps = 1;
						char absolute = false;
						newPath =
								new openLife::system::Path( folderPathArray, numSteps,
															absolute );

						delete [] folderPathArray;
					}
				}

				// safe to pass d_name in directly because it is copied
				// internally by the constructor

				fileVector->push_back(
						new File( newPath,
								  entry->d_name,
								  strlen( entry->d_name ) ) );
			}

			entry = readdir( directory );
		}

		// now we have a vector full of this directory's files
		int vectorSize = fileVector->size();

		*outNumFiles = vectorSize;

		if( vectorSize == 0 ) {
			delete fileVector;

			closedir( directory );

			delete [] stringName;

			return NULL;
		}
		else {
			File **returnFiles = new File *[vectorSize];
			for( int i=0; i<vectorSize; i++ ) {
				returnFiles[i] = *( fileVector->getElement( i ) );
			}

			delete fileVector;

			closedir( directory );

			delete [] stringName;

			return returnFiles;
		}
	}
	else {
		delete [] stringName;

		*outNumFiles = 0;
		return NULL;
	}



}




static int fileNameCompare(const void *inA, const void * inB ) {

	File *a = *( (File**)inA );
	File *b = *( (File**)inB );

	char *nameA = a->getFileName();
	char *nameB = b->getFileName();

	int result = strcmp( nameA, nameB );

	delete [] nameA;
	delete [] nameB;

	return result;
}




File **File::getChildFilesSorted( int *outNumFiles ) {
	File **result = getChildFiles( outNumFiles );


	qsort( result, *outNumFiles, sizeof(File*), fileNameCompare );

	return result;
}





File **File::getChildFilesRecursive( int inDepthLimit,
									 int *outNumFiles ) {

	// create a vector for results
	SimpleVector<File *> *resultVector = new SimpleVector<File *>();

	// call the recursive function
	getChildFilesRecursive( inDepthLimit, resultVector );


	// extract results from vector
	File **resultArray = NULL;

	int numResults = resultVector->size();

	if( numResults > 0 ) {
		resultArray = resultVector->getElementArray();
	}

	delete resultVector;



	*outNumFiles = numResults;
	return resultArray;
}



void File::getChildFilesRecursive(
		int inDepthLimit,
		SimpleVector<File *> *inResultVector ) {

	// get our child files
	int numChildren;
	File **childFiles = getChildFiles( &numChildren );

	if( childFiles != NULL ) {

		// for each child, add it to vector and
		// recurse into it if it is a directory

		for( int i=0; i<numChildren; i++ ) {

			File *child = childFiles[i];

			// add it to results vector
			inResultVector->push_back( child );

			if( child->isDirectory() ) {
				// skip recursion if we have hit our depth limit
				if( inDepthLimit > 0 ) {
					// recurse into this subdirectory
					child->getChildFilesRecursive( inDepthLimit - 1,
												   inResultVector );
				}
			}
		}

		delete [] childFiles;
	}
}



File *File::getChildFile( const char *inChildFileName ) {
	// make sure we are a directory
	if( !isDirectory() ) {
		return NULL;
	}

	// get a path to this directory
	openLife::system::Path *newPath;

	if( mPath != NULL ) {
		newPath = mPath->append( mName );
	}
	else {

		char **folderPathArray = new char*[1];
		folderPathArray[0] = mName;

		// a non-absolute path to this directory's contents
		int numSteps = 1;
		char absolute = false;
		newPath =
				new openLife::system::Path( folderPathArray, numSteps,
											absolute );

		delete [] folderPathArray;
	}

	return new File( newPath, inChildFileName );
}



File *File::getParentDirectory() {

	if( mPath != NULL ) {

		char *parentName;

		openLife::system::Path *parentPath;

		if( strcmp( mName, ".." ) == 0 ) {
			// already a parent dir reference
			// append one more parent dir reference with parentName below
			parentPath = mPath->append( ".." );

			parentName = stringDuplicate( ".." );
		}
		else {
			// not a parent dir reference, so we can truncate
			parentPath = mPath->truncate();

			parentName = mPath->getLastStep();
		}

		File *parentFile = new File( parentPath, parentName );

		delete [] parentName;

		return parentFile;
	}
	else {
		if( openLife::system::Path::isRoot( mName ) ) {
			// we are already at the root
			return new File( NULL, mName );
		}
		else {
			// append parent dir symbol to path
			char **parentPathSteps = new char*[1];
			parentPathSteps[0] = mName;

			openLife::system::Path *parentPath = new openLife::system::Path( parentPathSteps, 1, false );

			const char *parentName = "..";

			File *parentFile = new File( parentPath, parentName );

			delete [] parentPathSteps;

			return parentFile;
		}
	}
}


/**
 *
 * @return bool
 * @note test for the existence of a file
 */
char File::exists() {
	struct stat fileInfo;

	// get full file name
	int length;
	char *stringName = getFullFileName( &length );

	int statError = stat( stringName, &fileInfo );

	delete [] stringName;

	if( statError == 0 ) {
		return true;
	}
	else {
		// file does not exist
		return false;
	}
}



timeSec_t File::getModificationTime() {
	struct stat fileInfo;

	// get full file name
	int length;
	char *stringName = getFullFileName( &length );

	int statError = stat( stringName, &fileInfo );

	delete [] stringName;

	if( statError == 0 ) {
		return Time::normalize( fileInfo.st_mtime );
	}
	else {
		// file does not exist
		return 0;
	}
}



char File::remove() {
	char returnVal = false;

	if( exists() ) {
		char *stringName = getFullFileName();

		int error = ::remove( stringName );

		if( error == 0 ) {
			returnVal = true;
		}

		delete [] stringName;
	}

	return returnVal;
}



File *File::copy() {
	openLife::system::Path *pathCopy = NULL;

	if( mPath != NULL ) {
		pathCopy = mPath->copy();
	}

	return new File( pathCopy, mName );
}



void File::copy( File *inDestination, long inBlockSize ) {
	char *thisFileName = getFullFileName();
	char *destinationFileName = inDestination->getFullFileName();

	FILE *thisFile = fopen( thisFileName, "rb" );
	FILE *destinationFile = fopen( destinationFileName, "wb" );

	long length = getLength();

	long bytesCopied = 0;

	char *buffer = new char[ inBlockSize ];

	while( bytesCopied < length ) {

		long bytesToCopy = inBlockSize;

		// end of file case
		if( length - bytesCopied < bytesToCopy ) {
			bytesToCopy = length - bytesCopied;
		}

		fread( buffer, 1, bytesToCopy, thisFile );
		fwrite( buffer, 1, bytesToCopy, destinationFile );

		bytesCopied += bytesToCopy;
	}

	fclose( thisFile );
	fclose( destinationFile );

	delete [] buffer;
	delete [] thisFileName;
	delete [] destinationFileName;
}



char File::contentsMatches( File *inOtherFile ) {
	if( !exists() || ! inOtherFile->exists() ) {
		return false;
	}

	if( isDirectory() || inOtherFile->isDirectory() ) {
		return false;
	}

	if( getLength() != inOtherFile->getLength() ) {
		return false;
	}

	int lenA;
	unsigned char *contA = readFileContents( &lenA );

	int lenB;
	unsigned char *contB = inOtherFile->readFileContents( &lenB );


	char match = false;

	if( contA != NULL && contB != NULL && lenA == lenB ) {

		match = true;

		for( int i=0; i<lenA; i++ ) {
			if( contA[i] != contB[i] ) {
				match = false;
				break;
			}
		}
	}

	if( contA != NULL ) {
		delete [] contA;
	}
	if( contB != NULL ) {
		delete [] contB;
	}


	return match;
}




char *File::getFileName( int *outLength ) {
	char *returnName = stringDuplicate( mName );

	if( outLength != NULL ) {
		*outLength = mNameLength;
	}

	return returnName;
}



char *File::getFullFileName( int *outLength ) {
	int length = mNameLength;

	int pathLength = 0;
	char *path = NULL;
	if( mPath != NULL ) {
		path = mPath->getPathString( &pathLength );

		length += pathLength;
	}

	// extra character for '\0' termination
	char *returnString = new char[ length + 1 ];

	if( path != NULL ) {
		memcpy( returnString, path, pathLength );
		memcpy( &( returnString[pathLength] ), mName, mNameLength );

		delete [] path;
	}
	else {
		// no path, so copy the name directly in
		memcpy( returnString, mName, mNameLength );
	}

	// terminate the string
	returnString[ length ] = '\0';


	if( outLength != NULL ) {
		*outLength = length;
	}

	return returnString;
}



#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/io/file/FileOutputStream.h"



char *File::readFileContents() {

	int length;
	// text mode!
	unsigned char *data = readFileContents( &length, true );

	if( data == NULL ) {
		return NULL;
	}

	char *dataString = new char[ length + 1 ];

	memcpy( dataString, data, length );
	dataString[ length ] = '\0';

	delete [] data;
	return dataString;
}



int File::readFileIntContents( int inDefaultValue ) {
	char *cont = readFileContents();

	if( cont == NULL ) {
		return inDefaultValue;
	}

	int val;

	int numRead = sscanf( cont, "%d", &val );

	delete [] cont;

	if( numRead != 1 ) {
		return inDefaultValue;
	}

	return val;
}




unsigned char *File::readFileContents( int *outLength,
									   char inTextMode ) {

	if( exists() ) {
		int length = getLength();

		unsigned char *returnData = new unsigned char[ length ];

		if( returnData != NULL ) {
			FileInputStream *input = new FileInputStream( this, inTextMode );
			int numRead = input->read( returnData, length );

			delete input;

			// in text mode, read length might not equal binary file length,
			// due to line end conversion
			if( numRead == length ||
			( inTextMode && numRead >= 0 ) ) {
				*outLength = numRead;
				return returnData;
			}
			else {
				delete [] returnData;
				return NULL;
			}
		}
		else {
			// failed to allocate this much memory
			return NULL;
		}
	}
	else {
		return NULL;
	}

}



char File::writeToFile( const char *inString ) {
	return writeToFile( (unsigned char *)inString, strlen( inString ) );
}



char File::writeToFile( int inInt ) {
	char *stringVal = autoSprintf( "%d", inInt );

	char returnVal = writeToFile( stringVal );

	delete [] stringVal;

	return returnVal;
}



char File::writeToFile( unsigned char *inData, int inLength ) {
	FileOutputStream *output = new FileOutputStream( this );

	long numWritten = output->write( inData, inLength );

	delete output;

	if( inLength == numWritten ) {
		return true;
	}
	else {
		return false;
	}

}



#include "Directory.h"



char File::makeDirectory() {
	if( exists() ) {
		return false;
	}
	else {
		return Directory::makeDirectory( this );
	}
}