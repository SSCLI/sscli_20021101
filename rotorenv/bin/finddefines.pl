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
open(HEADERFILE,$ARGV[0]) || print "Unable to open file ".$ARGV[0] && exit 1;

while(<HEADERFILE>)
{
    if(/^#define/) {
       if(!/\\$/) {
           s/L"/"/;
           print;
       }
   }
}
close(HEADERFILE);
