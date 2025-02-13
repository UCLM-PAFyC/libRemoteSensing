#ifndef PERSISTENCEMANAGER_DEFINITIONS_H
#define PERSISTENCEMANAGER_DEFINITIONS_H

#include "../libIGDAL/SpatiaLite.h"


// TABLE_NESTED_GRID
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_TABLE_NAME                                      "nested_grid"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_ID                                        "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_ID_FIELD_TYPE                             SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_GEOGRAPHIC_CRS_PROJ4                      "geo_crs_proj4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_GEOGRAPHIC_CRS_PROJ4_FIELD_TYPE           SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_PROJECTED_CRS_PROJ4                       "prj_crs_proj4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_PROJECTED_CRS_PROJ4_FIELD_TYPE            SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_LOCAL_PARAMETERS                          "local_parameters"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NESTED_GRID_FIELD_LOCAL_PARAMETERS_FIELD_TYPE               SPATIALITE_FIELD_TYPE_TEXT

// TABLE_RASTER_UNIT_CONVERSIONS
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS                                     "raster_unit_conversions"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_ID                            "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_ID_FIELD_TYPE                 SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE                          "type"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_TYPE_FIELD_TYPE               SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN                          "gain"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_FIELD_PRECISION          15
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_GAIN_DIFFERENCE_TOLERANCE     0.00001

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET                        "offset"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_FIELD_TYPE             SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_FIELD_PRECISION        8
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_OFFSET_DIFFERENCE_TOLERANCE   0.001

//#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_FIELD_EQUATION_PER_UNIT             "per_unit"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_UNIT_CONVERSIONS_NONE                                "none"

// TABLE_NDVI_FILES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES                                                  "ndvi_files"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_ID                                         "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_ID_FIELD_TYPE                              SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID                                "tuplekey_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE                     SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID                             "ruc_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_UNIT_ID_FIELD_TYPE                  SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID                      "cm_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_COMPUTATION_METHOD_ID_FIELD_TYPE           SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME                                  "file_name"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_FILE_NAME_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID                             "raster_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE                  SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_TILES                                  "lod_tiles"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_TILES_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_GSD                                    "lod_gsd"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_NDVI_FILES_FIELD_LOD_GSD_FIELD_TYPE                          SPATIALITE_FIELD_TYPE_INTEGER

// TABLE_INTERCALIBRATION
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION                                            "intercalibration"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_ID                                   "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_ID_FIELD_TYPE                        SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_BAND_ID                              "band_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_BAND_ID_FIELD_TYPE                   SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_SOURCE_RASTER_FILE_ID                "source_raster_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_SOURCE_RASTER_FILE_ID_FIELD_TYPE     SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TARGET_RASTER_FILE_ID                "target_raster_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TARGET_RASTER_FILE_ID_FIELD_TYPE     SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TO8BITS                              "to8bits"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TO8BITS_FIELD_TYPE                   SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TOREFLECTANCE                        "toReflectance"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_TOREFLECTANCE_FIELD_TYPE             SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_INTERPOLATED                         "interpolated"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_INTERPOLATED_FIELD_TYPE              SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_GAIN                                 "gain"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_GAIN_FIELD_TYPE                      SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_GAIN_FIELD_PRECISION                 15
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_GAIN_DIFFERENCE_TOLERANCE            0.00001

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_OFFSET                               "offset"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_OFFSET_FIELD_TYPE                    SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_OFFSET_FIELD_PRECISION               8
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_INTERCALIBRATION_FIELD_OFFSET_DIFFERENCE_TOLERANCE          0.001

// TABLE_PIAS_FILES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES                                                  "pias_files"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID                                         "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_ID_FIELD_TYPE                              SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID                                "tuplekey_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE                     SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE                                  "pia_value"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_PIA_VALUE_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID                      "cm_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_COMPUTATION_METHOD_ID_FIELD_TYPE           SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME                                  "file_name"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FILE_NAME_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_INITIAL_DATE                               "initial_date"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_INITIAL_DATE_FIELD_TYPE                    SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FINAL_DATE                                 "final_date"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PIAS_FILES_FIELD_FINAL_DATE_FIELD_TYPE                      SPATIALITE_FIELD_TYPE_INTEGER

// TABLE_RASTER_FILES_BY_PIAS_FILES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES                                  "raster_files_by_pias_files"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_ID                         "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_ID_FIELD_TYPE              SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID             "raster_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE  SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID               "pias_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_BY_PIAS_FILES_FIELD_PIAS_FILE_ID_FIELD_TYPE    SPATIALITE_FIELD_TYPE_INTEGER

// TABLE_COMPUTATION_METHODS
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS                                         "computation_methods"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID                                "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_ID_FIELD_TYPE                     SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE                              "type"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_COMPUTATION_METHODS_FIELD_TYPE_FIELD_TYPE                   SPATIALITE_FIELD_TYPE_TEXT


//
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_ORTHOIMAGE                                     "orthoimage"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_LANDSAT8                                       "landsat8"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_SENTINEL2                                      "sentinel2"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_NDVI                                           "ndvi"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME                                         "projects"

// TABLE_RASTER_TYPES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_TABLE_NAME                                     "raster_types"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID                                       "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_ID_FIELD_TYPE                            SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE                                     "type"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_TYPES_FIELD_TYPE_FIELD_TYPE                          SPATIALITE_FIELD_TYPE_TEXT

// TABLE_RASTER_FILES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_TABLE_NAME                                     "raster_files"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID                                       "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_ID_FIELD_TYPE                            SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID                                "raster_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_RASTER_ID_FIELD_TYPE                     SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD                                       "jd"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_JD_FIELD_TYPE                            SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID                                  "type_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_RASTER_FILES_FIELD_TYPE_ID_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_TEXT

// TABLE_TUPLEKEYS
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_TABLE_NAME                                        "tuplekeys"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID                                          "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_ID_FIELD_TYPE                               SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY                                    "tuplekey"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TUPLEKEY_FIELD_TYPE                         SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_LOD                                         "lod"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_LOD_FIELD_TYPE                              SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_X                                      "tile_x"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_X_FIELD_TYPE                           SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_Y                                      "tile_y"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_TILE_Y_FIELD_TYPE                           SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM                                    "the_geom"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_FIELD_THE_GEOM_FIELD_TYPE                         SPATIALITE_FIELD_TYPE_WKT_GEOMETRY

// TABLE_TUPLEKEYS_RASTER_FILES
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_TABLE_NAME                           "tuplekeys_raster_files"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_ID                             "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_ID_FIELD_TYPE                  SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID                    "tuplekey_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_TUPLEKEY_ID_FIELD_TYPE         SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME                      "file_name"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_FILE_NAME_FIELD_TYPE           SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID                        "band_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_BAND_ID_FIELD_TYPE             SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID                 "raster_file_id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_TUPLEKEYS_RASTER_FILES_FIELD_RASTER_FILE_ID_FIELD_TYPE      SPATIALITE_FIELD_TYPE_INTEGER

// projects
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_TABLE_NAME                                         "projects"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_ID                                           "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_ID_FIELD_TYPE                                SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE                                         "code"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_CODE_FIELD_TYPE                              SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_RESULTS_PATH                                 "results_path"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_RESULTS_PATH_FIELD_TYPE                      SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE                                 "initial_date"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_INITIAL_DATE_FIELD_TYPE                      SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE                                   "final_date"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_FINAL_DATE_FIELD_TYPE                        SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_OUTPUT_SRID                                  "output_srid"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_OUTPUT_SRID_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM                                     "the_geom"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_PROJECTS_FIELD_THE_GEOM_FIELD_TYPE                          SPATIALITE_FIELD_TYPE_WKT_GEOMETRY


// TABLE_LANDSAT8_METADATA
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_TABLE_NAME                                "landsat8_metadata"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID                                  "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_ID_FIELD_TYPE                       SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE                       "metadata_file"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_METADATA_FILE_FIELD_TYPE            SPATIALITE_FIELD_TYPE_TEXT

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION                       "sun_elevation"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_TYPE            SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_ELEVATION_FIELD_PRECISION       8

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH                         "sun_azimuth"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_SUN_AZIMUTH_FIELD_PRECISION         8

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE                  "earth_sun_distance"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_TYPE       SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_EARTH_SUN_DISTANCE_FIELD_PRECISION  8

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_PREFIX                      "reflectance_add_band_"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_PREFIX                     "reflectance_mult_band_"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1                         "radiance_mult_band_1"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B1_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1                          "radiance_add_band_1"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B1_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2                         "radiance_mult_band_2"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B2_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2                          "radiance_add_band_2"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B2_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3                         "radiance_mult_band_3"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B3_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3                          "radiance_add_band_3"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B3_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4                         "radiance_mult_band_4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B4_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4                          "radiance_add_band_4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B4_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5                         "radiance_mult_band_5"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B5_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5                          "radiance_add_band_5"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B5_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6                         "radiance_mult_band_6"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B6_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6                          "radiance_add_band_6"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B6_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7                         "radiance_mult_band_7"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B7_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7                          "radiance_add_band_7"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B7_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8                         "radiance_mult_band_8"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B8_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8                          "radiance_add_band_8"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B8_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9                         "radiance_mult_band_9"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B9_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9                          "radiance_add_band_9"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B9_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10                        "radiance_mult_band_10"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10_FIELD_TYPE             SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B10_FIELD_PRECISION        15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10                         "radiance_add_band_10"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B10_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11                        "radiance_mult_band_11"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11_FIELD_TYPE             SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_MULT_B11_FIELD_PRECISION        15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11                         "radiance_add_band_11"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_RAD_ADD_B11_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1                         "reflectance_mult_band_1"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B1_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1                          "reflectance_add_band_1"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B1_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2                         "reflectance_mult_band_2"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B2_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2                          "reflectance_add_band_2"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B2_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3                         "reflectance_mult_band_3"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B3_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3                          "reflectance_add_band_3"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B3_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4                         "reflectance_mult_band_4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B4_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4                          "reflectance_add_band_4"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B4_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5                         "reflectance_mult_band_5"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B5_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5                          "reflectance_add_band_5"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B5_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6                         "reflectance_mult_band_6"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B6_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6                          "reflectance_add_band_6"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B6_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7                         "reflectance_mult_band_7"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B7_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7                          "reflectance_add_band_7"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B7_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8                         "reflectance_mult_band_8"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B8_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8                          "reflectance_add_band_8"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B8_FIELD_PRECISION          15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9                         "reflectance_mult_band_9"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9_FIELD_TYPE              SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_MULT_B9_FIELD_PRECISION         15

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9                          "reflectance_add_band_9"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9_FIELD_TYPE               SPATIALITE_FIELD_TYPE_DOUBLE
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_LANDSAT8_METADATA_FIELD_REF_ADD_B9_FIELD_PRECISION          15

// TABLE_SENTINEL2_METADATA
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_TABLE_NAME                               "sentinel2_metadata"

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID                                 "id"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_ID_FIELD_TYPE                      SPATIALITE_FIELD_TYPE_INTEGER

#define PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_METADATA_FILE                      "metadata_file"
#define PERSISTENCEMANAGER_SPATIALITE_TABLE_SENTINEL2_METADATA_FIELD_METADATA_FILE_FIELD_TYPE           SPATIALITE_FIELD_TYPE_TEXT


#endif // PERSISTENCEMANAGER_DEFINITIONS_H
