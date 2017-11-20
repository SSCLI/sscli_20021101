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
// PESectionMan implementation

#include "stdafx.h"

/*****************************************************************/
HRESULT PESectionMan::Init()
{
	const int initNumSections = 16;	
    sectStart = new PESection*[initNumSections];	
	if (!sectStart)
		return E_OUTOFMEMORY;
    sectCur = sectStart;
    sectEnd = &sectStart[initNumSections];

	return S_OK;
}

/*****************************************************************/
HRESULT PESectionMan::Cleanup()
{
	for (PESection** ptr = sectStart; ptr < sectCur; ptr++)
		delete *ptr;
    delete sectStart;

	return S_OK;
}

/*****************************************************************/
void PESectionMan::sectionDestroy(PESection **section)
{
	// check if this section is referenced in other sections' relocs
	for(PESection** ptr = sectStart; ptr < sectCur; ptr++)
	{
		if(ptr != section)
		{
		    for(PESectionReloc* cur = (*ptr)->m_relocStart; cur < (*ptr)->m_relocCur; cur++) 
			{
				if(cur->section == *section) //here it is! Delete the reference
				{
					for(PESectionReloc* tmp = cur; tmp < (*ptr)->m_relocCur; tmp++)
					{
						memcpy(tmp,(tmp+1),sizeof(PESectionReloc));
					}
					(*ptr)->m_relocCur--;
					cur--; // no position shift this time
				}
			}
		}
	}
	delete *section;
	*section = NULL;
}
/*****************************************************************/

/******************************************************************/
// Apply the relocs for all the sections
// Called by: ClassConverter after loading up during an in-memory conversion, 

void PESectionMan::applyRelocs(CeeGenTokenMapper *pTokenMapper)
{	
	// Cycle through each of the sections
	for(PESection ** ppCurSection = sectStart; ppCurSection < sectCur; ppCurSection++) {
		(*ppCurSection)->applyRelocs(pTokenMapper);
	} // End sections
}


/*****************************************************************/
PESection* PESectionMan::getSection(const char* name)
{
    int     len = (int)strlen(name);

    // the section name can be at most 8 characters including the null. 
    if (len < 8)
        len++;
    else 
        len = 8;

    // dbPrintf(("looking for section %s\n", name));
    for(PESection** cur = sectStart; cur < sectCur; cur++) {
		// dbPrintf(("searching section %s\n", (*cur)->m_ame));
		if (strncmp((*cur)->m_name, name, len) == 0) {
			// dbPrintf(("found section %s\n", (*cur)->m_name));
			return(*cur);
		}
	}
    return(0);
}

/******************************************************************/
HRESULT PESectionMan::getSectionCreate(const char* name, unsigned flags, 
													PESection **section)
{	
    PESection* ret = getSection(name);
	if (ret == NULL) 
		return(newSection(name, section, flags));
	*section = ret;
	return(S_OK);
}

/******************************************************************/
HRESULT PESectionMan::newSection(const char* name, PESection **section, 
						unsigned flags, unsigned estSize, unsigned estRelocs)
{
    if (sectCur >= sectEnd) {
		unsigned curLen = (unsigned)(sectCur-sectStart);
		unsigned newLen = curLen * 2 + 1;
		PESection** sectNew = new PESection*[newLen];
		TESTANDRETURN(sectNew, E_OUTOFMEMORY);
		memcpy(sectNew, sectStart, sizeof(PESection*)*curLen);
		delete sectStart;
		sectStart = sectNew;
		sectCur = &sectStart[curLen];
		sectEnd = &sectStart[newLen];
	}

    PESection* ret = new PESection(name, flags, estSize, estRelocs);
    TESTANDRETURN(ret, E_OUTOFMEMORY);
	// dbPrintf(("MAKING NEW %s SECTION data starts at 0x%x\n", name, ret->dataStart));
    *sectCur++ = ret;
	_ASSERTE(sectCur <= sectEnd);
	*section = ret;
    return(S_OK);
}


//Clone each of our sections.  This will cause a deep copy of the sections
HRESULT PESectionMan::cloneInstance(PESectionMan *destination) {
    _ASSERTE(destination);
    PESection       *pSection;
    PESection       **destPtr;
    HRESULT         hr = NOERROR;

    //Copy each of the sections
    for (PESection** ptr = sectStart; ptr < sectCur; ptr++) {
        destPtr = destination->sectStart;
        pSection = NULL;

        // try to find the matching section by name
        for (; destPtr < destination->sectCur; destPtr++)
        {
            if (strcmp((*destPtr)->m_name, (*ptr)->m_name) == 0)
            {
                pSection = *destPtr;
                break;
            }
        }
        if (destPtr >= destination->sectCur)
        {
            // cannot find a section in the destination with matching name
            // so create one!
            IfFailRet( destination->getSectionCreate((*ptr)->m_name,
		                                        (*ptr)->flags(), 
		                                        &pSection) );
        }
        if (pSection)
            IfFailRet( (*ptr)->cloneInstance(pSection) );
    }
    
    //destination->sectEnd=destination->sectStart + (sectEnd-sectStart);
    return S_OK;
}


//*****************************************************************************
// Implementation for PESection
//*****************************************************************************
/******************************************************************/
PESection::PESection(const char *name, unsigned flags, 
								 unsigned estSize, unsigned estRelocs) {


	dirEntry = -1;

    // No init needed for CBlobFectcher m_pIndex

    m_relocStart = new PESectionReloc[estRelocs];
	_ASSERTE(m_relocStart != NULL);

    m_relocCur =  m_relocStart;
    m_relocEnd = &m_relocStart[estRelocs];
    m_header = NULL;
	m_baseRVA = 0;
	m_filePos = 0;
	m_filePad = 0;
	m_flags = flags;

	_ASSERTE(strlen(name)<sizeof(m_name));
	strncpy(m_name, name, sizeof(m_name));
}


/******************************************************************/
PESection::~PESection() {
    delete m_relocStart;
}


/******************************************************************/
void PESection::writeSectReloc(unsigned val, CeeSection& relativeTo, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra) {

	addSectReloc(dataLen(), relativeTo, reloc, extra);
	unsigned* ptr = (unsigned*) getBlock(4);
	*ptr = val;
}

/******************************************************************/
HRESULT PESection::addSectReloc(unsigned offset, CeeSection& relativeToIn, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra) 
{
	return addSectReloc(
		offset, (PESection *)&relativeToIn.getImpl(), reloc, extra); 
}

/******************************************************************/
HRESULT PESection::addSectReloc(unsigned offset, PESection *relativeTo, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra) {

	/* dbPrintf(("******** GOT a section reloc for section %s offset 0x%x to section %x offset 0x%x\n",
		   header->m_name, offset, relativeTo->m_name, *((unsigned*) dataStart + offset))); */
	_ASSERTE(offset < dataLen());

    if (m_relocCur >= m_relocEnd)  {
		unsigned curLen = (unsigned)(m_relocCur-m_relocStart);
		unsigned newLen = curLen * 2 + 1;
		PESectionReloc* relocNew = new PESectionReloc[newLen];
        TESTANDRETURNMEMORY(relocNew);

		memcpy(relocNew, m_relocStart, sizeof(PESectionReloc)*curLen);
		delete m_relocStart;
		m_relocStart = relocNew;
		m_relocCur = &m_relocStart[curLen];
		m_relocEnd = &m_relocStart[newLen];
	}
	
    m_relocCur->type = reloc;
    m_relocCur->offset = offset;
    m_relocCur->section = relativeTo;
	if (extra)
		m_relocCur->extra = *extra;
	m_relocCur++;
	assert(m_relocCur <= m_relocEnd);
	return S_OK;
}

/******************************************************************/
// Compute a pointer (wrap blobfetcher)
char * PESection::computePointer(unsigned offset) const // virtual
{
	return m_blobFetcher.ComputePointer(offset);
}

/******************************************************************/
BOOL PESection::containsPointer(char *ptr) const // virtual
{
	return m_blobFetcher.ContainsPointer(ptr);
}

/******************************************************************/
// Compute an offset (wrap blobfetcher)
unsigned PESection::computeOffset(char *ptr) const // virtual
{
	return m_blobFetcher.ComputeOffset(ptr);
}


/******************************************************************/
HRESULT PESection::addBaseReloc(unsigned offset, CeeSectionRelocType reloc, CeeSectionRelocExtra *extra)
{
	// peSectionBase is a dummy section that causes any offsets to	
	// be relative to the PE base
	// can't be static because of initialization of local static 
	// objects is not thread-safe
	/* static */ PESection peSectionBase("BASE", 0, 0, 0);
	return addSectReloc(offset, &peSectionBase, reloc, extra);
}

/******************************************************************/
// Dynamic mem allocation, but we can't move old blocks (since others
// have pointers to them), so we need a fancy way to grow
char* PESection::getBlock(unsigned len, unsigned align)
{
	return m_blobFetcher.MakeNewBlock(len, align);
}

unsigned PESection::dataLen()
{
	return m_blobFetcher.GetDataLen();
}

// Apply all the relocs for in memory conversion


void PESection::applyRelocs(CeeGenTokenMapper *pTokenMapper)
{
	// For each section, go through each of its relocs
	for(PESectionReloc* pCurReloc = m_relocStart; pCurReloc < m_relocCur; pCurReloc++) {

		if (pCurReloc->type == srRelocMapToken) {
			unsigned * pos = (unsigned*) 
			  m_blobFetcher.ComputePointer(pCurReloc->offset);
			mdToken newToken;
			if (pTokenMapper->HasTokenMoved(*pos, newToken)) {
				// we have a mapped token
				*pos = newToken;
			}
		}


	} // End relocs
}		

HRESULT PESection::cloneInstance(PESection *destination) {
    PESectionReloc *cur;
    INT32 newSize;
    HRESULT hr = NOERROR;

    _ASSERTE(destination);

    destination->dirEntry = dirEntry;

    //Merge the information currently in the BlobFetcher into 
    //out current blob fetcher
    m_blobFetcher.Merge(&(destination->m_blobFetcher));

    //Copy the name.
	strncpy(destination->m_name, m_name, sizeof(m_name));

    //Clone the relocs
    //If the arrays aren't the same size, reallocate as necessary.
    
    newSize = (INT32)(m_relocCur-m_relocStart);

    if (newSize>(destination->m_relocEnd - destination->m_relocStart)) {
        delete destination->m_relocStart;

        destination->m_relocStart = new PESectionReloc[newSize];
        _ASSERTE(destination->m_relocStart != NULL);
        if (destination->m_relocStart == NULL)
            IfFailGo( E_OUTOFMEMORY );
        destination->m_relocEnd = destination->m_relocStart+(newSize);
    }

    //copy the correct data over into our new array.
    memcpy(destination->m_relocStart, m_relocStart, sizeof(PESectionReloc)*(newSize));
    destination->m_relocCur = destination->m_relocStart + (newSize);
    for (cur=destination->m_relocStart; cur<destination->m_relocCur; cur++) {
        cur->section=destination;
    }
ErrExit:
    return hr;
}
