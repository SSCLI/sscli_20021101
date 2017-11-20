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
/****************************************************************
*
* Overview: CustomerDebugHelper implements the features of the
*           customer checked build by handling activation status,
*           and logs and reports.
*       
*
****************************************************************/


#include "common.h"
#include "utilcode.h"
#include "customerdebughelper.h"
#include "eeconfig.h"


// Implementation of the CustomerDebugHelper class


CustomerDebugHelper* CustomerDebugHelper::m_pCdh = NULL;

// Constructor
CustomerDebugHelper::CustomerDebugHelper()
{
    m_pCrst                  = ::new Crst("CustomerDebugHelper", CrstSingleUseLock);

    m_iNumberOfProbes        = CUSTOMERCHECKEDBUILD_NUMBER_OF_PROBES;
    m_iNumberOfEnabledProbes = 0;

    m_aProbeNames            = new LPCWSTR  [m_iNumberOfProbes];
    m_aProbeStatus           = new BOOL     [m_iNumberOfProbes];
    m_aProbeRelevantMethods  = new RelevantMethodsList  [m_iNumberOfProbes];
    m_aProbeParseMethods     = new EnumParseMethods     [m_iNumberOfProbes];

    // Adding a probe requires 3 changes:
    //   (1) Add the probe to EnumProbes (CustomerDebugHelper.h)
    //   (2) Add the probe name to m_aProbeNames[] (CustomerDebugHelper.cpp)
    //   (3) Add the probe to machine.config with activation status in developerSettings

    m_aProbeNames[CustomerCheckedBuildProbe_StackImbalance]     = L"CustomerCheckedBuildProbe_StackImbalance";
    m_aProbeNames[CustomerCheckedBuildProbe_CollectedDelegate]  = L"CustomerCheckedBuildProbe_CollectedDelegate";
    m_aProbeNames[CustomerCheckedBuildProbe_InvalidIUnknown]    = L"CustomerCheckedBuildProbe_InvalidIUnknown";
    m_aProbeNames[CustomerCheckedBuildProbe_InvalidVariant]     = L"CustomerCheckedBuildProbe_InvalidVariant";
    m_aProbeNames[CustomerCheckedBuildProbe_Marshaling]         = L"CustomerCheckedBuildProbe_Marshaling";

    // Set-up customized parse methods

    for (int i=0; i < m_iNumberOfProbes; i++) {
        m_aProbeParseMethods[i] = NO_PARSING;  // Default to no customization
    }

    // By default, all probes will not have any customized parsing to determine
    // activation.  A probe is either enabled or disabled indepedent of the calling
    // method.
    //
    // To specify a customized parsing method, set the parse method in 
    // m_aProbeParseMethods to one of the appropriate EnumParseMethods.  Then edit 
    // machine.config by setting attribute [probe-name].RelevantMethods to semicolon 
    // seperated values.

    m_aProbeParseMethods[CustomerCheckedBuildProbe_Marshaling] = METHOD_NAME_PARSE;

    static const WCHAR  strRelevantMethodsExtension[] = L".RelevantMethods";
    CQuickArray<WCHAR>  strProbeRelevantMethodsAttribute;
    strProbeRelevantMethodsAttribute.Alloc(0);

    for (int iProbe=0; iProbe < m_iNumberOfProbes; iProbe++) {

        // Get probe activation status from machine.config

        LPWSTR strProbeStatus = EEConfig::GetConfigString((LPWSTR)m_aProbeNames[iProbe]);

        if (strProbeStatus == NULL)
            m_aProbeStatus[iProbe] = false;
        else
        {
            m_aProbeStatus[iProbe] = (wcscmp( strProbeStatus, L"true" ) == 0);

            if (m_aProbeStatus[iProbe])
                m_iNumberOfEnabledProbes++;
        }

        // Get probe relevant methods from machine.config

        strProbeRelevantMethodsAttribute.ReSize( (UINT)wcslen(m_aProbeNames[iProbe]) + lengthof(strRelevantMethodsExtension) );
        Wszwsprintf( (LPWSTR)strProbeRelevantMethodsAttribute.Ptr(), L"%s%s", m_aProbeNames[iProbe], strRelevantMethodsExtension );

        LPWSTR strProbeRelevantMethods = EEConfig::GetConfigString((LPWSTR)strProbeRelevantMethodsAttribute.Ptr());
        
        m_aProbeRelevantMethods[iProbe].Init();        
        if (strProbeRelevantMethods != NULL)
        {
            // Populate array with parsed tokens

            LPWSTR strToken = wcstok(strProbeRelevantMethods, L";");

            while (strToken != NULL)
            {
                LPWSTR strParsedToken = new WCHAR[wcslen(strToken) + 1];
                wcscpy(strParsedToken, strToken);

                // Strip parenthesis
                if (wcschr(strParsedToken, '(') != NULL)
                    *wcschr(strParsedToken, '(') = NULL;

                m_aProbeRelevantMethods[iProbe].InsertHead(new RelevantMethod(strParsedToken));
                strToken = wcstok(NULL, L";");
            }

            delete [] strToken;
        }

        delete [] strProbeStatus;
        delete [] strProbeRelevantMethods;
    }
};


// Destructor
CustomerDebugHelper::~CustomerDebugHelper()
{
    for (int iProbe=0; iProbe < m_iNumberOfProbes; iProbe++)
    {
        delete [] m_aProbeNames[iProbe];

        RelevantMethod* temp;
        do
        {
            temp = m_aProbeRelevantMethods[iProbe].RemoveHead();
            delete temp;
        }
        while (!m_aProbeRelevantMethods[iProbe].IsEmpty());
    }
    
    delete [] m_aProbeNames;
    delete [] m_aProbeStatus;
    delete [] m_aProbeRelevantMethods;
    delete [] m_aProbeParseMethods;
};


// Return instance of CustomerDebugHelper to caller
CustomerDebugHelper* CustomerDebugHelper::GetCustomerDebugHelper()
{
    if (m_pCdh == NULL)
    {
        CustomerDebugHelper* pCdh = new CustomerDebugHelper();
        if (InterlockedCompareExchangePointer((void**)&m_pCdh, (void*)pCdh, NULL) != NULL)
            delete pCdh;
    }
    return m_pCdh;
}


// Destroy instance of CustomerDebugHelper
void CustomerDebugHelper::Terminate()
{
    _ASSERTE(m_pCdh != NULL);
    delete m_pCdh;
    m_pCdh = NULL;
}

// Log information from probe
void CustomerDebugHelper::LogInfo(LPCWSTR strMessage, EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    //_ASSERTE(m_aProbeStatus[ProbeID] || !"Attempted to use disabled probe");
    _ASSERTE(strMessage != NULL);

    static const WCHAR  strLog[] = L"Logged information from %s: %s\n";
    CQuickArray<WCHAR>  strOutput;

    strOutput.Alloc( lengthof(strLog) + wcslen(m_aProbeNames[ProbeID]) + wcslen(strMessage) );

    Wszwsprintf( (LPWSTR)strOutput.Ptr(), strLog, m_aProbeNames[ProbeID], strMessage );
    WszOutputDebugString( (LPCWSTR)strOutput.Ptr() );
};


// Report errors from probe
void CustomerDebugHelper::ReportError(LPCWSTR strMessage, EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    //_ASSERTE(m_aProbeStatus[ProbeID] || !"Attempted to use disabled probe");
    _ASSERTE(strMessage != NULL);

    static const WCHAR  strReport[] = L"Reported error from %s: %s\n";
    CQuickArray<WCHAR>  strOutput;

    strOutput.Alloc( lengthof(strReport) + wcslen(m_aProbeNames[ProbeID]) + wcslen(strMessage) );
    
    Wszwsprintf( (LPWSTR)strOutput.Ptr(), strReport, m_aProbeNames[ProbeID], strMessage );
    WszOutputDebugString( (LPCWSTR)strOutput.Ptr() );
};


// Activation of customer checked build
BOOL CustomerDebugHelper::IsEnabled()
{
    return (m_iNumberOfEnabledProbes != 0);
};


// Activation of specific probe
BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE( m_aProbeParseMethods[ProbeID] == NO_PARSING ||
              !"Probe needs more information -- use IsProbeEnabled(ProbeID, strEnabledFor)" );
    return m_aProbeStatus[ProbeID];
};


// Customized activation of specific probe

BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID, LPCWSTR strEnabledFor)
{
    return IsProbeEnabled(ProbeID, strEnabledFor, m_aProbeParseMethods[ProbeID]);
}

BOOL CustomerDebugHelper::IsProbeEnabled(EnumProbes ProbeID, LPCWSTR strEnabledFor, EnumParseMethods enCustomParse)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE((0 <= enCustomParse) && (enCustomParse < NUMBER_OF_PARSE_METHODS));
    _ASSERTE(strEnabledFor != NULL);

    if (m_aProbeStatus[ProbeID])
    {
        if (m_aProbeRelevantMethods[ProbeID].IsEmpty())
            return true;
        else
        {
            CQuickArray<WCHAR>  strNamespaceClassMethod;
            CQuickArray<WCHAR>  strNamespaceClass;
            CQuickArray<WCHAR>  strNamespace;
            CQuickArray<WCHAR>  strClassMethod;
            CQuickArray<WCHAR>  strClass;
            CQuickArray<WCHAR>  strMethod;
            
            CQuickArray<WCHAR>  strInput;
            CQuickArray<WCHAR>  strTemp;

            RelevantMethod*     relevantMethod;
            BOOL                bFound = false;
            UINT                iLengthOfEnabledFor = (UINT)wcslen(strEnabledFor) + 1;

            static const WCHAR  strClassMethodFormat[] = L"%s::%s";


            switch(enCustomParse)
            {
                case METHOD_NAME_PARSE:

                    strInput.Alloc(iLengthOfEnabledFor);
                    wcscpy(strInput.Ptr(), strEnabledFor);

                    // Strip parenthesis

                    if (wcschr(strInput.Ptr(), '('))
                        *wcschr(strInput.Ptr(), '(') = NULL;

                    // Obtain namespace, class, and method names

                    strNamespaceClassMethod.Alloc(iLengthOfEnabledFor);
                    strNamespaceClass.Alloc(iLengthOfEnabledFor);
                    strNamespace.Alloc(iLengthOfEnabledFor);
                    strClassMethod.Alloc(iLengthOfEnabledFor);
                    strClass.Alloc(iLengthOfEnabledFor);
                    strMethod.Alloc(iLengthOfEnabledFor);

                    strTemp.Alloc(iLengthOfEnabledFor);
                    wcscpy(strTemp.Ptr(), strInput.Ptr());

                    if (wcschr(strInput.Ptr(), ':') &&
                        wcschr(strInput.Ptr(), '.') )
                    {
                        // input format is Namespace.Class::Method
                        wcscpy(strNamespaceClassMethod.Ptr(), strInput.Ptr());
                        wcscpy(strNamespaceClass.Ptr(),  wcstok(strTemp.Ptr(), L":"));
                        wcscpy(strMethod.Ptr(), wcstok(NULL, L":"));

                        ns::SplitPath(strNamespaceClass.Ptr(), strNamespace.Ptr(), iLengthOfEnabledFor, strClass.Ptr(), iLengthOfEnabledFor);
                        Wszwsprintf(strClassMethod.Ptr(), strClassMethodFormat, strClass.Ptr(), strMethod.Ptr());
                    }
                    else if (wcschr(strInput.Ptr(), ':'))
                    {
                        // input format is Class::Method
                        wcscpy(strClass.Ptr(),  wcstok(strTemp.Ptr(), L":"));
                        wcscpy(strMethod.Ptr(), wcstok(NULL, L":"));
                        
                        Wszwsprintf(strClassMethod.Ptr(), strClassMethodFormat, strClass.Ptr(), strMethod.Ptr());
                        
                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strNamespaceClass.Ptr() = NULL;
                        *strNamespace.Ptr() = NULL;
                    }
                    else if (wcschr(strInput.Ptr(), '.'))
                    {
                        // input format is Namespace.Class
                        wcscpy(strNamespaceClass.Ptr(), strInput.Ptr());
                        ns::SplitPath(strNamespaceClass.Ptr(), strNamespace.Ptr(), iLengthOfEnabledFor, strClass.Ptr(), iLengthOfEnabledFor);

                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strClassMethod.Ptr() = NULL;
                        *strMethod.Ptr() = NULL;
                    }
                    else
                    {
                        // input has no separators -- assume Method
                        wcscpy(strMethod.Ptr(), strInput.Ptr());

                        *strNamespaceClassMethod.Ptr() = NULL;
                        *strNamespaceClass.Ptr() = NULL;
                        *strNamespace.Ptr() = NULL;
                        *strClassMethod.Ptr() = NULL;
                        *strClass.Ptr() = NULL;
                    }

                    // Compare namespace, class, and method names to m_aProbeRelevantMethods

                    // Take lock to prevent concurrency failure if m_aProbeRelevantMethods is modified
                    m_pCrst->Enter();

                    relevantMethod = m_aProbeRelevantMethods[ProbeID].GetHead();
                    while (relevantMethod != NULL)
                    {
                        if ( _wcsicmp(strNamespaceClassMethod.Ptr(), relevantMethod->Value()) == 0  || 
                             _wcsicmp(strNamespaceClass.Ptr(), relevantMethod->Value()) == 0        || 
                             _wcsicmp(strNamespace.Ptr(), relevantMethod->Value()) == 0             ||
                             _wcsicmp(strClassMethod.Ptr(), relevantMethod->Value()) == 0           ||
                             _wcsicmp(strClass.Ptr(), relevantMethod->Value()) == 0                 ||
                             _wcsicmp(strMethod.Ptr(), relevantMethod->Value()) == 0                )
                        {
                             bFound = true;
                             break;
                        }
                        else
                            relevantMethod = m_aProbeRelevantMethods[ProbeID].GetNext(relevantMethod);
                    }
                    m_pCrst->Leave();

                    return bFound;


                case NO_PARSING:
                    return IsProbeEnabled(ProbeID);


                case GENERIC_PARSE:
                default:

                    // Case-insensitive string matching

                    // Take lock to prevent concurrency failure if m_aProbeRelevantMethods is modified
                    m_pCrst->Enter();

                    relevantMethod = m_aProbeRelevantMethods[ProbeID].GetHead();
                    while (relevantMethod != NULL) 
                    {
                        if (_wcsicmp(strEnabledFor, relevantMethod->Value()) == 0)
                        {
                            bFound = true;
                            break;
                        }
                        else
                            relevantMethod = m_aProbeRelevantMethods[ProbeID].GetNext(relevantMethod);
                    }
                    m_pCrst->Leave();

                    return bFound;
            }
        }
    }
    else
        return false;
};


// Enable specific probe
BOOL CustomerDebugHelper::EnableProbe(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    if (!InterlockedExchange((LPLONG) &m_aProbeStatus[ProbeID], TRUE))
        InterlockedIncrement((LPLONG) &m_iNumberOfEnabledProbes);
    return true;
};


// Customized enabling for specific probe
// Note that calling a customized enable does not necessarily 
// mean the probe is actually enabled.
BOOL CustomerDebugHelper::EnableProbe(EnumProbes ProbeID, LPCWSTR strEnableFor)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE(strEnableFor != NULL);

    CQuickArray<WCHAR> strParsedEnable;
    strParsedEnable.Alloc((UINT)wcslen(strEnableFor) + 1);
    wcscpy(strParsedEnable.Ptr(), strEnableFor);

    // Strip parenthesis
    if (wcschr(strParsedEnable.Ptr(), '(') != NULL)
        *wcschr(strParsedEnable.Ptr(), '(') = NULL;

    BOOL bAlreadyExists = false;

    // Take lock to avoid concurrent read/write failures
    m_pCrst->Enter();

    RelevantMethod* relevantMethod = m_aProbeRelevantMethods[ProbeID].GetHead();
    while (relevantMethod != NULL)
    {
        if (_wcsicmp(strParsedEnable.Ptr(), relevantMethod->Value()) == 0)
            bAlreadyExists = true;
        relevantMethod = m_aProbeRelevantMethods[ProbeID].GetNext(relevantMethod);
    }
    if (!bAlreadyExists)
    {
        LPWSTR strNewEnable = new WCHAR[wcslen(strParsedEnable.Ptr()) + 1];
        wcscpy(strNewEnable, strParsedEnable.Ptr());
        m_aProbeRelevantMethods[ProbeID].InsertHead(new RelevantMethod(strNewEnable));
    }
    m_pCrst->Leave();

    return !bAlreadyExists;
}


// Disable specific probe
BOOL CustomerDebugHelper::DisableProbe(EnumProbes ProbeID)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    if (InterlockedExchange((LPLONG) &m_aProbeStatus[ProbeID], FALSE))
        InterlockedDecrement((LPLONG) &m_iNumberOfEnabledProbes);
    return true;
};


// Customized disabling for specific probe
BOOL CustomerDebugHelper::DisableProbe(EnumProbes ProbeID, LPCWSTR strDisableFor)
{
    _ASSERTE((0 <= ProbeID) && (ProbeID < m_iNumberOfProbes));
    _ASSERTE(strDisableFor != NULL);

    CQuickArray<WCHAR> strParsedDisable;
    strParsedDisable.Alloc((UINT)wcslen(strDisableFor) + 1);
    wcscpy(strParsedDisable.Ptr(), strDisableFor);

    // Strip parenthesis
    if (wcschr(strParsedDisable.Ptr(), '(') != NULL)
        *wcschr(strParsedDisable.Ptr(), '(') = NULL;

    // Take lock to avoid concurrent read/write failures
    m_pCrst->Enter();

    BOOL bRemovedProbe = false;
    RelevantMethod* relevantMethod = m_aProbeRelevantMethods[ProbeID].GetHead();
    while (relevantMethod != NULL)
    {
        if (_wcsicmp(strParsedDisable.Ptr(), relevantMethod->Value()) == 0)
        {
            relevantMethod = m_aProbeRelevantMethods[ProbeID].FindAndRemove(relevantMethod);
            delete relevantMethod;
            bRemovedProbe = true;
            break;
        }
        relevantMethod = m_aProbeRelevantMethods[ProbeID].GetNext(relevantMethod);
    }
    m_pCrst->Leave();

    return bRemovedProbe;
}
