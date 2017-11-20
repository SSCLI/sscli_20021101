# ==++==
# 
#   
#    Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
#   
#    The use and distribution terms for this software are contained in the file
#    named license.txt, which can be found in the root of this distribution.
#    By using this software in any fashion, you are agreeing to be bound by the
#    terms of this license.
#   
#    You must not remove this notice, or any other, from this software.
#   
# 
# ==--==

# change references to CallConvAttribute into modopt([mscorlib]System.Runtime.CompilerServices.*)

while(<>) {
    if (m/\.class .* CallConv.*Attribute/) {
        $incallconvattributeimpl=1;
        next;
    }        
    if(m/\/\/ end of class CallConv.*Attribute/) {
        $incallconvattributeimpl=0;
        next;
    }
    
    if(m/\.custom instance void CallConv(.*)Attribute/) {
        $pendingcallconv = $1;
        next;
    }
    
    if ($pendingcallconv) {                     
        if(m/.*Invoke\(.*/) {        
            print "modopt([mscorlib]System.Runtime.CompilerServices.CallConv" . $pendingcallconv . ")\n";    
            $pendingcallconv = "";
        }
    }
    
    print unless $incallconvattributeimpl;
}
