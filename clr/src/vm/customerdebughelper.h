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


#ifndef _CUSTOMERDEBUGHELPER_
#define _CUSTOMERDEBUGHELPER_



// Enumeration of probes (ProbeID)
enum EnumProbes
{
    // Adding a probe requires 3 changes:
    //   (1) Add the probe to EnumProbes (CustomerDebugHelper.h)
    //   (2) Add the probe name to m_aProbeNames[] (CustomerDebugHelper.cpp)
    //   (3) Add the probe to machine.config with activation status in developerSettings

    CustomerCheckedBuildProbe_StackImbalance = 0,
    CustomerCheckedBuildProbe_CollectedDelegate,
    CustomerCheckedBuildProbe_InvalidIUnknown,
    CustomerCheckedBuildProbe_InvalidVariant,
    CustomerCheckedBuildProbe_Marshaling,
    CUSTOMERCHECKEDBUILD_NUMBER_OF_PROBES
};



// Enumeration of parsing methods for customized probe enabling
enum EnumParseMethods
{
    // By default, all probes will not have any customized parsing to determine
    // activation.  A probe is either enabled or disabled indepedent of the calling
    // method.
    //
    // To specify a customized parsing method, set the parse method in 
    // m_aProbeParseMethods to one of the appropriate EnumParseMethods.  Then edit 
    // machine.config by setting attribute [probe-name].RelevantMethods to semicolon 
    // seperated values.

    NO_PARSING = 0,
    GENERIC_PARSE,
    METHOD_NAME_PARSE,
    NUMBER_OF_PARSE_METHODS
};



// RelevantMethod type for list of methods relevant to a specific probe
// This allows for customized activation/deactivation of the customer checked
// build probe on different methods.

class RelevantMethod
{

public:

    SLink   m_link;

    RelevantMethod(LPCWSTR strRelevantMethod)
    {
        m_str = strRelevantMethod;
    }

    ~RelevantMethod()
    {
        delete [] m_str;
    }

    LPCWSTR Value()
    {
        return m_str;
    }

private:

    LPCWSTR m_str;
};

typedef SList<RelevantMethod, offsetof(RelevantMethod, m_link), true> RelevantMethodsList;




// Mechanism for handling Customer Checked Build functionality
class CustomerDebugHelper
{

public:

    // Constructor and destructor
    CustomerDebugHelper();
    ~CustomerDebugHelper();

    // Return and destroy instance of CustomerDebugHelper
    static CustomerDebugHelper* GetCustomerDebugHelper();
    static void Terminate();

    // Methods used to log/report
    void        LogInfo (LPCWSTR strMessage, EnumProbes ProbeID);
    void        ReportError (LPCWSTR strMessage, EnumProbes ProbeID);

    // Activation of customer-checked-build
    BOOL        IsEnabled();

    // Activation of specific probes
    BOOL        IsProbeEnabled  (EnumProbes ProbeID);
    BOOL        IsProbeEnabled  (EnumProbes ProbeID, LPCWSTR strEnabledFor);
    BOOL        EnableProbe     (EnumProbes ProbeID);
    BOOL        EnableProbe     (EnumProbes ProbeID, LPCWSTR strEnableFor);
    BOOL        DisableProbe    (EnumProbes ProbeID);
    BOOL        DisableProbe    (EnumProbes ProbeID, LPCWSTR strDisableFor);

private:

    static CustomerDebugHelper* m_pCdh;
    
    Crst*       m_pCrst;

    int         m_iNumberOfProbes;
    int         m_iNumberOfEnabledProbes;
    
    LPCWSTR*    m_aProbeNames;              // Map ProbeID to probe name
    BOOL*       m_aProbeStatus;             // Map ProbeID to probe activation

    // Used for custom enabling of probes
    RelevantMethodsList*    m_aProbeRelevantMethods;    // Map of ProbeID to relevant methods
    EnumParseMethods*       m_aProbeParseMethods;       // Map of ProbeID to parsing methods

    BOOL IsProbeEnabled (EnumProbes ProbeID, LPCWSTR strEnabledFor, EnumParseMethods enCustomParse);

};


#endif // _CUSTOMERDEBUGHELPER_
