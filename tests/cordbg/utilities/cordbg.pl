#!/usr/bin/perl -w
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
#
# cordbg.pl
#
# Custom driver for cordbg test:
#     Run the test.cmds test under cordbg.
# If there's a .tpl baseline, compare against it.
# If there's a .failcount use that to compare against.

use strict;

sub Usage() {
    print "Usage: perl cordbg.pl [test]\n";
    print "\n\t[tests]\tNames of tests to execute\n";
}

# Global Variables
my(
   @CommandLine,        # Copy of the original command line
   $TestName,           # name of the test we want to run
   $Cordbg,             # Cordbg location
   $cmdline,            # Command line to run cordbg
   $StartTime,         	# StartTime	
   $ResetDbg,           # cordbg reset cmds location
   $retval,             # return value
   $LogFile,            # LogFile to output to
   $DriverName,         # Name of the driver
   $CSCFLAGS,           # Flags to the CSC compiler
   $TestDirWithSeperator,   # Location of tests
   $OutputDir,              # Ouput directory name - build dependent
   $OutputDirWithSeperator, # Same as above, but with the path seperator appended
   $CorDbgDiff,             # Location of cordbgdiff perl script
   $TestForErrors,          # Location of testforerrors
   );

#
# LogMessage - Logs a message to the console and logfile
#
sub LogMessage
{
    my $Message = shift(@_);
    
    print "cordbg.pl:" . $Message . "\n"; 
}

#
# cmd_redirect -- execute a cmd, redirecting stdout, stderr.
#
# Redirects STDERR to STDOUT, and then redirects STDOUT to the
# argument named in $redirect.  It is done this way since
# invoking system() with i/o redirection under Win9x masks
# the return code, always yielding a 0.
#
# The return value is the actual return value from the test.
#
sub cmd_redirect
{
    my ($cmd) = @_;
    my $retval;

    LogMessage("cmd == [" . $cmd . "]");

    system($cmd);
    $retval = $?;
    if (($retval >> 8) != 0) {
        # The program exited with a non-zero result.
        $retval >>= 8;
    } elsif (($retval & 127) != 0) {
        # The program exited due to a signal.
        $retval &= 127;
    } elsif (($retval & 128) != 0) {
        # The program dumped core.
        $retval &= 128;
    } else {
        $retval = 0;
    }

    return $retval;
}

# Setup the output directory
if($ENV{BUILD_ALT_DIR}) {
    $OutputDir = "obj" . $ENV{BUILD_ALT_DIR};
} else {
    $OutputDir = "obj";
}

$StartTime = time();

# OS Specific Initialization
if (uc($^O) ne "MSWIN32") {
    $OutputDirWithSeperator = $OutputDir . "/";
    $Cordbg = "cordbg";
    $ResetDbg = $ENV{ROTOR_DIR} . "/tests/cordbg/utilities/cordbginit.cmds";
    $CorDbgDiff = $ENV{ROTOR_DIR} . "/tests/cordbg/utilities/cordbgdiff.pl";
    $TestForErrors = $ENV{ROTOR_DIR} . "/tests/cordbg/utilities/testforerrors.pl";
    $TestDirWithSeperator = $ENV{ROTOR_DIR} . "/tests/cordbg/";
} else {
    $OutputDirWithSeperator = $OutputDir . "\\";
    $Cordbg = "cordbg.exe";
    $ResetDbg = $ENV{ROTOR_DIR} . "\\tests\\cordbg\\utilities\\cordbginit.cmds";
    $CorDbgDiff = $ENV{ROTOR_DIR} . "\\tests\\cordbg\\utilities\\cordbgdiff.pl";
    $TestForErrors = $ENV{ROTOR_DIR} . "\\tests\\cordbg\\utilities\\testforerrors.pl";
    $TestDirWithSeperator = $ENV{ROTOR_DIR} . "\\tests\\cordbg\\";
}

################
# Main Program #
################

# Capture the command line for reporting
@CommandLine=@ARGV;
$TestName = pop(@CommandLine) ;

#init out value
$retval=0;


#Setup some global falgs
$CSCFLAGS = "-debug -nologo $ENV{CSC_COMPILE_FLAGS}";


chdir $OutputDirWithSeperator;

# Reset cordbg's settings to the default values.
$cmdline = $Cordbg . " < " . $ResetDbg . " > " . $TestName . ".reset";

$? = cmd_redirect( $cmdline );
if($?) {
    print ("[" . $cmdline . "] returned [" . $? . "]");
    die "Test Failed";
}

$cmdline = $Cordbg . " < " . $TestDirWithSeperator . $TestName . ".cmds" . " > " . $TestName . ".out"; 

$? = cmd_redirect( $cmdline );
if($?) {
    print ("[" . $cmdline . "] returned [" . $? . "]");
    die "Test Failed";
}

if (-e $TestDirWithSeperator . $TestName . ".tpl")
{
    #*********************************************************************
    #**** Compare output with template file
    #****
    $cmdline = "perl " . $CorDbgDiff . " " . $TestName . ".out " . $TestDirWithSeperator . $TestName . ".tpl";
    $? = cmd_redirect( $cmdline );
    if($?) {
        print ("[" . $cmdline . "] returned [" . $? . "]");
        die "Test Failed";
    }    
}
elsif (-e $TestDirWithSeperator . $TestName . ".failcount")
{
    #*********************************************************************
    # **** Check if there are some errors in the output
    # ****
    
    $cmdline = "perl " . $TestForErrors . " " . $TestName . ".out " . $TestDirWithSeperator . $TestName . ".failcount";
    $? = cmd_redirect( $cmdline );
    if($?) {
        print ("[" . $cmdline . "] returned [" . $? . "]");
        die "Test Failed";
    }    
    
}

exit($retval);

###############
# End of Main #
###############
