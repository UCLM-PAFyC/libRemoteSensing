#ifndef ALGORITHMS_DEFINITIONS_H
#define ALGORITHMS_DEFINITIONS_H

#include "gdal.h"

#include "SceneLandsat8_definitions.h"
#include "SceneSentinel2_definitions.h"
#include "ParameterDefinitions.h"

#define ALGORITHMS_DATE_STRING_FORMAT                               "yyyy/MM/dd"
#define ALGORITHMS_DATE_TIME_FILE_NAME_STRING_FORMAT                "yyyyMMdd_hhmmss"
#define ALGORITHMS_RESULTS_FILE_EXTENSION                           "txt"

#define ALGORITHMS_NDVI_CODE                                        "NDVI"
#define ALGORITHMS_NDVI_GUI_TAG                                     "Normalized Difference Vegetation Index Computation"
#define ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT                      "NDVI_ImageFormat"
#define ALGORITHMS_NDVI_PARAMETER_IMAGE_FORMAT_1                    "GTiff"
#define ALGORITHMS_NDVI_PARAMETER_IMAGE_SUFFIX                      "NDVI_ImageSuffix"
#define ALGORITHMS_NDVI_PARAMETER_COMPUTATION_METHOD                "NDVI_ComputationMethod"
#define ALGORITHMS_NDVI_COMPUTATION_METHOD_1                        "Ref_TOA"
#define ALGORITHMS_NDVI_COMPUTATION_METHOD_2                        "DN"
#define ALGORITHMS_NDVI_COMPUTATION_METHOD_3                        "DN8Bits"
#define ALGORITHMS_NDVI_PARAMETER_NO_DATA_VALUE                     "NDVI_NoDataValue"
#define ALGORITHMS_NDVI_PARAMETER_IMAGE_OPTIONS                     "NDVI_ImageOptions"
#define ALGORITHMS_NDVI_PARAMETER_BUILD_OVERVIEWS                   "NDVI_BuildOverviews"

#define ALGORITHMS_CLOUDREMOVAL_CODE                                "CLOUDREMOVAL"
#define ALGORITHMS_CLOUDREMOVAL_GUI_TAG                             "Cloud Removal"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_NUMBER_OF_DATES           "CLOUDREMOVAL_Num_Dates"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_MAX_PER_PART_CLOUDY       "CLOUDREMOVAL_MaxPerPartCloudy"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_INTERPOLATION_METHOD      "CLOUDREMOVAL_interpolationMethod"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_REMOVE_FULL_CLOUDY        "CLOUDREMOVAL_removeFullCloudy"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_APPLY_CLOUD_FREE_IMPROVEMENT  "CLOUDREMOVAL_applyCloudFreeImprovement"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS        "CLOUDREMOVAL_BandsCombinations"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_STRING_SEPARATOR       ENUM_CHARACTER_SEPARATOR
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_BANDS_COMBINATIONS_BANDS_STRING_SEPARATOR        "#"
#define ALGORITHMS_CLOUDREMOVAL_PARAMETER_WRITEIMAGEFILES_FOLDER    "woc"

#define ALGORITHMS_INTERPOLATION_METHOD_AKIMA_SPLINE                "AkimaSpline"
#define ALGORITHMS_INTERPOLATION_METHOD_CUBIC_SPLINE                "CubicSpline"
#define ALGORITHMS_INTERPOLATION_METHOD_BESSEL_SPLINE               "BesselSpline"

#define ALGORITHMS_INTC_CODE                                        "INTC"
#define ALGORITHMS_INTC_GUI_TAG                                     "Intercalibration"
#define ALGORITHMS_INTC_PARAMETER_GAIN                              "INTC_GAIN"
#define ALGORITHMS_INTC_PARAMETER_OFFSET                            "INTC_OFFSET"
#define ALGORITHMS_INTC_PARAMETER_MIN_PIAS_PIXELS                   "INTC_Min_Pias_Pixels"
#define ALGORITHMS_INTC_PARAMETER_MIN_BANDS_PIXELS                  "INTC_Min_Bands_Pixels"
#define ALGORITHMS_INTC_PARAMETER_TO8BITS                           "INTC_to8bits"
#define ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES                   "INTC_writeImageFiles"
#define ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILES_FOLDER            "intc"
#define ALGORITHMS_INTC_PARAMETER_WRITEIMAGEFILESTO8BITS            "INTC_writeImageTo8bits"
#define ALGORITHMS_INTC_PARAMETER_TOREFLECTANCE                     "INTC_toReflectance"
#define ALGORITHMS_INTC_PARAMETER_REMOVE_OUTLIERS                   "INTC_removeOutliers"
#define ALGORITHMS_INTC_PARAMETER_INTERPOLATION_METHOD              "INTC_interpolationMethod"
#define ALGORITHMS_INTC_PARAMETER_WEIGHT_REF                        "INTC_WEIGHT_REF"
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_1                    REMOTESENSING_LANDSAT8_BAND_B2_CODE
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_2                    REMOTESENSING_LANDSAT8_BAND_B3_CODE
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_3                    REMOTESENSING_LANDSAT8_BAND_B4_CODE
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_4                    REMOTESENSING_LANDSAT8_BAND_B5_CODE
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_5                    REMOTESENSING_LANDSAT8_BAND_B6_CODE
#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_6                    REMOTESENSING_LANDSAT8_BAND_B7_CODE
//#define ALGORITHMS_INTC_PROCESS_LANDSAT8_BANDS_7                    REMOTESENSING_LANDSAT8_BAND_B8_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_1                   REMOTESENSING_SENTINEL2_BAND_B2_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_2                   REMOTESENSING_SENTINEL2_BAND_B3_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_3                   REMOTESENSING_SENTINEL2_BAND_B4_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_4                   REMOTESENSING_SENTINEL2_BAND_B5_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_5                   REMOTESENSING_SENTINEL2_BAND_B6_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_6                   REMOTESENSING_SENTINEL2_BAND_B7_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_7                   REMOTESENSING_SENTINEL2_BAND_B8_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_8                   REMOTESENSING_SENTINEL2_BAND_B8A_CODE
//#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_9                   REMOTESENSING_SENTINEL2_BAND_B9_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_10                  REMOTESENSING_SENTINEL2_BAND_B10_CODE
#define ALGORITHMS_INTC_PROCESS_SENTINEL2_BANDS_11                  REMOTESENSING_SENTINEL2_BAND_B11_CODE


#define ALGORITHMS_PIAS_CODE                                        "PIAS"
#define ALGORITHMS_PIAS_GUI_TAG                                     "Pseudo-invariant Areas Extraction"
#define ALGORITHMS_PIAS_PARAMETER_NDVI_STD_MAX                      "PIAS_NDVI_Std_Max"
#define ALGORITHMS_PIAS_PARAMETER_NDVI_MIN                          "PIAS_NDVI_Min"
#define ALGORITHMS_PIAS_PARAMETER_NDVI_MAX                          "PIAS_NDVI_Max"
#define ALGORITHMS_PIAS_PARAMETER_PIA_VALUE                         "PIAS_PIA_Value"
#define ALGORITHMS_PIAS_PARAMETER_PIA_NO_DATA_VALUE                 "PIAS_PIA_NoDataValue"
#define ALGORITHMS_PIAS_PARAMETER_PIA_CLOUD_VALUE                   "PIAS_CLOUD_VALUE"
#define ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT                      "PIAS_ImageFormat"
#define ALGORITHMS_PIAS_PARAMETER_IMAGE_FORMAT_1                    "GTiff"
#define ALGORITHMS_PIAS_PARAMETER_IMAGE_BASE_NAME                   "PIAS_ImageBaseName"
#define ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD                "PIAS_ComputationMethod"
#define ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_1              "NDVI_Statistics"
//#define ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_2              "Ref_Surf"
//#define ALGORITHMS_PIAS_PARAMETER_COMPUTATION_METHOD_3              "DN"
#define ALGORITHMS_PIAS_LANDSAT8_BAND_CLOUD_MASKS                   REMOTESENSING_LANDSAT8_BAND_B0_CODE
#define ALGORITHMS_PIAS_LANDSAT8_BAND_RED                           REMOTESENSING_LANDSAT8_BAND_B4_CODE
#define ALGORITHMS_PIAS_LANDSAT8_BAND_NIR                           REMOTESENSING_LANDSAT8_BAND_B5_CODE
#define ALGORITHMS_PIAS_SENTINEL2_BAND_CLOUD_MASKS                  REMOTESENSING_SENTINEL2_BAND_B0_CODE
#define ALGORITHMS_PIAS_SENTINEL2_BAND_RED                          REMOTESENSING_SENTINEL2_BAND_B4_CODE
#define ALGORITHMS_PIAS_SENTINEL2_BAND_NIR                          REMOTESENSING_SENTINEL2_BAND_B8_CODE
#define ALGORITHMS_PIAS_GDAL_DATA_TYPE                              GDT_Byte
#define ALGORITHMS_PIAS_PARAMETER_IMAGE_OPTIONS                     "PIAS_ImageOptions"
#define ALGORITHMS_PIAS_PARAMETER_BUILD_OVERVIEWS                   "PIAS_BuildOverviews"

#define ALGORITHMS_REFL_CODE                                        "REFL"
#define ALGORITHMS_REFL_GUI_TAG                                     "Reflectance Scenes Computation"
#define ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT                      "REFL_ImageFormat"
#define ALGORITHMS_REFL_PARAMETER_IMAGE_FORMAT_1                    "GTiff"
#define ALGORITHMS_REFL_PARAMETER_IMAGE_SUFFIX                      "REFL_ImageSuffix"
#define ALGORITHMS_REFL_PARAMETER_COMPUTATION_METHOD                "REFL_ComputationMethod"
#define ALGORITHMS_REFL_COMPUTATION_METHOD_1                        "Ref_TOA"
#define ALGORITHMS_REFL_PARAMETER_NO_DATA_VALUE                     "REFL_NoDataValue"
#define ALGORITHMS_REFL_PARAMETER_IMAGE_OPTIONS                     "REFL_ImageOptions"
#define ALGORITHMS_REFL_PARAMETER_BUILD_OVERVIEWS                   "REFL_BuildOverviews"

#endif // ALGORITMS_DEFINITIONS_H
