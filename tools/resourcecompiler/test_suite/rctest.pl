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
# Resource Compiler Test Driver
#

use Cwd;
use File::Compare;

# Global Variables
my(
   $LogFile,                  # Name of the log file
   $FailCount,                # Total number of suites that failed
   );

sub Usage() {
    print "Usage: perl rctest.pl\n";
}

#
# LogMessage - Logs a message to the console
#
sub LogMessage
{
    my $Message = shift(@_);
    
    print "rctest.pl:" . $Message . "\n"; 
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
    my ($directory) = shift(@_);
    my ($file) = shift(@_);
    my ($parameters)=@_;
    my $preprocessfile;
    my $TestBaseLineFile;
    my $retval;
    my $cmd;
    my $OutputFileNoQuote;
    my $OutputFile;
    
    $preprocessfile = $file . ".i";

    $cmd = "resourcecompiler";
    if ($parameters)
    {
        $cmd = $cmd . " " . $parameters . " ";
    }
    
    if (length($file) != 0)
    {
        if (length($directory) != 0)
        {
            $OutputFileNoQuote = $directory . $pathseperator . $file;
            $cmd = $cmd . " \"" . $OutputFileNoQuote . "\"";
            $OutputFile = $OutputFileNoQuote . ".satellite";
            $TestBaseLineFile = $OutputFileNoQuote . ".baseline";
        }
        else
        {
            $OutputFileNoQuote = $file;
            $cmd = $cmd . " \"" . $OutputFileNoQuote . "\"";
            $OutputFile = $OutputFileNoQuote . ".satellite";
            $TestBaseLineFile = $OutputFileNoQuote . ".baseline";
        }
    }
        
    LogMessage("cmd == [" . $cmd . "]");

    open SAVEOUT, ">&STDOUT";
    open SAVEERR, ">&STDERR";

    # This gets perl to shut up about only using SAVEOUT and SAVEERR
    # once each.
    print SAVEOUT "";
    print SAVEERR "";
    
    open STDOUT, ">>$LogFile" or die "Can't redirect stdout to ($LogFile)";
    open STDERR, ">&STDOUT" or die "Can't dup stdout";

    select STDERR; $| = 1;
    select STDOUT; $| = 1;

    $retval = system($cmd) >> 8;

    if (-e $preprocessfile) {
        $FailCount++;
        print "TEST FAILED! preprocessor file " . $preprocessfile . "not deleted";
    }
    
    if ($TestBaseLineFile)
    {
        if (-e $TestBaseLineFile)
        {
            print "Comparing Output \"" . $OutputFile . "\" and baseline \"" . $TestBaseLineFile . "\"\n";
            if (compare($TestBaseLineFile,$OutputFile) != 0)
            {
                $FailCount++;
                print "TEST FAILED! Output \"" . $OutputFile . "\" different then baseline \"" . $TestBaseLineFile . "\"\n";
            }        
        }
    }
    
    close STDOUT;
    close STDERR;

    open STDOUT, ">&SAVEOUT";
    open STDERR, ">&SAVEERR";

    return $retval;
}

################
# Main Program #
################

# Initialization

# Take a quick look around to see if we are running in an known environement
if(not $ENV{ROTOR_DIR}) {
    die "rctest.pl : Environement variable ROTOR_DIR not set - cannot continue\n";
}

$LogFile = Cwd::cwd() . "/test.log";
if(uc($^O) eq "FREEBSD") {
  $BaseLineFile = Cwd::cwd() . "/baselineunix.log";
  $pathseperator = "/";
} elsif (uc($^O) eq "MSWIN32") {
  $BaseLineFile = Cwd::cwd() . "/baseline.log";
  $pathseperator = "\\";
} else {
    die "Unknown operating system " . $^O . "\n";
}

unlink $LogFile;
$FailCount = 0;

unlink "success tests\*test.satellite";
unlink "fail tests\*test.satellite";
unlink "*.satellite";
unlink "*.i";

print "Running tests\n"; 

$? = cmd_redirect ("success tests", "basictest");
$? = cmd_redirect ("success tests", "bigtest");
$? = cmd_redirect ("success tests", "otherresourcetest");
$? = cmd_redirect ("success tests", "preproctest");
$? = cmd_redirect ("success tests", "sorttest");
$? = cmd_redirect ("success tests", "whitespacetest");
$? = cmd_redirect ("success tests", "stringtableformattest");

$? = cmd_redirect ("success tests", "basictest", "-fo out1test.satellite" );
if (! (-e "out1test.satellite"))
{
   $FailCount++;
   print "TEST FAILED! renamed file 1 not created";
}
$? = cmd_redirect ("success tests", "basictest", "-foout2test.satellite");
if (! (-e "out2test.satellite"))
{
   $FailCount++;
   print "TEST FAILED! renamed file 2 not created";
}
$? = cmd_redirect ("success tests", "basictest", "/fo out3test.satellite" );
if (! (-e "out3test.satellite"))
{
   $FailCount++;
   print "TEST FAILED! renamed file 3 not created";
}
$? = cmd_redirect ("success tests", "basictest", "/foout4test.satellite");
if (! (-e "out4test.satellite"))
{
    $FailCount++;
    print "TEST FAILED! renamed file 4 not created";
}

$? = cmd_redirect ("success tests", "definetest", "/dTEST_DEFINE=1");

#Check parameter validation
$? = cmd_redirect ("", "", "/r");
$? = cmd_redirect ("", "basictest", "/q");

#Check failure code paths
$? = cmd_redirect ("fail tests", "badcodepagepragma1test");
$? = cmd_redirect ("fail tests", "badcodepagepragma2test");
$? = cmd_redirect ("fail tests", "badincludetest");
$? = cmd_redirect ("fail tests", "baduserdefinedtest");
$? = cmd_redirect ("fail tests", "doubletest");
$? = cmd_redirect ("fail tests", "noBEGINtest");
$? = cmd_redirect ("fail tests", "noENDtest");
$? = cmd_redirect ("fail tests", "noidtest");
$? = cmd_redirect ("fail tests", "novaluetest");
$? = cmd_redirect ("fail tests", "unexpectedendoffile1test");
$? = cmd_redirect ("fail tests", "unexpectedendoffile2test");
$? = cmd_redirect ("fail tests", "unexpectedendoffile3test");
$? = cmd_redirect ("fail tests", "unknownescapevaluetest");
$? = cmd_redirect ("fail tests", "unknownpragmatest");

# Compare against the baseline

if (compare($LogFile,$BaseLineFile) != 0) 
{
    $FailCount++;
    print "Error: Differences between baseline file: " . $BaseLineFile . " and output log file: " . $LogFile ."\n";
}

print "Test Completed\n";
print "  Test Failed: ";
print $FailCount;
print "\n";

# Return an error if any tests failed
exit $FailCount if $FailCount > 0;

print "Tests passed\n"; 

###############
# End of Main #
###############
