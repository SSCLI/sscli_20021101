// ==++==
//
//   
//    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//   
//    The use and distribution terms for this software are contained in the file
//    named license.txt, which can be found in the root of this distribution.
//    By using this software in any fashion, you are agreeing to be bound by the
//    terms of this license.
//   
//    You must not remove this notice, or any other, from this software.
//   
//
// ==--==
// ===========================================================================
// File: images.h
//
// This file defines indexes into the imagelist IMAGES.BMP
// ===========================================================================

#ifndef _IMAGES_H_
#define _IMAGES_H_

#define IMG_NONE                 -1
#define IMG_UNKNOWN              -1

// Images are determined by an IMG_* + IMG_ACC_* value.  There are IMG_ACC_TYPE_COUNT versions of each image.
#define IMG_ACC_TYPE_COUNT      6

#define IMG_CLASS               (IMG_ACC_TYPE_COUNT * 0)
#define IMG_CONSTANT            (IMG_ACC_TYPE_COUNT * 1)
#define IMG_DELEGATE            (IMG_ACC_TYPE_COUNT * 2)
#define IMG_ENUM                (IMG_ACC_TYPE_COUNT * 3)
#define IMG_ENUMMEMBER          (IMG_ACC_TYPE_COUNT * 4)
#define IMG_EVENT               (IMG_ACC_TYPE_COUNT * 5)
#define IMG_EXCEPTION           (IMG_ACC_TYPE_COUNT * 6)
#define IMG_FIELD               (IMG_ACC_TYPE_COUNT * 7)
#define IMG_INTERFACE           (IMG_ACC_TYPE_COUNT * 8)
#define IMG_macro               (IMG_ACC_TYPE_COUNT * 9)        // NOTE:  non-C# image...
#define IMG_map                 (IMG_ACC_TYPE_COUNT * 10)       // NOTE:  non-C# image...
#define IMG_mapitem             (IMG_ACC_TYPE_COUNT * 11)       // NOTE:  non-C# image...
#define IMG_METHOD              (IMG_ACC_TYPE_COUNT * 12)
#define IMG_overload            (IMG_ACC_TYPE_COUNT * 13)       // NOTE:  non-C# image...
#define IMG_module              (IMG_ACC_TYPE_COUNT * 14)       // NOTE:  non-C# image...
#define IMG_NAMESPACE           (IMG_ACC_TYPE_COUNT * 15)
#define IMG_OPERATOR            (IMG_ACC_TYPE_COUNT * 16)
#define IMG_PROPERTY            (IMG_ACC_TYPE_COUNT * 17)
#define IMG_STRUCT              (IMG_ACC_TYPE_COUNT * 18)
#define IMG_template            (IMG_ACC_TYPE_COUNT * 19)       // NOTE:  non-C# image...
#define IMG_typedef             (IMG_ACC_TYPE_COUNT * 20)       // NOTE:  non-C# image...
#define IMG_type                (IMG_ACC_TYPE_COUNT * 21)       // NOTE:  non-C# image..
#define IMG_union               (IMG_ACC_TYPE_COUNT * 22)       // NOTE:  non-C# image..
#define IMG_VARIABLE            (IMG_ACC_TYPE_COUNT * 23)
#define IMG_valuetype           (IMG_ACC_TYPE_COUNT * 24)       // NOTE:  non-C# image...

#define IMG_ERROR               (IMG_ACC_TYPE_COUNT * 25)       // Error glyph (NOTE:  Do not add IMG_ACC_ to this!!!)
#define IMG_ASSEMBLY		    IMG_ERROR + 6
#define IMG_CSCPROJECT          IMG_ERROR + 10

#define IMG_MAX_IMAGES          IMG_CSCPROJECT

#define IMG_ACC_PUBLIC          0
#define IMG_ACC_INTERNAL        1
#define IMG_ACC_friend          2                               // NOTE:  non-C# image...
#define IMG_ACC_PROTECTED       3
#define IMG_ACC_PRIVATE         4
#define IMG_ACC_shortcut        5

#define IMGLIST_WIDTH           16

#define IMGLIST_BACKGROUND      0x0000ff00

#endif  // _IMAGES_H_
