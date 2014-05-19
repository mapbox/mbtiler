/*****************************************************************************
 * MBTiler
 *
 * Writes directories of suitably tiled PNGs or JPEGS to a SQLite file
 * using the SQLite C API.
 *
 * Inspired by and modeled after GDAL's MBTiles driver.
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
            "CREATE TABLE metadata (name text, value text)", 
            NULL, NULL, NULL);
    rc = sqlite3_exec(
            db, 
            "CREATE TABLE tiles "
            "(zoom_level, tile_column, tile_row, tile_data)",
            NULL, NULL, NULL);

    // Write MBTiles metadata.
    rc = sqlite3_prepare_v2(
            db,
            "INSERT INTO metadata "
            "(name, value) "
            "VALUES (?1, ?2)",
            -1, &meta_stmt, NULL );
    rc = sqlite3_bind_text(meta_stmt, 1, "name", 4, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, name, strlen(name), SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "type", 4, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "overlay", 7, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "version", 7, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "1.1", 3, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "description", 11, SQLITE_STATIC);
    rc = sqlite3_bind_text(
            meta_stmt, 2, description, strlen(description), SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_reset(meta_stmt);
    rc = sqlite3_bind_text(meta_stmt, 1, "format", 6, SQLITE_STATIC);
    rc = sqlite3_bind_text(meta_stmt, 2, "png", 3, SQLITE_STATIC);
    rc = sqlite3_step(meta_stmt);
    rc = sqlite3_finalize(meta_stmt);

    // Prepare the tile insert statement.
    rc = sqlite3_prepare_v2(
            db,
            "INSERT INTO tiles "
            "(zoom_level, tile_column, tile_row, tile_data) "
            "VALUES (?, ?, ?, ?)",
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


/*int MBTiler::InsertImage(
        const char *pszFilename,
        int zoom,
        int tile_column,
        int tile_row)
{
    int rc;

    // Bind values.
    rc = sqlite3_bind_int(tile_stmt, 1, &zoom);
    rc = sqlite3_bind_int(tile_stmt, 2, &tile_column);
    rc = sqlite3_bind_int(tile_stmt, 3, &tile_row);
    rc = sqlite3_bind_blob(tile_stmt, 4, buffer, n, SQLITE_STATIC);

    // Execute and finalize.
    rc = sqlite3_step(tile_stmt);
    
    return TRUE;
}*/


int main(int argc, char** argv) {
    MBTiler tiler("test", "A testing dataset");
    return 0;
}

