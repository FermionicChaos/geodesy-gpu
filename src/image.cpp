#include <geodesy/gpu/image.h>

// Standard C Libs
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>
#include <float.h>

#include <assert.h>

#include <vector>
#include <algorithm>

#include "glslang_util.h"

#include <geodesy/gpu/pipeline.h>
#include <geodesy/gpu/context.h>

// #define STB_IMAGE_IMPLEMENTATION
// #include <stb_image.h>

//// Image Loading
//#define FREEIMAGE_LIB
//#include <FreeImage.h>

// So gross.
// Group these later based on spec.
// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#texel-block-size

/*
Size table;

8
16
24
32
48
64
96
128
192
256
*/

namespace geodesy::gpu {

	/*
	static FREE_IMAGE_FORMAT extid_to_fifid(io::file::extension ExtensionID) {
		switch (ExtensionID) {
		default:								return FIF_UNKNOWN;
		case io::file::extension::IMAGE_BMP:	return FIF_BMP;
		case io::file::extension::IMAGE_ICO:	return FIF_ICO;
		case io::file::extension::IMAGE_JPEG:	return FIF_JPEG;
		case io::file::extension::IMAGE_JNG:	return FIF_JNG;
		case io::file::extension::IMAGE_KOALA:	return FIF_KOALA;
		case io::file::extension::IMAGE_LBM:	return FIF_LBM;
		case io::file::extension::IMAGE_IFF:	return FIF_IFF;
		case io::file::extension::IMAGE_MNG:	return FIF_MNG;
		case io::file::extension::IMAGE_PBM:	return FIF_PBM;
		case io::file::extension::IMAGE_PBMRAW: return FIF_PBMRAW;
		case io::file::extension::IMAGE_PCD:	return FIF_PCD;
		case io::file::extension::IMAGE_PCX:	return FIF_PCX;
		case io::file::extension::IMAGE_PGM:	return FIF_PGM;
		case io::file::extension::IMAGE_PGMRAW: return FIF_PGMRAW;
		case io::file::extension::IMAGE_PNG:	return FIF_PNG;
		case io::file::extension::IMAGE_PPM:	return FIF_PPM;
		case io::file::extension::IMAGE_PPMRAW: return FIF_PPMRAW;
		case io::file::extension::IMAGE_RAS:	return FIF_RAS;
		case io::file::extension::IMAGE_TARGA:	return FIF_TARGA;
		case io::file::extension::IMAGE_TIFF:	return FIF_TIFF;
		case io::file::extension::IMAGE_WBMP:	return FIF_WBMP;
		case io::file::extension::IMAGE_PSD:	return FIF_PSD;
		case io::file::extension::IMAGE_CUT:	return FIF_CUT;
		case io::file::extension::IMAGE_XBM:	return FIF_XBM;
		case io::file::extension::IMAGE_XPM:	return FIF_XPM;
		case io::file::extension::IMAGE_DDS:	return FIF_DDS;
		case io::file::extension::IMAGE_GIF:	return FIF_GIF;
		case io::file::extension::IMAGE_HDR:	return FIF_HDR;
		case io::file::extension::IMAGE_FAXG3:	return FIF_FAXG3;
		case io::file::extension::IMAGE_SGI:	return FIF_SGI;
		case io::file::extension::IMAGE_EXR:	return FIF_EXR;
		case io::file::extension::IMAGE_J2K:	return FIF_J2K;
		case io::file::extension::IMAGE_JP2:	return FIF_JP2;
		case io::file::extension::IMAGE_PFM:	return FIF_PFM;
		case io::file::extension::IMAGE_PICT:	return FIF_PICT;
		case io::file::extension::IMAGE_RAW:	return FIF_RAW;
		case io::file::extension::IMAGE_WEBP:	return FIF_WEBP;
		case io::file::extension::IMAGE_JXR:	return FIF_JXR;
		}
	}
	*/

	image::create_info::create_info() {
		this->Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		this->Sample = image::sample::COUNT_1;
		this->Tiling = image::tiling::OPTIMAL;
		this->Memory = 0;
		this->Usage = image::usage::TRANSFER_DST | image::usage::TRANSFER_SRC;
		this->MipLevels = false;
	}

	image::create_info::create_info(int aSample, int aTiling, int aMemory, int aUsage) : create_info() {
		this->Sample = aSample;
		this->Tiling = aTiling;
		this->Memory = aMemory;
		this->Usage = aUsage;
	}

	size_t image::bytes_per_pixel(int aFormat) {
		return (image::bits_per_pixel(aFormat) / 8);
	}

	size_t image::bits_per_pixel(int aFormat) {
		switch (aFormat) {
		default: return 0;
		case VK_FORMAT_R4G4_UNORM_PACK8: return 8;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return 16;
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return 16;
		case VK_FORMAT_R5G6B5_UNORM_PACK16: return 16;
		case VK_FORMAT_B5G6R5_UNORM_PACK16: return 16;
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return 16;
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return 16;
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return 16;
		case VK_FORMAT_R8_UNORM: return 8;
		case VK_FORMAT_R8_SNORM: return 8;
		case VK_FORMAT_R8_USCALED: return 8;
		case VK_FORMAT_R8_SSCALED: return 8;
		case VK_FORMAT_R8_UINT: return 8;
		case VK_FORMAT_R8_SINT: return 8;
		case VK_FORMAT_R8_SRGB: return 8;
		case VK_FORMAT_R8G8_UNORM: return 16;
		case VK_FORMAT_R8G8_SNORM: return 16;
		case VK_FORMAT_R8G8_USCALED: return 16;
		case VK_FORMAT_R8G8_SSCALED: return 16;
		case VK_FORMAT_R8G8_UINT: return 16;
		case VK_FORMAT_R8G8_SINT: return 16;
		case VK_FORMAT_R8G8_SRGB: return 16;
		case VK_FORMAT_R8G8B8_UNORM: return 24;
		case VK_FORMAT_R8G8B8_SNORM: return 24;
		case VK_FORMAT_R8G8B8_USCALED: return 24;
		case VK_FORMAT_R8G8B8_SSCALED: return 24;
		case VK_FORMAT_R8G8B8_UINT: return 24;
		case VK_FORMAT_R8G8B8_SINT: return 24;
		case VK_FORMAT_R8G8B8_SRGB: return 24;
		case VK_FORMAT_B8G8R8_UNORM: return 24;
		case VK_FORMAT_B8G8R8_SNORM: return 24;
		case VK_FORMAT_B8G8R8_USCALED: return 24;
		case VK_FORMAT_B8G8R8_SSCALED: return 24;
		case VK_FORMAT_B8G8R8_UINT: return 24;
		case VK_FORMAT_B8G8R8_SINT: return 24;
		case VK_FORMAT_B8G8R8_SRGB: return 24;
		case VK_FORMAT_R8G8B8A8_UNORM: return 32;
		case VK_FORMAT_R8G8B8A8_SNORM: return 32;
		case VK_FORMAT_R8G8B8A8_USCALED: return 32;
		case VK_FORMAT_R8G8B8A8_SSCALED: return 32;
		case VK_FORMAT_R8G8B8A8_UINT: return 32;
		case VK_FORMAT_R8G8B8A8_SINT: return 32;
		case VK_FORMAT_R8G8B8A8_SRGB: return 32;
		case VK_FORMAT_B8G8R8A8_UNORM: return 32;
		case VK_FORMAT_B8G8R8A8_SNORM: return 32;
		case VK_FORMAT_B8G8R8A8_USCALED: return 32;
		case VK_FORMAT_B8G8R8A8_SSCALED: return 32;
		case VK_FORMAT_B8G8R8A8_UINT: return 32;
		case VK_FORMAT_B8G8R8A8_SINT: return 32;
		case VK_FORMAT_B8G8R8A8_SRGB: return 32;
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_UINT_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_SINT_PACK32: return 32;
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32: return 32;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_UINT_PACK32: return 32;
		case VK_FORMAT_A2B10G10R10_SINT_PACK32: return 32;
		case VK_FORMAT_R16_UNORM: return 16;
		case VK_FORMAT_R16_SNORM: return 16;
		case VK_FORMAT_R16_USCALED: return 16;
		case VK_FORMAT_R16_SSCALED: return 16;
		case VK_FORMAT_R16_UINT: return 16;
		case VK_FORMAT_R16_SINT: return 16;
		case VK_FORMAT_R16_SFLOAT: return 16;
		case VK_FORMAT_R16G16_UNORM: return 16;
		case VK_FORMAT_R16G16_SNORM: return 16;
		case VK_FORMAT_R16G16_USCALED: return 16;
		case VK_FORMAT_R16G16_SSCALED: return 16;
		case VK_FORMAT_R16G16_UINT: return 16;
		case VK_FORMAT_R16G16_SINT: return 16;
		case VK_FORMAT_R16G16_SFLOAT: return 16;
		case VK_FORMAT_R16G16B16_UNORM: return 48;
		case VK_FORMAT_R16G16B16_SNORM: return 48;
		case VK_FORMAT_R16G16B16_USCALED: return 48;
		case VK_FORMAT_R16G16B16_SSCALED: return 48;
		case VK_FORMAT_R16G16B16_UINT: return 48;
		case VK_FORMAT_R16G16B16_SINT: return 48;
		case VK_FORMAT_R16G16B16_SFLOAT: return 48;
		case VK_FORMAT_R16G16B16A16_UNORM: return 64;
		case VK_FORMAT_R16G16B16A16_SNORM: return 64;
		case VK_FORMAT_R16G16B16A16_USCALED: return 64;
		case VK_FORMAT_R16G16B16A16_SSCALED: return 64;
		case VK_FORMAT_R16G16B16A16_UINT: return 64;
		case VK_FORMAT_R16G16B16A16_SINT: return 64;
		case VK_FORMAT_R16G16B16A16_SFLOAT: return 64;
		case VK_FORMAT_R32_UINT: return 32;
		case VK_FORMAT_R32_SINT: return 32;
		case VK_FORMAT_R32_SFLOAT: return 32;
		case VK_FORMAT_R32G32_UINT: return 64;
		case VK_FORMAT_R32G32_SINT: return 64;
		case VK_FORMAT_R32G32_SFLOAT: return 64;
		case VK_FORMAT_R32G32B32_UINT: return 96;
		case VK_FORMAT_R32G32B32_SINT: return 96;
		case VK_FORMAT_R32G32B32_SFLOAT: return 96;
		case VK_FORMAT_R32G32B32A32_UINT: return 128;
		case VK_FORMAT_R32G32B32A32_SINT: return 128;
		case VK_FORMAT_R32G32B32A32_SFLOAT: return 128;
		case VK_FORMAT_R64_UINT: return 64;
		case VK_FORMAT_R64_SINT: return 64;
		case VK_FORMAT_R64_SFLOAT: return 64;
		case VK_FORMAT_R64G64_UINT: return 128;
		case VK_FORMAT_R64G64_SINT: return 128;
		case VK_FORMAT_R64G64_SFLOAT: return 128;
		case VK_FORMAT_R64G64B64_UINT: return 192;
		case VK_FORMAT_R64G64B64_SINT: return 192;
		case VK_FORMAT_R64G64B64_SFLOAT: return 192;
		case VK_FORMAT_R64G64B64A64_UINT: return 256;
		case VK_FORMAT_R64G64B64A64_SINT: return 256;
		case VK_FORMAT_R64G64B64A64_SFLOAT: return 256;
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return 32;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return 32;
		case VK_FORMAT_D16_UNORM: return 16;
		case VK_FORMAT_X8_D24_UNORM_PACK32: return 32;
		case VK_FORMAT_D32_SFLOAT: return 32;
		case VK_FORMAT_S8_UINT: return 8;
		case VK_FORMAT_D16_UNORM_S8_UINT: return 24;
		case VK_FORMAT_D24_UNORM_S8_UINT: return 32;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return 40;
			// I might comment this section out.
			/*
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK                              : return 0;
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK                               : return 0;
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_BC1_RGBA_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_BC2_UNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC2_SRGB_BLOCK                                   : return 0;
		case VK_FORMAT_BC3_UNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC3_SRGB_BLOCK                                   : return 0;
		case VK_FORMAT_BC4_UNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC4_SNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC5_UNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC5_SNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK                                : return 0;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK                                : return 0;
		case VK_FORMAT_BC7_UNORM_BLOCK                                  : return 0;
		case VK_FORMAT_BC7_SRGB_BLOCK                                   : return 0;
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK                          : return 0;
		case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK                           : return 0;
		case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK                        : return 0;
		case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK                         : return 0;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK                        : return 0;
		case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK                         : return 0;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK                              : return 0;
		case VK_FORMAT_EAC_R11_SNORM_BLOCK                              : return 0;
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK                           : return 0;
		case VK_FORMAT_EAC_R11G11_SNORM_BLOCK                           : return 0;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_5x4_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_5x4_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_5x5_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_5x5_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_6x5_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_6x5_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_8x5_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_8x5_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_8x6_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_8x6_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK                              : return 0;
		case VK_FORMAT_ASTC_10x5_UNORM_BLOCK                            : return 0;
		case VK_FORMAT_ASTC_10x5_SRGB_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_10x6_UNORM_BLOCK                            : return 0;
		case VK_FORMAT_ASTC_10x6_SRGB_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_10x8_UNORM_BLOCK                            : return 0;
		case VK_FORMAT_ASTC_10x8_SRGB_BLOCK                             : return 0;
		case VK_FORMAT_ASTC_10x10_UNORM_BLOCK                           : return 0;
		case VK_FORMAT_ASTC_10x10_SRGB_BLOCK                            : return 0;
		case VK_FORMAT_ASTC_12x10_UNORM_BLOCK                           : return 0;
		case VK_FORMAT_ASTC_12x10_SRGB_BLOCK                            : return 0;
		case VK_FORMAT_ASTC_12x12_UNORM_BLOCK                           : return 0;
		case VK_FORMAT_ASTC_12x12_SRGB_BLOCK                            : return 0;
		case VK_FORMAT_G8B8G8R8_422_UNORM                               : return 0;
		case VK_FORMAT_B8G8R8G8_422_UNORM                               : return 0;
		case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM                        : return 0;
		case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM                         : return 0;
		case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM                        : return 0;
		case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM                         : return 0;
		case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM                        : return 0;
		case VK_FORMAT_R10X6_UNORM_PACK16                               : return 0;
		case VK_FORMAT_R10X6G10X6_UNORM_2PACK16                         : return 0;
		case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16               : return 0;
		case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16           : return 0;
		case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16           : return 0;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16       : return 0;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16        : return 0;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16       : return 0;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16        : return 0;
		case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16       : return 0;
		case VK_FORMAT_R12X4_UNORM_PACK16                               : return 0;
		case VK_FORMAT_R12X4G12X4_UNORM_2PACK16                         : return 0;
		case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16               : return 0;
		case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16           : return 0;
		case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16           : return 0;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16       : return 0;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16        : return 0;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16       : return 0;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16        : return 0;
		case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16       : return 0;
		case VK_FORMAT_G16B16G16R16_422_UNORM                           : return 0;
		case VK_FORMAT_B16G16R16G16_422_UNORM                           : return 0;
		case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM                     : return 0;
		case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM                      : return 0;
		case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM                     : return 0;
		case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM                      : return 0;
		case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM                     : return 0;
		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG                      : return 0;
		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG                      : return 0;
		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG                      : return 0;
		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG                      : return 0;
		case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG                       : return 0;
		case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG                       : return 0;
		case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG                       : return 0;
		case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG                       : return 0;
		case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT                        : return 0;
		case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT                       : return 0;
		case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT                       : return 0;
		case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT                       : return 0;
		case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT                      : return 0;
		case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT                      : return 0;
		case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT                      : return 0;
		case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT                     : return 0;
		case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT    : return 0;
		case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT    : return 0;
		case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT                  : return 0;
		case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT                        : return 0;
		case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT                        : return 0;
			*/
		}
	}

	size_t image::channel_count(int aFormat) {
		switch (aFormat) {
		// ===== 1 CHANNEL (R) =====
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16_USCALED:
		case VK_FORMAT_R16_SSCALED:
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R64_UINT:
		case VK_FORMAT_R64_SINT:
		case VK_FORMAT_R64_SFLOAT:
			return 1;
		// ===== 2 CHANNELS (RG) =====
		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8_USCALED:
		case VK_FORMAT_R8G8_SSCALED:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8_SRGB:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16_USCALED:
		case VK_FORMAT_R16G16_SSCALED:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R64G64_UINT:
		case VK_FORMAT_R64G64_SINT:
		case VK_FORMAT_R64G64_SFLOAT:
			return 2;
		// ===== 3 CHANNELS (RGB) =====
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
		case VK_FORMAT_B5G6R5_UNORM_PACK16:
		case VK_FORMAT_R8G8B8_UNORM:
		case VK_FORMAT_R8G8B8_SNORM:
		case VK_FORMAT_R8G8B8_USCALED:
		case VK_FORMAT_R8G8B8_SSCALED:
		case VK_FORMAT_R8G8B8_UINT:
		case VK_FORMAT_R8G8B8_SINT:
		case VK_FORMAT_R8G8B8_SRGB:
		case VK_FORMAT_B8G8R8_UNORM:
		case VK_FORMAT_B8G8R8_SNORM:
		case VK_FORMAT_B8G8R8_USCALED:
		case VK_FORMAT_B8G8R8_SSCALED:
		case VK_FORMAT_B8G8R8_UINT:
		case VK_FORMAT_B8G8R8_SINT:
		case VK_FORMAT_B8G8R8_SRGB:
		case VK_FORMAT_R16G16B16_UNORM:
		case VK_FORMAT_R16G16B16_SNORM:
		case VK_FORMAT_R16G16B16_USCALED:
		case VK_FORMAT_R16G16B16_SSCALED:
		case VK_FORMAT_R16G16B16_UINT:
		case VK_FORMAT_R16G16B16_SINT:
		case VK_FORMAT_R16G16B16_SFLOAT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R64G64B64_UINT:
		case VK_FORMAT_R64G64B64_SINT:
		case VK_FORMAT_R64G64B64_SFLOAT:
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			return 3;
		// ===== 4 CHANNELS (RGBA) =====
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return 4;
		// ===== DEPTH/STENCIL FORMATS (1 or 2 channels) =====
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_S8_UINT:
			return 1;
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return 2;  // Depth + Stencil
		default: return 0;
		}
	}

	VkImageAspectFlags image::aspect_flag(int aFormat) {
		VkImageAspectFlags AspectFlag = 0;
		switch (aFormat) {
		default: 
			AspectFlag = 0;
			break;
		case VK_FORMAT_R4G4_UNORM_PACK8:
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16: 
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16: 
		case VK_FORMAT_R5G6B5_UNORM_PACK16: 
		case VK_FORMAT_B5G6R5_UNORM_PACK16: 
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16: 
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16: 
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16: 
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8_USCALED:
		case VK_FORMAT_R8_SSCALED:
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_SRGB:
		case VK_FORMAT_R8G8_UNORM: 
		case VK_FORMAT_R8G8_SNORM: 
		case VK_FORMAT_R8G8_USCALED: 
		case VK_FORMAT_R8G8_SSCALED: 
		case VK_FORMAT_R8G8_UINT: 
		case VK_FORMAT_R8G8_SINT: 
		case VK_FORMAT_R8G8_SRGB: 
		case VK_FORMAT_R8G8B8_UNORM: 
		case VK_FORMAT_R8G8B8_SNORM: 
		case VK_FORMAT_R8G8B8_USCALED: 
		case VK_FORMAT_R8G8B8_SSCALED: 
		case VK_FORMAT_R8G8B8_UINT: 
		case VK_FORMAT_R8G8B8_SINT: 
		case VK_FORMAT_R8G8B8_SRGB: 
		case VK_FORMAT_B8G8R8_UNORM: 
		case VK_FORMAT_B8G8R8_SNORM: 
		case VK_FORMAT_B8G8R8_USCALED: 
		case VK_FORMAT_B8G8R8_SSCALED: 
		case VK_FORMAT_B8G8R8_UINT: 
		case VK_FORMAT_B8G8R8_SINT: 
		case VK_FORMAT_B8G8R8_SRGB: 
		case VK_FORMAT_R8G8B8A8_UNORM: 
		case VK_FORMAT_R8G8B8A8_SNORM: 
		case VK_FORMAT_R8G8B8A8_USCALED: 
		case VK_FORMAT_R8G8B8A8_SSCALED: 
		case VK_FORMAT_R8G8B8A8_UINT: 
		case VK_FORMAT_R8G8B8A8_SINT: 
		case VK_FORMAT_R8G8B8A8_SRGB: 
		case VK_FORMAT_B8G8R8A8_UNORM: 
		case VK_FORMAT_B8G8R8A8_SNORM: 
		case VK_FORMAT_B8G8R8A8_USCALED: 
		case VK_FORMAT_B8G8R8A8_SSCALED: 
		case VK_FORMAT_B8G8R8A8_UINT: 
		case VK_FORMAT_B8G8R8A8_SINT: 
		case VK_FORMAT_B8G8R8A8_SRGB: 
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32: 
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32: 
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32: 
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: 
		case VK_FORMAT_A8B8G8R8_UINT_PACK32: 
		case VK_FORMAT_A8B8G8R8_SINT_PACK32: 
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32: 
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32: 
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32: 
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32: 
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: 
		case VK_FORMAT_A2R10G10B10_UINT_PACK32: 
		case VK_FORMAT_A2R10G10B10_SINT_PACK32: 
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32: 
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32: 
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32: 
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: 
		case VK_FORMAT_A2B10G10R10_UINT_PACK32: 
		case VK_FORMAT_A2B10G10R10_SINT_PACK32: 
		case VK_FORMAT_R16_UNORM: 
		case VK_FORMAT_R16_SNORM: 
		case VK_FORMAT_R16_USCALED: 
		case VK_FORMAT_R16_SSCALED: 
		case VK_FORMAT_R16_UINT: 
		case VK_FORMAT_R16_SINT: 
		case VK_FORMAT_R16_SFLOAT: 
		case VK_FORMAT_R16G16_UNORM: 
		case VK_FORMAT_R16G16_SNORM: 
		case VK_FORMAT_R16G16_USCALED: 
		case VK_FORMAT_R16G16_SSCALED: 
		case VK_FORMAT_R16G16_UINT: 
		case VK_FORMAT_R16G16_SINT: 
		case VK_FORMAT_R16G16_SFLOAT: 
		case VK_FORMAT_R16G16B16_UNORM: 
		case VK_FORMAT_R16G16B16_SNORM: 
		case VK_FORMAT_R16G16B16_USCALED: 
		case VK_FORMAT_R16G16B16_SSCALED: 
		case VK_FORMAT_R16G16B16_UINT: 
		case VK_FORMAT_R16G16B16_SINT: 
		case VK_FORMAT_R16G16B16_SFLOAT: 
		case VK_FORMAT_R16G16B16A16_UNORM: 
		case VK_FORMAT_R16G16B16A16_SNORM: 
		case VK_FORMAT_R16G16B16A16_USCALED: 
		case VK_FORMAT_R16G16B16A16_SSCALED: 
		case VK_FORMAT_R16G16B16A16_UINT: 
		case VK_FORMAT_R16G16B16A16_SINT: 
		case VK_FORMAT_R16G16B16A16_SFLOAT: 
		case VK_FORMAT_R32_UINT: 
		case VK_FORMAT_R32_SINT: 
		case VK_FORMAT_R32_SFLOAT: 
		case VK_FORMAT_R32G32_UINT: 
		case VK_FORMAT_R32G32_SINT: 
		case VK_FORMAT_R32G32_SFLOAT: 
		case VK_FORMAT_R32G32B32_UINT: 
		case VK_FORMAT_R32G32B32_SINT: 
		case VK_FORMAT_R32G32B32_SFLOAT: 
		case VK_FORMAT_R32G32B32A32_UINT: 
		case VK_FORMAT_R32G32B32A32_SINT: 
		case VK_FORMAT_R32G32B32A32_SFLOAT: 
		case VK_FORMAT_R64_UINT: 
		case VK_FORMAT_R64_SINT: 
		case VK_FORMAT_R64_SFLOAT: 
		case VK_FORMAT_R64G64_UINT: 
		case VK_FORMAT_R64G64_SINT: 
		case VK_FORMAT_R64G64_SFLOAT: 
		case VK_FORMAT_R64G64B64_UINT: 
		case VK_FORMAT_R64G64B64_SINT: 
		case VK_FORMAT_R64G64B64_SFLOAT: 
		case VK_FORMAT_R64G64B64A64_UINT: 
		case VK_FORMAT_R64G64B64A64_SINT: 
		case VK_FORMAT_R64G64B64A64_SFLOAT: 
		case VK_FORMAT_B10G11R11_UFLOAT_PACK32: 
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			AspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
			break;
		case VK_FORMAT_D16_UNORM: 
		case VK_FORMAT_X8_D24_UNORM_PACK32: 
		case VK_FORMAT_D32_SFLOAT: 
			AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		case VK_FORMAT_D16_UNORM_S8_UINT: 
		case VK_FORMAT_D24_UNORM_S8_UINT: 
		case VK_FORMAT_D32_SFLOAT_S8_UINT: 
		case VK_FORMAT_S8_UINT:
			AspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		}
		return AspectFlag;
	}

	VkFormat image::glsl_to_format(const glslang::TObjectReflection& aVariable) {
		const glslang::TType* Type = aVariable.getType();
		glslang::TBasicType basicType = Type->getBasicType();
		int vectorSize = Type->getVectorSize();
		switch (basicType) {
		case glslang::EbtFloat:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R32_SFLOAT;
			case 2: return VK_FORMAT_R32G32_SFLOAT;
			case 3: return VK_FORMAT_R32G32B32_SFLOAT;
			case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
			default: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			
		case glslang::EbtDouble:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R64_SFLOAT;
			case 2: return VK_FORMAT_R64G64_SFLOAT;
			case 3: return VK_FORMAT_R64G64B64_SFLOAT;
			case 4: return VK_FORMAT_R64G64B64A64_SFLOAT;
			default: return VK_FORMAT_R64G64B64A64_SFLOAT;
			}
			
		case glslang::EbtInt:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R32_SINT;
			case 2: return VK_FORMAT_R32G32_SINT;
			case 3: return VK_FORMAT_R32G32B32_SINT;
			case 4: return VK_FORMAT_R32G32B32A32_SINT;
			default: return VK_FORMAT_R32G32B32A32_SINT;
			}
			
		case glslang::EbtUint:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R32_UINT;
			case 2: return VK_FORMAT_R32G32_UINT;
			case 3: return VK_FORMAT_R32G32B32_UINT;
			case 4: return VK_FORMAT_R32G32B32A32_UINT;
			default: return VK_FORMAT_R32G32B32A32_UINT;
			}
			
		case glslang::EbtInt16:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R16_SINT;
			case 2: return VK_FORMAT_R16G16_SINT;
			case 3: return VK_FORMAT_R16G16B16_SINT;
			case 4: return VK_FORMAT_R16G16B16A16_SINT;
			default: return VK_FORMAT_R16G16B16A16_SINT;
			}
			
		case glslang::EbtUint16:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R16_UINT;
			case 2: return VK_FORMAT_R16G16_UINT;
			case 3: return VK_FORMAT_R16G16B16_UINT;
			case 4: return VK_FORMAT_R16G16B16A16_UINT;
			default: return VK_FORMAT_R16G16B16A16_UINT;
			}
			
		case glslang::EbtInt8:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R8_SINT;
			case 2: return VK_FORMAT_R8G8_SINT;
			case 3: return VK_FORMAT_R8G8B8_SINT;
			case 4: return VK_FORMAT_R8G8B8A8_SINT;
			default: return VK_FORMAT_R8G8B8A8_SINT;
			}
			
		case glslang::EbtUint8:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R8_UINT;
			case 2: return VK_FORMAT_R8G8_UINT;
			case 3: return VK_FORMAT_R8G8B8_UINT;
			case 4: return VK_FORMAT_R8G8B8A8_UINT;
			default: return VK_FORMAT_R8G8B8A8_UINT;
			}
			
		case glslang::EbtFloat16:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R16_SFLOAT;
			case 2: return VK_FORMAT_R16G16_SFLOAT;
			case 3: return VK_FORMAT_R16G16B16_SFLOAT;
			case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
			default: return VK_FORMAT_R16G16B16A16_SFLOAT;
			}
			
		case glslang::EbtBool:
			switch (vectorSize) {
			case 1: return VK_FORMAT_R8_UINT;
			case 2: return VK_FORMAT_R8G8_UINT;
			case 3: return VK_FORMAT_R8G8B8_UINT;
			case 4: return VK_FORMAT_R8G8B8A8_UINT;
			default: return VK_FORMAT_R8G8B8A8_UINT;
			}
			
		case glslang::EbtSampler:
			// For samplers, return a common texture format
			// The actual format depends on the sampled image
			switch (Type->getSampler().type) {
			case glslang::EbtFloat:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case glslang::EbtInt:
				return VK_FORMAT_R8G8B8A8_SINT;
			case glslang::EbtUint:
				return VK_FORMAT_R8G8B8A8_UINT;
			default:
				return VK_FORMAT_R8G8B8A8_UNORM;
			}
			
		// Matrix types - typically stored as arrays of vectors
		case glslang::EbtStruct:
			// For struct types, return a generic format
			return VK_FORMAT_R8G8B8A8_UNORM;
			
		default:
			// Unknown or unsupported type
			return VK_FORMAT_UNDEFINED;
		}
	}

	image::image() {
		this->Context								= nullptr;
		this->Type 									= resource::type::IMAGE;
		this->OpaquePercentage 						= 0.0f;
		this->TransparentPercentage 				= 0.0f;
		this->TranslucentPercentage 				= 0.0f;
		this->CreateInfo							= {};
		this->Handle								= VK_NULL_HANDLE;
		this->View 									= VK_NULL_HANDLE;
		this->MemoryType							= 0;
		this->MemoryHandle							= VK_NULL_HANDLE;
		this->CreateInfo.sType						= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		this->CreateInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
		this->CreateInfo.queueFamilyIndexCount		= 0;
	}

	// image::image(std::string aFilePath) : file(aFilePath) {
	// 	this->zero_out();

	// 	// Load the image file.
	// 	void* lData = NULL;
	// 	int lWidth, lHeight, lChannels;
	// 	// Check if the image file is HDR, or standard.
	// 	if (!stbi_is_hdr(aFilePath.c_str())) {
	// 		this->HostData = (void*)stbi_load(aFilePath.c_str(), &lWidth, &lHeight, &lChannels, STBI_rgb_alpha);
	// 		lChannels = 4; // Force 4 channels for standard images.
	// 		this->CreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	// 		this->HostSize = lWidth * lHeight * lChannels * sizeof(char);

	// 		// Analyze Transparency of the image.
	// 		float TotalPixels = (float)(lWidth * lHeight);
	// 		math::vec<uchar, 4> *Pixel = (math::vec<uchar, 4> *)this->HostData;
	// 		for (size_t i = 0; i < lWidth*lHeight; i++) {
	// 			// Inspect each alpha channel pixel.
	// 			if (Pixel[i][3] == 0xFF) {
	// 				this->OpaquePercentage += 1.0f;
	// 			}
	// 			else if (Pixel[i][3] == 0x00) {
	// 				this->TransparentPercentage += 1.0f;
	// 			}
	// 			else {
	// 				this->TranslucentPercentage += 1.0f;
	// 			}
	// 		}

	// 		// Normalize the percentages.
	// 		this->OpaquePercentage /= TotalPixels;
	// 		this->TransparentPercentage /= TotalPixels;
	// 		this->TranslucentPercentage /= TotalPixels;
	// 	}
	// 	else {
	// 		// Load the HDR image file.
	// 		this->HostData = (void*)stbi_loadf(aFilePath.c_str(), &lWidth, &lHeight, &lChannels, STBI_rgb_alpha);
	// 		lChannels = 4; // Force 4 channels for HDR images.
	// 		this->CreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	// 		this->HostSize = lWidth * lHeight * lChannels * sizeof(float);

	// 		// Analyze Transparency of the image.
	// 		float TotalPixels = (float)(lWidth * lHeight);
	// 		math::vec<float, 4> *Pixel = (math::vec<float, 4> *)this->HostData;
	// 		for (size_t i = 0; i < lWidth*lHeight; i++) {
	// 			// Inspect each alpha channel pixel.
	// 			if (Pixel[i][3] >= 0.999f) {
	// 				this->OpaquePercentage += 1.0f;
	// 			}
	// 			else if (Pixel[i][3] <= 0.001f) {
	// 				this->TransparentPercentage += 1.0f;
	// 			}
	// 			else {
	// 				this->TranslucentPercentage += 1.0f;
	// 			}
	// 		}

	// 		// Normalize the percentages.
	// 		this->OpaquePercentage /= TotalPixels;
	// 		this->TransparentPercentage /= TotalPixels;
	// 		this->TranslucentPercentage /= TotalPixels;
	// 	}
	// 	this->CreateInfo.imageType = VK_IMAGE_TYPE_2D;
	// 	this->CreateInfo.extent.width = lWidth;
	// 	this->CreateInfo.extent.height = lHeight;
	// 	this->CreateInfo.extent.depth = 1;
	// 	this->CreateInfo.arrayLayers = 1;
	// }

	image::image(format aFormat, unsigned int aX, unsigned int aY, unsigned int aZ, unsigned int aT, size_t aSourceSize, void* aSourceData) : image() {
		this->CreateInfo.format = (VkFormat)aFormat;
		this->CreateInfo.extent = {aX, aY, aZ};
		this->CreateInfo.arrayLayers = aT;
			
		size_t PixelSize = bytes_per_pixel(aFormat);

		this->HostSize = aX * aY * aZ * aT * PixelSize;
		this->HostData = malloc(this->HostSize);

		// Check if source data is provided.
		if (aSourceSize != 0) {
			// Check if provided data matches pixel size
			if (aSourceSize != PixelSize || aSourceData == nullptr) {
				throw std::runtime_error("Invalid source data or size does not match pixel format");
			}
			for (size_t i = 0; i < (this->HostSize / PixelSize); i++) {
				memcpy((char*)this->HostData + i * PixelSize, aSourceData, PixelSize);
			}
		}
		else {
			memset(this->HostData, 0, this->HostSize);
		}
	}

	// image::image(std::shared_ptr<context> aContext, create_info aCreateInfo, std::string aFilePath) : image(aFilePath) {}

	image::image(std::shared_ptr<context> aContext, create_info aCreateInfo, std::shared_ptr<image> aHostImage) 
	: 
	image(aContext, aCreateInfo, (format)aHostImage->CreateInfo.format, aHostImage->CreateInfo.extent.width, aHostImage->CreateInfo.extent.height, aHostImage->CreateInfo.extent.depth, aHostImage->CreateInfo.arrayLayers, aHostImage->HostData) 
	{}

	image::image(std::shared_ptr<context> aContext, create_info aCreateInfo, format aFormat, unsigned int aX, unsigned int aY, unsigned int aZ, unsigned int aT, void* aTextureData) : image() {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateImage vkCreateImage = (PFN_vkCreateImage)aContext->function_pointer("vkCreateImage");
		PFN_vkBindImageMemory vkBindImageMemory = (PFN_vkBindImageMemory)aContext->function_pointer("vkBindImageMemory");

		// Image Handle Info
		this->Context								= aContext;

		this->CreateInfo.sType						= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		this->CreateInfo.pNext						= NULL;
		this->CreateInfo.flags						= 0;
		if ((aY == 1) && (aZ == 1)) {
			// 1D Image
			this->CreateInfo.imageType 					= VK_IMAGE_TYPE_1D;
		} else if (aZ == 1) {
			// 2D Image
			this->CreateInfo.imageType 					= VK_IMAGE_TYPE_2D;
		} else {
			// 3D Image
			this->CreateInfo.imageType 					= VK_IMAGE_TYPE_3D;
		}
		this->CreateInfo.format						= (VkFormat)aFormat;
		this->CreateInfo.extent						= { aX, aY, aZ };
		if (aCreateInfo.MipLevels) {
			this->CreateInfo.mipLevels					= std::floor(std::log2(std::max(std::max(aX, aY), aZ))) + 1;
		}
		else {
			this->CreateInfo.mipLevels					= 1;
		}
		this->CreateInfo.arrayLayers				= aT;
		this->CreateInfo.samples					= (VkSampleCountFlagBits)aCreateInfo.Sample;
		this->CreateInfo.tiling						= (VkImageTiling)aCreateInfo.Tiling;
		this->CreateInfo.usage						= (VkImageUsageFlags)aCreateInfo.Usage;
		this->CreateInfo.sharingMode				= VK_SHARING_MODE_EXCLUSIVE;
		this->CreateInfo.queueFamilyIndexCount		= 0;
		this->CreateInfo.pQueueFamilyIndices		= NULL;
		this->CreateInfo.initialLayout				= VK_IMAGE_LAYOUT_UNDEFINED;

		Result = vkCreateImage(aContext->Handle, &CreateInfo, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			// TODO: Error handling
			throw std::runtime_error("Failed to create image.");
		}

		// Get memory requirements of the image object.
		VkMemoryRequirements MemoryRequirements = this->memory_requirements();

		// Find the memory index for the heap that best suits the memory requirements, and desired memory properties.
		this->MemoryType = aCreateInfo.Memory;
		this->MemoryHandle = this->Context->allocate_memory(MemoryRequirements, aCreateInfo.Memory);

		// Bind the image object to the memory object.
		Result = vkBindImageMemory(this->Context->Handle, this->Handle, this->MemoryHandle, 0);
		if (Result != VK_SUCCESS) {
			// TODO: Error handling
			throw std::runtime_error("Failed to create image.");
		}

		// Write data to the image.
		if (aTextureData != NULL) {

			// Transition to TRANSFER_DST_OPTIMAL
			Result = this->transition(LAYOUT_UNDEFINED, TRANSFER_DST_OPTIMAL);

			// Write the texture data to the image.
			Result = this->write({ 0, 0, 0 }, 0, aTextureData, 0, { aX, aY, aZ }, aT);

			// Generate mipmaps & transition to SHADER_READ_ONLY_OPTIMAL
			Result = this->generate_mipmaps(TRANSFER_DST_OPTIMAL, SHADER_READ_ONLY_OPTIMAL, VK_FILTER_LINEAR);

		}
		else {
			// Transition to SHADER_READ_ONLY_OPTIMAL
			Result = this->transition(LAYOUT_UNDEFINED, (layout)aCreateInfo.Layout);
		}

		this->CreateInfo.initialLayout				= (VkImageLayout)aCreateInfo.Layout;

		// Create Image View
		this->View = this->view();
	}

	// Destructor
	image::~image() {
		if (View != VK_NULL_HANDLE) {
			PFN_vkDestroyImageView vkDestroyImageView = (PFN_vkDestroyImageView)this->Context->function_pointer("vkDestroyImageView");
			vkDestroyImageView(Context->Handle, View, NULL);
		}
		if (Handle != VK_NULL_HANDLE) {
			PFN_vkDestroyImage vkDestroyImage = (PFN_vkDestroyImage)this->Context->function_pointer("vkDestroyImage");
			vkDestroyImage(Context->Handle, Handle, NULL);
			Context->free_memory(MemoryHandle);
		}
		// if (HostData != NULL) {
		// 	stbi_image_free(HostData);
		// }
	}

	// Device Operation Support: T.
	void image::copy(command_buffer* aCommandBuffer, VkOffset3D aDestinationOffset, uint32_t aDestinationArrayLayer, std::shared_ptr<buffer> aSourceData, size_t aSourceOffset, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkBufferImageCopy Region{};
		Region.bufferOffset 		= aSourceOffset;
		Region.bufferRowLength 		= 0;
		Region.bufferImageHeight 	= 0;
		Region.imageSubresource 	= { aspect_flag(CreateInfo.format), 0, aDestinationArrayLayer, aArrayLayerCount };
		Region.imageOffset 			= aDestinationOffset;
		Region.imageExtent 			= aRegionExtent;
		std::vector<VkBufferImageCopy> RegionList = { Region };
		this->copy(aCommandBuffer, aSourceData, RegionList);
	}

	void image::copy(command_buffer* aCommandBuffer, std::shared_ptr<buffer> aSourceData, std::vector<VkBufferImageCopy> aRegionList) {
		PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)this->Context->function_pointer("vkCmdCopyBufferToImage");
		vkCmdCopyBufferToImage(aCommandBuffer->Handle, aSourceData->Handle, this->Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, aRegionList.size(), aRegionList.data());
	}

	void image::copy(command_buffer* aCommandBuffer, VkOffset3D aDestinationOffset, uint32_t aDestinationArrayLayer, std::shared_ptr<image> aSourceData, VkOffset3D aSourceOffset, uint32_t aSourceArrayLayer, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkImageCopy Region{};
		Region.srcSubresource 		= { aspect_flag(aSourceData->CreateInfo.format), 0, aSourceArrayLayer, aArrayLayerCount };
		Region.srcOffset 			= aSourceOffset;
		Region.dstSubresource 		= { aspect_flag(this->CreateInfo.format), 0, aDestinationArrayLayer, aArrayLayerCount };
		Region.dstOffset 			= aDestinationOffset;
		Region.extent 				= aRegionExtent;
		std::vector<VkImageCopy> RegionList = { Region };
		this->copy(aCommandBuffer, aSourceData, RegionList);
	}

	void image::copy(command_buffer* aCommandBuffer, std::shared_ptr<image> aSourceData, std::vector<VkImageCopy> aRegionList) {
		PFN_vkCmdCopyImage vkCmdCopyImage = (PFN_vkCmdCopyImage)this->Context->function_pointer("vkCmdCopyImage");
		vkCmdCopyImage(
			aCommandBuffer->Handle,
			aSourceData->Handle, 	VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			this->Handle, 			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			aRegionList.size(), aRegionList.data()
		);
	}

	void image::transition(
		command_buffer* aCommandBuffer,
		layout aCurrentLayout, layout aFinalLayout,
		VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask,
		uint32_t aMipLevel, uint32_t aMipLevelCount,
		uint32_t aArrayLayerStart, uint32_t aArrayLayerCount
	) {
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)this->Context->function_pointer("vkCmdPipelineBarrier");
		VkImageMemoryBarrier ImageMemoryBarrier = this->memory_barrier(
			// All Write Ops Must Finish			// Prepare Reading
			device::access::MEMORY_WRITE, 			device::access::MEMORY_READ,
			// Previous Layout						// New Layout
			aCurrentLayout, 						aFinalLayout,
			aMipLevel, 								aMipLevelCount,
			aArrayLayerStart, 						aArrayLayerCount			
		);
		vkCmdPipelineBarrier(
			aCommandBuffer->Handle,
			aSrcStageMask, aDstStageMask, 
			0,
			0, NULL,
			0, NULL,
			1, &ImageMemoryBarrier
		);
	}

	void image::clear(command_buffer* aCommandBuffer, VkClearColorValue aClearColor, image::layout aCurrentImageLayout, uint32_t aStartingArrayLayer, uint32_t aArrayLayerCount) {
		VkImageSubresourceRange SubresourceRange{};
		PFN_vkCmdClearColorImage vkCmdClearColorImage = (PFN_vkCmdClearColorImage)this->Context->function_pointer("vkCmdClearColorImage");
		SubresourceRange.aspectMask 	= VK_IMAGE_ASPECT_COLOR_BIT;
		SubresourceRange.baseMipLevel 	= 0;
		SubresourceRange.levelCount 	= this->CreateInfo.mipLevels;
		SubresourceRange.baseArrayLayer = std::min(aStartingArrayLayer, this->CreateInfo.arrayLayers - 1);
		SubresourceRange.layerCount 	= std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - SubresourceRange.baseArrayLayer);
		this->transition(aCommandBuffer, aCurrentImageLayout, TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, this->CreateInfo.mipLevels, aStartingArrayLayer, aArrayLayerCount);
		vkCmdClearColorImage(aCommandBuffer->Handle, this->Handle, (VkImageLayout)TRANSFER_DST_OPTIMAL, &aClearColor, 1, &SubresourceRange);
		this->transition(aCommandBuffer, TRANSFER_DST_OPTIMAL, aCurrentImageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, this->CreateInfo.mipLevels, aStartingArrayLayer, aArrayLayerCount);
	}

	void image::clear_depth(command_buffer* aCommandBuffer, VkClearDepthStencilValue aClearDepthStencil, image::layout aCurrentImageLayout, uint32_t aStartingArrayLayer, uint32_t aArrayLayerCount) {
		VkImageSubresourceRange SubresourceRange{};
		PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)this->Context->function_pointer("vkCmdClearDepthStencilImage");
		SubresourceRange.aspectMask 	= VK_IMAGE_ASPECT_DEPTH_BIT;
		SubresourceRange.baseMipLevel 	= 0;
		SubresourceRange.levelCount 	= this->CreateInfo.mipLevels;
		SubresourceRange.baseArrayLayer = std::min(aStartingArrayLayer, this->CreateInfo.arrayLayers - 1);
		SubresourceRange.layerCount 	= std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - SubresourceRange.baseArrayLayer);
		this->transition(aCommandBuffer, aCurrentImageLayout, TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, this->CreateInfo.mipLevels, aStartingArrayLayer, aArrayLayerCount);
		vkCmdClearDepthStencilImage(aCommandBuffer->Handle, this->Handle, (VkImageLayout)TRANSFER_DST_OPTIMAL, &aClearDepthStencil, 1, &SubresourceRange);
		this->transition(aCommandBuffer, TRANSFER_DST_OPTIMAL, aCurrentImageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, this->CreateInfo.mipLevels, aStartingArrayLayer, aArrayLayerCount);
	}

	VkResult image::copy(VkOffset3D aDestinationOffset, uint32_t aDestinationArrayLayer, std::shared_ptr<buffer> aSourceData, size_t aSourceOffset, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkBufferImageCopy Region{};
		Region.bufferOffset 						= aSourceOffset;
		Region.bufferRowLength 						= 0;
		Region.bufferImageHeight 					= 0;
		Region.imageSubresource 					= { aspect_flag(this->CreateInfo.format), 0, aDestinationArrayLayer, std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - aDestinationArrayLayer) };
		Region.imageOffset 							= aDestinationOffset;
		Region.imageExtent 							= aRegionExtent;
		std::vector<VkBufferImageCopy> RegionList = { Region };
		return this->copy(aSourceData, RegionList);
	}

	VkResult image::copy(std::shared_ptr<buffer> aSourceData, std::vector<VkBufferImageCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::TRANSFER);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record command buffer.
		Result = CommandBuffer->begin();
		this->copy(CommandBuffer.get(), aSourceData, aRegionList);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::TRANSFER, CommandBuffer);

		return Result;
	}

	VkResult image::copy(VkOffset3D aDestinationOffset, uint32_t aDestinationArrayLayer, std::shared_ptr<image> aSourceData, VkOffset3D aSourceOffset, uint32_t aSourceArrayLayer, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkImageCopy Region {};
		Region.srcSubresource 	= { aspect_flag(aSourceData->CreateInfo.format), 0, aSourceArrayLayer, std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - aSourceArrayLayer) };
		Region.srcOffset 		= aSourceOffset;
		Region.dstSubresource 	= { aspect_flag(this->CreateInfo.format), 0, aDestinationArrayLayer, std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - aDestinationArrayLayer) };
		Region.dstOffset 		= aDestinationOffset;
		Region.extent 			= aRegionExtent;
		std::vector<VkImageCopy> RegionList = { Region };
		return this->copy(aSourceData, RegionList);
	}

	VkResult image::copy(std::shared_ptr<image> aSourceData, std::vector<VkImageCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::TRANSFER);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record command buffer.
		Result = CommandBuffer->begin();
		this->copy(CommandBuffer.get(), aSourceData, aRegionList);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::TRANSFER, CommandBuffer);

		return Result;
	}

	VkResult image::transition(
		layout aCurrentLayout, layout aFinalLayout,
		uint32_t aMipLevel, uint32_t aMipLevelCount,
		uint32_t aArrayLayerStart, uint32_t aArrayLayerCount
	) {
		// This function will transition the designated image resources after 
		// all other operations are completed.
		VkResult Result = VK_SUCCESS;
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)this->Context->function_pointer("vkCmdPipelineBarrier");
		VkImageMemoryBarrier ImageMemoryBarrier = this->memory_barrier(
			// All Write Ops Must Finish			// Prepare Reading
			device::access::MEMORY_WRITE, 			device::access::MEMORY_READ,
			// Previous Layout						// New Layout
			aCurrentLayout, 						aFinalLayout,
			aMipLevel, aMipLevelCount,
			aArrayLayerStart, aArrayLayerCount			
		);
		auto CommandPool = Context->create<command_pool>(device::operation::TRANSFER);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record Command Buffer
		Result = CommandBuffer->begin();
		vkCmdPipelineBarrier(
			CommandBuffer->Handle, 
			pipeline::stage::BOTTOM_OF_PIPE, pipeline::stage::TOP_OF_PIPE, 
			0,
			0, NULL,
			0, NULL,
			1, &ImageMemoryBarrier
		);
		Result = CommandBuffer->end();

		// Execute
		Result = Context->execute_and_wait(device::operation::TRANSFER, CommandBuffer);

		return Result;
	}

	VkResult image::clear(VkClearColorValue aClearColor, image::layout aCurrentImageLayout, uint32_t aStartingArrayLayer, uint32_t aArrayLayerCount) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record command buffer.
		Result = CommandBuffer->begin();
		this->clear(CommandBuffer.get(), aClearColor, aCurrentImageLayout, aStartingArrayLayer, aArrayLayerCount);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}

	VkResult image::clear_depth(VkClearDepthStencilValue aClearDepthStencil, image::layout aCurrentImageLayout, uint32_t aStartingArrayLayer, uint32_t aArrayLayerCount) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record command buffer.
		Result = CommandBuffer->begin();
		this->clear_depth(CommandBuffer.get(), aClearDepthStencil, aCurrentImageLayout, aStartingArrayLayer, aArrayLayerCount);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}

	VkResult image::generate_mipmaps(layout aCurrentLayout, layout aFinalLayout, VkFilter aFilter) {
		VkResult Result = VK_SUCCESS;
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)this->Context->function_pointer("vkCmdPipelineBarrier");
		PFN_vkCmdBlitImage vkCmdBlitImage = (PFN_vkCmdBlitImage)this->Context->function_pointer("vkCmdBlitImage");

		// ----- Generate MipMaps ----- // 

		this->transition(aCurrentLayout, layout::TRANSFER_DST_OPTIMAL);

		auto CommandPool = Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		VkOffset3D d = { (int32_t)this->CreateInfo.extent.width, (int32_t)this->CreateInfo.extent.height, (int32_t)this->CreateInfo.extent.depth };

		Result = CommandBuffer->begin();
		for (uint32_t i = 0; i < this->CreateInfo.mipLevels - 1; i++) {
			VkImageMemoryBarrier ImageTransition{};
			VkImageBlit IBO{};

			// Image Blitting Source Level
			IBO.srcSubresource.aspectMask = aspect_flag(this->CreateInfo.format);
			IBO.srcSubresource.mipLevel = i;
			IBO.srcSubresource.baseArrayLayer = 0;
			IBO.srcSubresource.layerCount = this->CreateInfo.arrayLayers;
			IBO.srcOffsets[0] = { 0, 0, 0 };
			IBO.srcOffsets[1] = { (d.x >> i) ? d.x >> i : 1, (d.y >> i) ? d.y >> i : 1, (d.z >> i) ? d.z >> i : 1 };

			// Image Blitting Destination Level
			IBO.dstSubresource.aspectMask = aspect_flag(this->CreateInfo.format);
			IBO.dstSubresource.mipLevel = i + 1;
			IBO.dstSubresource.baseArrayLayer = 0;
			IBO.dstSubresource.layerCount = this->CreateInfo.arrayLayers;
			IBO.dstOffsets[0] = { 0, 0, 0 };
			IBO.dstOffsets[1] = { (d.x >> (i + 1)) ? d.x >> (i + 1) : 1, (d.y >> (i + 1)) ? d.y >> (i + 1) : 1, (d.z >> (i + 1)) ? d.z >> (i + 1) : 1 };

			// Source Image Section
			ImageTransition = this->memory_barrier(
				// All Write Ops Must Finish			// Prepare Reading
				device::access::MEMORY_WRITE, device::access::MEMORY_READ,
				// Previous Layout						// New Layout
				image::layout::TRANSFER_DST_OPTIMAL, image::layout::TRANSFER_SRC_OPTIMAL,
				i, 1
			);

			vkCmdPipelineBarrier(
				CommandBuffer->Handle,
				pipeline::stage::BOTTOM_OF_PIPE, pipeline::stage::TOP_OF_PIPE,
				0,
				0, NULL,
				0, NULL,
				1, &ImageTransition
			);

			vkCmdBlitImage(
				CommandBuffer->Handle,
				this->Handle, (VkImageLayout)layout::TRANSFER_SRC_OPTIMAL,
				this->Handle, (VkImageLayout)layout::TRANSFER_DST_OPTIMAL,
				1, &IBO,
				aFilter
			);

		}

		// Transition the final mip level to TRANSFER_SRC_OPTIMAL.
		VkImageMemoryBarrier FinalTransition = this->memory_barrier(
			device::access::MEMORY_WRITE, device::access::MEMORY_READ,
			image::layout::TRANSFER_DST_OPTIMAL, image::layout::TRANSFER_SRC_OPTIMAL,
			this->CreateInfo.mipLevels - 1, 1
		);

		vkCmdPipelineBarrier(
			CommandBuffer->Handle,
			pipeline::stage::BOTTOM_OF_PIPE, pipeline::stage::TOP_OF_PIPE,
			0,
			0, NULL,
			0, NULL,
			1, &FinalTransition
		);

		FinalTransition = this->memory_barrier(
			device::access::MEMORY_WRITE, 			device::access::MEMORY_READ,
			image::layout::TRANSFER_SRC_OPTIMAL, 	aFinalLayout
		);

		vkCmdPipelineBarrier(
			CommandBuffer->Handle,
			pipeline::stage::BOTTOM_OF_PIPE, pipeline::stage::TOP_OF_PIPE,
			0,
			0, NULL,
			0, NULL,
			1, &FinalTransition
		);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}


	// Write to image data memory from host memory.
	VkResult image::write(VkOffset3D aDestinationOffset, uint32_t aDestinationArrayLayer, void* aSourceData, size_t aSourceOffset, VkExtent3D aDestinationExtent, uint32_t aDestinationArrayLayerCount) {
		VkBufferImageCopy Region{};
		Region.bufferOffset 						= aSourceOffset;
		Region.bufferRowLength 						= 0;
		Region.bufferImageHeight 					= 0;
		Region.imageSubresource.aspectMask 			= this->aspect_flag(this->CreateInfo.format);
		Region.imageSubresource.mipLevel 			= 0;
		Region.imageSubresource.baseArrayLayer 		= aDestinationArrayLayer;
		Region.imageSubresource.layerCount 			= std::min(aDestinationArrayLayerCount, this->CreateInfo.arrayLayers - aDestinationArrayLayer);
		Region.imageOffset 							= aDestinationOffset;
		Region.imageExtent 							= aDestinationExtent;
		std::vector<VkBufferImageCopy> RegionList = { Region };
		return this->write(aSourceData, RegionList);
	}

	VkResult image::write(void* aSourceData, std::vector<VkBufferImageCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;

		// Staging Buffer size is determined by the largest transfer region, and then allocated so that all regions can be copied to the buffer.
		// without reallocation.

		size_t StagingBufferSize = 0;
		for (const VkBufferImageCopy& Region : aRegionList) {
			size_t RegionSize = Region.imageExtent.width * Region.imageExtent.height * Region.imageExtent.depth * Region.imageSubresource.layerCount * bytes_per_pixel(this->CreateInfo.format);
			if (StagingBufferSize < RegionSize) {
				StagingBufferSize = RegionSize;
			}
		}

		std::shared_ptr<buffer> StagingBuffer = std::make_shared<buffer>(
			Context,
			device::memory::DEVICE_LOCAL,
			buffer::TRANSFER_SRC | buffer::TRANSFER_DST,
			StagingBufferSize
		);

		for (const VkBufferImageCopy& Region : aRegionList) {
			// Calculate Region Size
			size_t RegionSize = Region.imageExtent.width * Region.imageExtent.height * Region.imageExtent.depth * Region.imageSubresource.layerCount * bytes_per_pixel(this->CreateInfo.format);
			
			// Write data to device buffer.
			Result = StagingBuffer->write(0, aSourceData, Region.bufferOffset, RegionSize);

			// Copy data from device buffer to image.
			Result = this->copy(Region.imageOffset, Region.imageSubresource.baseArrayLayer, StagingBuffer, Region.bufferOffset, Region.imageExtent, Region.imageSubresource.layerCount);
		}

		return Result;
	}

	// Read from image data memory to host memory.
	VkResult image::read(VkOffset3D aSourceOffset, uint32_t aSourceArrayLayer, void* aDestinationData, size_t aDestinationOffset, VkExtent3D aSourceExtent, uint32_t aSourceArrayLayerCount) {
		VkBufferImageCopy Region{};
		Region.bufferOffset 						= aDestinationOffset;
		Region.bufferRowLength 						= 0;
		Region.bufferImageHeight 					= 0;
		Region.imageSubresource.aspectMask 			= this->aspect_flag(this->CreateInfo.format);
		Region.imageSubresource.mipLevel 			= 0;
		Region.imageSubresource.baseArrayLayer 		= aSourceArrayLayer;
		Region.imageSubresource.layerCount 			= std::min(aSourceArrayLayerCount, this->CreateInfo.arrayLayers - aSourceArrayLayer);
		Region.imageOffset 							= aSourceOffset;
		Region.imageExtent 							= aSourceExtent;
		std::vector<VkBufferImageCopy> RegionList = { Region };
		return this->read(aDestinationData, RegionList);
	}

	VkResult image::read(void* aDestinationData, std::vector<VkBufferImageCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;

		size_t StagingBufferSize = 0;
		for (const VkBufferImageCopy& Region : aRegionList) {
			size_t RegionSize = Region.imageExtent.width * Region.imageExtent.height * Region.imageExtent.depth * Region.imageSubresource.layerCount * bytes_per_pixel(this->CreateInfo.format);
			if (StagingBufferSize < RegionSize) {
				StagingBufferSize = RegionSize;
			}
		}

		std::shared_ptr<buffer> StagingBuffer = std::make_shared<buffer>(
			Context,
			device::memory::DEVICE_LOCAL,
			buffer::TRANSFER_SRC | buffer::TRANSFER_DST,
			StagingBufferSize
		);

		for (const VkBufferImageCopy& Region : aRegionList) {
			// Calculate Region Size
			size_t RegionSize = Region.imageExtent.width * Region.imageExtent.height * Region.imageExtent.depth * Region.imageSubresource.layerCount * bytes_per_pixel(this->CreateInfo.format);

			// Copy data from texture to staging buffer
			Result = StagingBuffer->copy(0, this->shared_from_this(), Region.imageOffset, Region.imageSubresource.baseArrayLayer, Region.imageExtent, Region.imageSubresource.layerCount);

			// Read data from device buffer.
			Result = StagingBuffer->read(0, aDestinationData, Region.bufferOffset, RegionSize);
			
		}

		return Result;
	}

	VkImageView image::view(
		uint32_t aMipLevel, uint32_t aMipLevelCount,
		uint32_t aArrayLayerStart, uint32_t aArrayLayerCount
	) const {
		VkResult Result = VK_SUCCESS;
		VkImageViewCreateInfo IVCI{};
		VkImageView IV = VK_NULL_HANDLE;
		PFN_vkCreateImageView vkCreateImageView = (PFN_vkCreateImageView)this->Context->function_pointer("vkCreateImageView");
		IVCI.sType								= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		IVCI.pNext								= NULL;
		IVCI.flags								= 0;
		IVCI.image								= this->Handle;
		switch(CreateInfo.imageType) {
		case VK_IMAGE_TYPE_1D:
			IVCI.viewType 							= VK_IMAGE_VIEW_TYPE_1D;
			break;
		case VK_IMAGE_TYPE_2D:
			IVCI.viewType 							= VK_IMAGE_VIEW_TYPE_2D;
			break;
		case VK_IMAGE_TYPE_3D:
			IVCI.viewType 							= VK_IMAGE_VIEW_TYPE_3D;
			break;
		default:
			IVCI.viewType 							= VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			break;
		}
		IVCI.format								= CreateInfo.format;
		IVCI.components							= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		IVCI.subresourceRange.aspectMask		= aspect_flag(CreateInfo.format);
		IVCI.subresourceRange.baseMipLevel		= aMipLevel;
		IVCI.subresourceRange.levelCount		= std::min(aMipLevelCount, CreateInfo.mipLevels - aMipLevel);
		IVCI.subresourceRange.baseArrayLayer	= aArrayLayerStart;
		IVCI.subresourceRange.layerCount		= std::min(aArrayLayerCount, CreateInfo.arrayLayers - aArrayLayerStart);
		Result = vkCreateImageView(this->Context->Handle, &IVCI, NULL, &IV);
		return IV;
	}

	VkAttachmentDescription image::description(layout aStartingLayout, layout aEndingLayout) const {
		VkAttachmentDescription AD{};
		AD.flags			= 0;
		AD.format			= this->CreateInfo.format;
		AD.samples			= this->CreateInfo.samples;
		AD.loadOp			= VK_ATTACHMENT_LOAD_OP_LOAD;
		AD.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
		AD.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_CLEAR;
		AD.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_STORE;
		AD.initialLayout	= (VkImageLayout)aStartingLayout;
		AD.finalLayout		= (VkImageLayout)aEndingLayout;
		return AD;
	}

	VkImageMemoryBarrier image::memory_barrier(
		unsigned int aSrcAccess, unsigned int aDstAccess,
		unsigned int aOldLayout, unsigned int aNewLayout,
		uint32_t aMipLevel, uint32_t aMipLevelCount,
		uint32_t aArrayLayerStart, uint32_t aArrayLayerCount
	) const {
		VkImageMemoryBarrier MemoryBarrier {};
		MemoryBarrier.sType								= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		MemoryBarrier.pNext								= NULL;
		MemoryBarrier.srcAccessMask						= aSrcAccess;
		MemoryBarrier.dstAccessMask						= aDstAccess;
		MemoryBarrier.oldLayout							= (VkImageLayout)aOldLayout;
		MemoryBarrier.newLayout							= (VkImageLayout)aNewLayout;
		MemoryBarrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		MemoryBarrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
		MemoryBarrier.image								= this->Handle;
		MemoryBarrier.subresourceRange.aspectMask		= this->aspect_flag(this->CreateInfo.format);
		MemoryBarrier.subresourceRange.baseMipLevel		= aMipLevel;
		MemoryBarrier.subresourceRange.levelCount 		= std::min(aMipLevelCount, this->CreateInfo.mipLevels - aMipLevel);
		MemoryBarrier.subresourceRange.baseArrayLayer	= aArrayLayerStart;
		MemoryBarrier.subresourceRange.layerCount		= std::min(aArrayLayerCount, this->CreateInfo.arrayLayers - aArrayLayerStart);
		return MemoryBarrier;
	}

	VkMemoryRequirements image::memory_requirements() const {
		return this->Context->get_image_memory_requirements(this->Handle);
	}

	bool image::has_alpha_channel() const {
		switch (this->CreateInfo.format) {
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_USCALED:
		case VK_FORMAT_R8G8B8A8_SSCALED:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_SNORM:
		case VK_FORMAT_B8G8R8A8_USCALED:
		case VK_FORMAT_B8G8R8A8_SSCALED:
		case VK_FORMAT_B8G8R8A8_UINT:
		case VK_FORMAT_B8G8R8A8_SINT:
		case VK_FORMAT_B8G8R8A8_SRGB:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		case VK_FORMAT_R16G16B16A16_UNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
		case VK_FORMAT_R16G16B16A16_USCALED:
		case VK_FORMAT_R16G16B16A16_SSCALED:
		case VK_FORMAT_R16G16B16A16_UINT:
		case VK_FORMAT_R16G16B16A16_SINT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		case VK_FORMAT_R32G32B32A32_UINT:
		case VK_FORMAT_R32G32B32A32_SINT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		case VK_FORMAT_R64G64B64A64_UINT:
		case VK_FORMAT_R64G64B64A64_SINT:
		case VK_FORMAT_R64G64B64A64_SFLOAT:
			return true;
		default:
			return false;
		}
	}

	int image::transparency(int aChannelSelection) const {
		int TransparencyType = -1; // 0 = Opaque, 1 = Transparent, 2 = Translucent
		// Opaque - 0: All pixels are 1.0f
		// Transparent - 1: At least one pixel is 0.0f
		// Translucent - 2: x% of pixels are greater than 0.0f and less than 0.95f.

		// Check if Channel Selection is valid
		if ((aChannelSelection < 0) || (aChannelSelection >= image::channel_count(this->CreateInfo.format))) return TransparencyType;

		return TransparencyType;
	}

}
