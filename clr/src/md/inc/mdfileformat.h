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
//*****************************************************************************
// MDFileFormat.h
//
// This file contains a set of helpers to verify and read the file format.
// This code does not handle the paging of the data, or different types of
// I/O.  See the StgTiggerStorage and StgIO code for this level of support.
//
//*****************************************************************************
#ifndef __MDFileFormat_h__
#define __MDFileFormat_h__

//*****************************************************************************
// The signature ULONG is the first 4 bytes of the file format.  The second
// signature string starts the header containing the stream list.  It is used
// for an integrity check when reading the header in lieu of a more complicated
// system.
//*****************************************************************************
#define STORAGE_MAGIC_SIG   0x424A5342  // BSJB



//*****************************************************************************
// These values get written to the signature at the front of the file.  Changing
// these values should not be done lightly because all old files will no longer
// be supported.  In a future revision if a format change is required, a
// backwards compatible migration path must be provided.
//*****************************************************************************

#define FILE_VER_MAJOR  1
#define FILE_VER_MINOR  1



#define MAXSTREAMNAME   32

enum
{
    STGHDR_NORMAL           = 0x00,     // Normal default flags.
    STGHDR_EXTRADATA        = 0x01,     // Additional data exists after header.
};


//*****************************************************************************
// This is the formal signature area at the front of the file. This structure
// is not allowed to change, the shim depends on it staying the same size.
// Use the reserved pointer if it must extended.
//*****************************************************************************
struct STORAGESIGNATURE
{
public:
    ULONG Signature() { return VAL32(m_lSignature);}
    void SetSignature(ULONG Signature) { m_lSignature = VAL32(Signature);}
    USHORT MajorVer() { return VAL16(m_iMajorVer);}
    void SetMajorVer(USHORT iMajorVer) { m_iMajorVer = VAL16(iMajorVer); }
    USHORT MinorVer() { return VAL16(m_iMinorVer);}
    void SetMinorVer(USHORT iMinorVer) { m_iMinorVer = VAL16(iMinorVer); }
    ULONG ExtraDataOffset() { return VAL32(m_lExtraData);}
    void SetExtraDataOffset(USHORT ExtraDataOffset) { m_lExtraData = VAL32(ExtraDataOffset); }
    ULONG VersionStringLength() { return VAL32(m_lVersionStringLength);}
    void SetVersionStringLength(ULONG VersionStringLength) { m_lVersionStringLength = VAL32(VersionStringLength); }
private:
    ULONG       m_lSignature;             // "Magic" signature.
    USHORT      m_iMajorVer;              // Major file version.
    USHORT      m_iMinorVer;              // Minor file version.
    ULONG       m_lExtraData;             // Offset to next structure of information 
    ULONG       m_lVersionStringLength;   // Length of version string
    BYTE        m_pVersion[0];            // Version string
};


//*****************************************************************************
// The header of the storage format.
//*****************************************************************************
struct STORAGEHEADER
{
    BYTE        fFlags;                 // STGHDR_xxx flags.
    BYTE        pad;
    USHORT      iStreams;               // How many streams are there.
    USHORT GetiStreams()
    {
        return VAL16(iStreams);
    }
    void SetiStreams(USHORT iStreamsCount)
    {
        iStreams = VAL16(iStreamsCount);
    }
};


//*****************************************************************************
// Each stream is described by this struct, which includes the offset and size
// of the data.  The name is stored in ANSI null terminated.
//*****************************************************************************
struct STORAGESTREAM
{
    ULONG       ulOffset;               // Offset in file for this stream.
    ULONG       ulSize;                 // Size of the file.
    char        rcName[MAXSTREAMNAME];  // Start of name, null terminated.

    inline STORAGESTREAM *NextStream()
    {
        int         iLen = (int)(strlen(rcName) + 1);
        iLen = ALIGN4BYTE(iLen);
        return ((STORAGESTREAM *) (((BYTE*)this) + (sizeof(ULONG) * 2) + iLen));
    }

    inline ULONG GetStreamSize()
    {
        return (ULONG)(strlen(rcName) + 1 + (sizeof(STORAGESTREAM) - sizeof(rcName)));
    }

    inline LPCWSTR GetName(LPWSTR szName, int iMaxSize)
    {
        VERIFY(::WszMultiByteToWideChar(CP_ACP, 0, rcName, -1, szName, iMaxSize));
        return (szName);
    }
    void SetSize(ULONG Size)
    {
        ulSize = VAL32(Size);
    }
    void SetOffset(ULONG Offset)
    {
        ulOffset = VAL32(Offset);
    }
    ULONG GetSize()
    {
        return VAL32(ulSize);
    }
    ULONG GetOffset()
    {
        return VAL32(ulOffset);
    }
};


class MDFormat
{
public:
//*****************************************************************************
// Verify the signature at the front of the file to see what type it is.
//*****************************************************************************
    static HRESULT VerifySignature(
        STORAGESIGNATURE *pSig);                // The signature to check.

//*****************************************************************************
// Skip over the header and find the actual stream data.
//*****************************************************************************
    static STORAGESTREAM *MDFormat::GetFirstStream(// Return pointer to the first stream.
        STORAGEHEADER *pHeader,             // Return copy of header struct.
        const void *pvMd);                  // Pointer to the full file.

};

#endif // __MDFileFormat_h__
