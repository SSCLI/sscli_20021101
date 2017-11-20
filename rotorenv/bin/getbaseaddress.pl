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

# [0] = File to rebase
# [1] = coffbase.txt keyword to look up
$VERSION_FILE_NAME1 = $ENV{COFFBASE_TXT_FILE};
$BaseAddress = "";
$RebaseFile = $ARGV[0];
$CoffBase = $ARGV[1];
$CoffBase =~ s/(.+)\..*$/\1/;
open(VERSION1,$VERSION_FILE_NAME1) || print "Unable to open version file ".$VERSION_FILE_NAME2 && exit 1;
while(<VERSION1>)
{
   if ( /\b$CoffBase\s+(0x\S*).*/i )
   {
      $BaseAddress = $1;
      last;
   }
}
close(VERSION1);
if ($BaseAddress == "")
{
   $cmdstr = "rebase -q -b ".$BaseAddress." ".$RebaseFile;
   print "\t".$cmdstr."\n";
   system($cmdstr);
}
else
{
   print "Base address not found for ".$CoffBase."\n";
}


