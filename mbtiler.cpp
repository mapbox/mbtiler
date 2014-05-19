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
#include "cpl_vsil_curl_priv.h"

#include "zlib.h"
#include "json.h"

#include <math.h>

extern "C" void GDALRegister_MBTiles();

CPL_CVSID("$Id$");

static const char * const apszAllowedDrivers[] = {"JPEG", "PNG", NULL};

int main(int argc, char** argv) {

    return 0;
}

