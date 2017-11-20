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
#ifndef PEWriter_H
#define PEWriter_H

#include <crtwrap.h>

#include <windows.h>

#include "ceegen.h"

#include "pesectionman.h"

class PEWriter;
class PEWriterSection;

struct _IMAGE_SECTION_HEADER;

#define SUBSECTION_ALIGN 16
/***************************************************************/
// PEWriter is derived from PESectionManager. While the base class just
// manages the sections, PEWriter can actually write them out.

class PEWriter : public PESectionMan
{
public:

    HRESULT Init(PESectionMan *pFrom);
    HRESULT Cleanup();

	HRESULT link();
	HRESULT fixup(CeeGenTokenMapper *pMapper);
    HRESULT write(const LPWSTR fileName);
	HRESULT write(void **ppImage);

	// calling these functions is optinal
    void setImageBase(size_t);
    void setFileAlignment(ULONG);
    void stripRelocations(bool val);        // default = false

    // these affect the charactertics in the NT file header
	void setCharacteristics(unsigned mask);
	void clearCharacteristics(unsigned mask);

    // these affect the charactertics in the NT optional header
	void setDllCharacteristics(unsigned mask);
	void clearDllCharacteristics(unsigned mask);

	// sets the SubSystem field in the optional header
    void setSubsystem(unsigned subsystem, unsigned major, unsigned minor);

    // specify the entry point as an offset into the text section. The
	// method will convert into an RVA from the base
	void setEntryPointTextOffset(unsigned entryPoint);

	HRESULT setDirectoryEntry(PEWriterSection *section, ULONG entry, ULONG size, ULONG offset=0);

    int fileAlignment();

	// get the RVA for the IL section
	ULONG getIlRva();

	// set the RVA for the IL section by supplying offset to the IL section
	void setIlRva(DWORD offset);

	unsigned getSubsystem();

	size_t getImageBase();

    void setEnCRvaBase(ULONG dataBase, ULONG rdataBase);

    HRESULT getFileTimeStamp(time_t *pTimeStamp);

private:
	DWORD  m_ilRVA;
    BOOL   m_encMode;
    ULONG  m_dataRvaBase;
    ULONG  m_rdataRvaBase;
    time_t m_peFileTimeStamp;

    HANDLE   m_file;

	PEWriterSection **getSectStart() {
		return (PEWriterSection**)sectStart;
	}
	PEWriterSection **getSectCur() {
		return (PEWriterSection**)sectCur;
	}
	void setSectCur(PEWriterSection **newCur) {
		sectCur = (PESection**)newCur;
	}

    IMAGE_NT_HEADERS32* ntHeaders;

	IMAGE_DOS_HEADER dosHeader;
	unsigned virtualPos;
	unsigned filePos;
	PEWriterSection *reloc;
	PEWriterSection *strtab;

	IMAGE_SECTION_HEADER *headers, *headersEnd;

	struct directoryEntry {
		PEWriterSection *section;
		ULONG offset;
		ULONG size;
	}  *pEntries;
    USHORT cEntries;

    HRESULT Open(LPCWSTR fileName);
    HRESULT Write(const void *data, long size);
    HRESULT Seek(long offset);
    HRESULT Pad(long align);
    HRESULT Close();
};

// This class encapsulates emitting the base reloc section
class PERelocSection
{
 private:
    PEWriterSection *section;
    unsigned relocPage;
    unsigned relocSize;
    DWORD *relocSizeAddr;
    unsigned pages;

#ifdef _DEBUG
    unsigned lastRVA;
#endif

 public:
    PERelocSection(PEWriterSection *pBaseReloc);
    void AddBaseReloc(unsigned rva, int type, unsigned short highAdj);
    void Finish();
};

// don't add any new virtual methods or fields to this class. We cast a PESection object
// to a PEWriterSection object to get the right applyRelocs method, but the object is created
// as a PESection type.
class PEWriterSection : public PESection {
	friend class PEWriter;
    PEWriterSection(const char* name, unsigned flags, 
		unsigned estSize, unsigned estRelocs) : PESection(name, flags, estSize, estRelocs) {
	}
    void applyRelocs(size_t imageBase, PERelocSection* relocSection, CeeGenTokenMapper *pTokenMapper,
                     DWORD rdataRvaBase, DWORD dataRvaBase);
	HRESULT write(HANDLE file);
    unsigned writeMem(void ** pMem);

};

inline int PEWriter::fileAlignment() {
    return VAL32(ntHeaders->OptionalHeader.FileAlignment);
}

inline void PEWriter::setImageBase(size_t imageBase) {
    ntHeaders->OptionalHeader.ImageBase = VAL32((DWORD)imageBase);
}

inline void PEWriter::setFileAlignment(ULONG fileAlignment) {
    ntHeaders->OptionalHeader.FileAlignment = VAL32((DWORD)fileAlignment);
}

inline size_t PEWriter::getImageBase() {
    return VAL32(ntHeaders->OptionalHeader.ImageBase);
}

inline void PEWriter::setSubsystem(unsigned subsystem, unsigned major, unsigned minor) {
    ntHeaders->OptionalHeader.Subsystem = VAL16(subsystem);
    ntHeaders->OptionalHeader.MajorSubsystemVersion = VAL16(major);
    ntHeaders->OptionalHeader.MinorSubsystemVersion = VAL16(minor);
}

inline void PEWriter::setCharacteristics(unsigned mask) {
    ntHeaders->FileHeader.Characteristics |= VAL16(mask);
}

inline void PEWriter::clearCharacteristics(unsigned mask) {
    ntHeaders->FileHeader.Characteristics &= VAL16(~mask);
}

inline void PEWriter::setDllCharacteristics(unsigned mask) {
	ntHeaders->OptionalHeader.DllCharacteristics |= VAL16(mask); 
}

inline void PEWriter::clearDllCharacteristics(unsigned mask) {
	ntHeaders->OptionalHeader.DllCharacteristics &= VAL16(~mask); 
}

inline void PEWriter::setEntryPointTextOffset(unsigned offset) {
    ntHeaders->OptionalHeader.AddressOfEntryPoint = VAL32(offset);
}

inline unsigned PEWriter::getSubsystem() {
    return VAL16(ntHeaders->OptionalHeader.Subsystem);
}

#endif
