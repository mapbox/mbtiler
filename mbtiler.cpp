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

#include "cpl_conv.h"
#include "cpl_string.h"
#include "sqlite3.h"

#include <stdlib.h>

#include <iostream>
#include <stdexcept>

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
        void InsertImage(
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

    std::string basename(pszName);
    std::string extension(".mbtiles");
    std::string filename = basename + extension;

    // Initialize database and create tables.
    // TODO: check return values of sqlite3 functions.
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

void MBTiler::InsertImage(
        const char *pszFilename,
        int zoom,
        int tile_column,
        int tile_row)
{
    int rc;
    FILE *fp = NULL;
    unsigned char *buffer = NULL;
    size_t file_size;
    size_t result;

    // Get the file contents.
    fp = fopen(pszFilename, "rb");
    if (fp == NULL) {
        throw std::runtime_error(
                "NULL result from GDALOpen() of first input image");
    }

    // Determine the file and blob size.
    fseek(fp, 0, SEEK_END);
    file_size = static_cast<size_t>(ftell(fp));
    rewind(fp);

    buffer = (unsigned char *) malloc(sizeof(unsigned char)*file_size);
    if (buffer == NULL) {
        throw std::runtime_error(
                "NULL buffer");
    }
    result = fread(buffer, 1, file_size, fp);
    if (result != file_size) {
        free(buffer);
        throw std::runtime_error(
                "NULL result from GDALOpen() of first input image");
    }
    fclose(fp);

    rc = sqlite3_reset(tile_stmt);
    rc = sqlite3_bind_int(tile_stmt, 1, zoom);
    rc = sqlite3_bind_int(tile_stmt, 2, tile_column);
    rc = sqlite3_bind_int(tile_stmt, 3, tile_row);
    rc = sqlite3_bind_blob(
            tile_stmt, 4, buffer, static_cast<int>(file_size), SQLITE_STATIC);
    rc = sqlite3_step(tile_stmt);

    free(buffer);
}


int main(int argc, char** argv) {

    if (argc < 3) {
        std::clog << "Usage: expects two arguments: <filename> <description>\n";
        return 1;
    }
    try {
        MBTiler tiler(argv[1], argv[2]);

        // Write one tile.
        tiler.InsertImage(
            "tests/data/15/16979/21196.png",
            15, 16979, 21196);
    }
    catch (std::exception const& ex)
    {
        std::clog << "Fail! tile not written: '" << ex.what() << "'\n";
        return 1;
    }
    return 0;
}
