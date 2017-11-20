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
# strip the date, time and path from the MIDL generated headers, 
#  so they stay the same between runs

# Insert copyright banner
print "// ==++==\n";
print "//\n";
print "//   Copyright (c) Microsoft Corporation.  All rights reserved.\n";
print "//\n";
print "// ==--==\n";

while(<>) {
    if (m/^#pragma warning/) {
        # Wrap in #ifdef _MSC_VER
        print "#ifdef _MSC_VER$/";
        print;
        print "#endif$/";
        next;
    }
    if(m/\/\* at .*/) {
        $ingarbage=1;
        next;
    }
    if(m/\/\* Compiler settings for .*/) {
        $ingarbage=1;
        next;
    }
    if($ingarbage and m/\*\//) {
        $ingarbage=0;
        next;
    }
    if(m/^#endif !/) {
	s/^(#endif )/\1\/\/ / ;
    }
    print unless $ingarbage;
}
