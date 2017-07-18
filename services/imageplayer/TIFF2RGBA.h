/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 *  @author   Tellen Yu
 *  @version  1.0
 *  @date     2015/06/08
 *  @par function description:
 *  - 1 save rgb data to picture
 */

#ifndef TIFF_2_RGBA_H
#define TIFF_2_RGBA_H

#include <SkBitmap.h>

#define MAX_PIC_SIZE                8000

namespace android {

class TIFF2RGBA {
  public:
    TIFF2RGBA();
    ~TIFF2RGBA();

    static int tiffDecodeBound(const char *filePath, int *width, int *height);
    static int tiffDecoder(const char *filePath, SkBitmap *pBitmap);

};

}  // namespace android

#endif // TIFF_2_RGBA_H
