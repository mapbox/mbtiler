/*****************************************************************************
 * MBTiler
 *
 * Writes directories of suitably tiled PNGs or JPEGS to a SQLite file
 * using the SQLite C API.
 *
 * Inspired by GDAL's MBTiles driver, this module has no dependencies
 * not already provided by GDAL and OGR.
 *
 * Author: Sean Gillies <sean@mapbox.com>
 *
 ****************************************************************************/

#include "gdal_priv.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "stdlib.h"
#include "unistd.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "gdal_frmts.h"
#include "gdal_pam.h"
#include "ogr_api.h"
#include "sqlite3.h"
#include "cpl_vsil_curl_priv.h"

#include "zlib.h"

#include <math.h>

CPL_CVSID("$Id$");

static const char * const apszAllowedDrivers[] = {"JPEG", "PNG", NULL};

class MBTiler
{
    char *name;
    char *description;
    sqlite3 *db;
    sqlite3_stmt *meta_stmt;
    sqlite3_stmt *tile_stmt;
    
    public:
        MBTiler(const char *pszName, const char *pszDescription);
        ~MBTiler();
        int InsertImage(
            const char *pszFilename,
            int zoom,
            int tile_column,
            int tile_row);
};

MBTiler::MBTiler(const char *pszName, const char *pszDescription)
{
    int rc;
    
    name = CPLStrdup(pszName);
    description = CPLStrdup(pszDescription);

    // Initialize database and create tables.
    std::string basename(pszName);
    std::string extension(".mbtiles");
    std::string filename = basename + extension;
    rc = sqlite3_open(filename.c_str(), &db);
    rc = sqlite3_exec(
            db, 
            "CREATE TABLE metadata (name text, value text);", 
            NULL, NULL, NULL);
    rc = sqlite3_exec(
            db, 
            "CREATE TABLE tiles "
            "(zoom_level integer, tile_column integer, "
            "tile_row integer, tile_data blob);",
            NULL, NULL, NULL);

    // Write MBTiles metadata.
    rc = sqlite3_prepare_v2(
            db,
            "INSERT INTO metadata "
            "(name, value) "
            "VALUES (?1, ?2);",
            -1, &meta_stmt, NULL );
    rc = sqlite3_bind_text(meta_stmt, 1, "name", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, name, -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "type", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "overlay", -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "version", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "1.0.0", -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "description", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(
            meta_stmt, 2, description, -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "format", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "png", -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);

    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "minzoom", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "15", -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);

    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "maxzoom", -1, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "15", -1, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);

    rc = sqlite3_finalize(meta_stmt);

    // Prepare the tile insert statement.
    rc = sqlite3_prepare_v2(
            db,
            "INSERT INTO tiles "
            "(zoom_level, tile_column, tile_row, tile_data) "
            "VALUES (?1, ?2, ?3, ?4);",
            -1, &tile_stmt, NULL );
}

MBTiler::~MBTiler()
{
    if (tile_stmt != NULL) {
        sqlite3_finalize(tile_stmt);
    }
    if (db != NULL) {
        sqlite3_close(db);
    }
    CPLFree(name);
    CPLFree(description);
}

int MBTiler::InsertImage(
        const char *pszFilename,
        int zoom,
        int tile_column,
        int tile_row)
{
    int rc;
    FILE *png;
    unsigned char *buffer;
    size_t file_size;
    size_t n=65536;
    size_t result;

    // Get the file contents.
    png = fopen(pszFilename, "rb");
    if (png == NULL) {
        std::clog << "Failed to open file";
        return -1;
    }

    // obtain file size:
    fseek(png, 0, SEEK_END);
    file_size = ftell(png);
    rewind(png);

    buffer = (unsigned char *) malloc(sizeof(unsigned char)*file_size);
    result = fread(buffer, 1, file_size, png);
    if (result != file_size) {
        std::clog << "Unexpected number of bytes read: " << result << "\n";
        return -1;
    }
    fclose(png);

    rc = sqlite3_reset(tile_stmt);

    // Bind values.
    rc = sqlite3_bind_int(tile_stmt, 1, zoom);
    rc = sqlite3_bind_int(tile_stmt, 2, tile_column);
    rc = sqlite3_bind_int(tile_stmt, 3, tile_row);
    rc = sqlite3_bind_blob(tile_stmt, 4, buffer, file_size, SQLITE_STATIC);

    // Execute and finalize.
    rc = sqlite3_step(tile_stmt);
    
    return TRUE;
}


int main(int argc, char** argv) {
    
    int retval;

    try {
        MBTiler tiler(argv[1], argv[2]);

        // Write one tile.
        if (TRUE != tiler.InsertImage(
                        "tests/data/15/16979/21196.png", 
                        15, 16979, 11571))
        {
            throw std::runtime_error(
                "NULL result from GDALOpen() of first input image");
        }
    }
    catch (std::exception const& ex)
    {
        std::clog << "Fail! tile not written: '" << ex.what() << "'\n";
        return 1;
    }
    return 0;
}


