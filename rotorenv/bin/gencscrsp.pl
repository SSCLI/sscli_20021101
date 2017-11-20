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

$VERSION_FILE_NAME1 = $ENV{COFFBASE_TXT_FILE};
$ResponseFile = $ARGV[0];
$Args = $ARGV[1];
$CscBase = $ARGV[2];
$CscBase =~ s/(.+)\..*$/\1/;
$BaseAddress = "";
$Verbose = 0;
if ($Verbose !~ 0)
{
   print "Arg0: ".$ARGV[0]."\n";
   print "Arg1: ".$ARGV[1]."\n";
   print "Arg2: ".$ARGV[2]."\n";
}
if (defined $VERSION_FILE_NAME1) {
    open(VERSION1,$VERSION_FILE_NAME1) || print "Unable to open version file ".$VERSION_FILE_NAME2 && exit 1;
    while(<VERSION1>)
    {
       if ( /\b$CscBase\s+(0x\S*).*/i )
       {
          $BaseAddress = "/baseaddress:".$1." ";
          if ($Verbose !~ 0)
          {
             print "Found base address: ".$BaseAddress."\n";
          }
          last;
       }
    }
    close(VERSION1);
}
open(RSPFILE, ">$ResponseFile") || print "Unable to open response file write." && exit 1;
if (defined $VERSION_FILE_NAME1) {
    print RSPFILE $BaseAddress;
}
print RSPFILE $Args;
close(RSPFILE);

