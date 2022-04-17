// Copyright (c) 2019 Andrew Demuskov
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QBIT_CUBIX_H
#define QBIT_CUBIX_H

#include "../../amount.h"
#include "../../entity.h"
#include "../../txassettype.h"
#include "../../vm/vm.h"

#include <boost/function.hpp>


/*
JPEG:	To use JPEG files, include the file gil/extension/io/jpeg_io.hpp. 
		If you are using run-time images, you need to include gil/extension/io/jpeg_dynamic_io.hpp instead. 
		You need to compile and link against libjpeg.lib 
		(available at http://www.ijg.org). You need to have jpeglib.h in your include path.

TIFF:	To use TIFF files, include the file gil/extension/io/tiff_io.hpp. 
		If you are using run-time images, you need to include gil/extension/io/tiff_dynamic_io.hpp instead. 
		You need to compile and link against libtiff.lib 
		(available at http://www.libtiff.org). You need to have tiffio.h in your include path.

PNG:	To use PNG files, include the file gil/extension/io/png_io.hpp. 
		If you are using run-time images, you need to include gil/extension/io/png_dynamic_io.hpp instead. 
		You need to compile and link against libpng.lib 
		(available at http://wwwlibpng.org). You need to have png.h in your include path.
*/

namespace qbit {
namespace cubix {

#define TX_CUBIX_MEDIA_NAME_SIZE 256 
#define TX_CUBIX_MEDIA_DESCRIPTION_SIZE 512 

typedef LimitedString<TX_CUBIX_MEDIA_NAME_SIZE> cubix_media_name_t;
typedef LimitedString<TX_CUBIX_MEDIA_DESCRIPTION_SIZE> cubix_media_description_t;

#define CUBIX_MIN_FEE 1000
#define CUBIX_FEE_RATE 20

#if defined (MOBILE_PLATFORM)
    #define CUBIX_MAX_DATA_CHUNK 1024*200
#else
    #define CUBIX_MAX_DATA_CHUNK 1024*200
#endif

#define CUBIX_MAX_DATA_SIZE 50*1024*1024

#define CUBIX_PREVIEW_WIDTH 540

#define TX_CUBIX_MEDIA_HEADER	Transaction::CUSTOM_50 //
#define TX_CUBIX_MEDIA_SUMMARY	Transaction::CUSTOM_51 //
#define TX_CUBIX_MEDIA_DATA		Transaction::CUSTOM_52 //

struct doneDownloadWithErrorFunctionExtraArgs {
	unsigned short type;
	unsigned short previewType;

	doneDownloadWithErrorFunctionExtraArgs(unsigned short _type, unsigned short _previewType) : type(_type), previewType(_previewType) {}
};

typedef boost::function<void (TransactionPtr, const std::string& /*previewFile*/, const std::string& /*originalFile*/,
                              unsigned short /*orientation*/, unsigned int /*duration*/,
                              uint64_t /*size*/, const doneDownloadWithErrorFunctionExtraArgs& /*extra*/,
                              const ProcessingError& /*error*/)> doneDownloadWithErrorFunction;

} // cubix
} // qbit

#endif
