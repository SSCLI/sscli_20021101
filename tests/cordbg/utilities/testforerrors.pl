#! /usr/bin/perl -w
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

=head1 NAME

testForErrs.pl - tests for errors in output files

=head1 USAGE

 testForErrs.pl output_file [expected_count_file]
 where

B<output_file> is file which is scanned for the error messages

B<expected_count_file> is file which contains # of expected occurences
(could be specified >0 if you are intentionally expecting some errors)

=head1 expected_coutn_file (usually named xx.failcount)

The file consist of individul sections. Section could be:

=over 2

=item [ErrorsAllowed]

This Section consist of one line statit how many errors are ignored in the
parsed file. It could be either one number, indicating exact match or range
"number-numger", evenutally range with some missing bound - upper unlimited:
"number-" or down unlimited: "-number".

=item [ErrorLineRx]

There should be set of regular expression which mark lines cointaining some errors.
There is implicitely one regular expression - /error/i.

=item [IgnoreLineRx]

Those are regular expression marking lines that should be ignored during scanning for errors.

=item [ExpectedRx]

Regulare expression which must appear in the output. Note that lines matching I<IngoreLineRx>
are not scanned. You can state the number of occurances requied in the form:
 [ExpectedRx]
 .*hello.*
 !Count: 2

=back

=cut

my $usage="
 see POD for usage.
";

my @errorRE = 
(
 qr/error/i
 );

my @ignoreRx =
(
# qr/Error loading source file.*/i
);

my @expectedRX = ();			# will be read from .failcount

die $usage if(@ARGV != 1 and @ARGV!= 2);

my ($minErrorsAllowed,$maxErrorsAllowed);

if(defined $ARGV[1]) {
	my $fcr = new FailCountReader( $ARGV[1] );
	push @ignoreRx, $fcr->getSection("IgnoreLineRx");
	push @errorRE, $fcr->getSection("ErrorLineRx");

	# expectedRX is a little bit more tricky
	my @section = $fcr->getSection("ExpectedRx");
	my $i=0;
	while($i<@section) {
		my $expectRx=$section[$i];
		if($#section >= $i+1 and $section[$i+1]=~/^\!Count:(.*)/) {
			my $countNum=$1;
			die "wrong count Number" unless $countNum;
			push @expectRx, [$expectRx,$countNum];
			$i++;
		} else {
			push @expectRx, $expectRx;
		}
		$i++;
	}
	my @errorsAllowedSec = $fcr->getSection("ErrorsAllowed");
	if(@errorsAllowedSec>0) {
		die "wrong format of 'ErrorsAllowed' section" if @errorsAllowedSec!=1;
		$_ = $errorsAllowedSec[0];
		if(m/^\d+$/) {
			$minErrorsAllowed=$maxErrorsAllowed=$_;
		} elsif (m/^(\d+)-$/) {
			$minErrorsAllowed=$1;
		} elsif(m/^-(\d+)$/) {
			$maxErrorsAllowed=$1;
		} elsif(m/^(\d+)-(\d+)$/) {
			$minErrorsAllowed=$1;
			$maxErrorsAllowed=$2;
		} else {
			die "Wrong 'ErrorsAllowed' format!";
		}
	}
} else {
	#if nothing is there then no errors are allowed!
	$minErrorsAllowed=$maxErrorsAllowed = 0;
}

$"="\n";

print "ingnoreLineRx\n",
      "=============\n@ignoreRx\n\n",
      "errorLineRx\n",
      "===========\n@errorRE\n\n",
      "expectedRX\n",
      "===========\n@expectedRX\n\n";

print "ErrorRange: ", (defined $minErrorsAllowed)?$minErrorsAllowed:"unlimited", " - ",
  (defined $maxErrorsAllowed)?$maxErrorsAllowed:"unlimited", "\n";
  
$ef = new SmartGrep ( dataFile => $ARGV[0],
					  ignoreLineRx=> \@ignoreRx,
					  errorLineRx=> \@errorRE,
					  expectedRx=> \@expectedRX
					);


my $errorsFound = $ef->Scan();

my $outOfBounds = 0;
$outOfBounds=1 if(defined $minErrorsAllowed and $minErrorsAllowed>$errorsFound);
$outOfBounds=1 if(defined $maxErrorsAllowed and $maxErrorsAllowed<$errorsFound);

if($outOfBounds or ! $ef->ExpectedCountsOK()) {
	print "Found $errorsFound\n";
	$ef->ReportErrors();
	exit 255;
}
exit 0;

########################################### SmartGrep #################################
package FailCountReader;

=head1 NAME

FailCountReader

=head1 SYNOPSIS

 $fcr = new FailCountReader(
                            "tests.failcount"
                            );
 my @sectionHelloContent = $fcr->getSection("hello");

=cut

sub new {
	my $class = shift;
	my $filename = shift;
	my $this = bless { }, $class;

	die "Filename required!" unless defined $filename;
	my $failcount = \*FILE;
	open FILE, "<$filename" or die;
	$this->Scan($failcount);
	close $failcount;
	return $this;
}

sub Scan {
	my FailCountReader $this = shift;
	my $failcount = shift;
	
	my %sections;
	my $activeSection;
	while(<$failcount>) {
		chomp;
		s/\#.*$//;				# ignore comments
		next if m/^\s*$/;		# get rid of empty lines
		if(m/^\[([\w\d]+)\]\s*$/) {
			$activeSection = uc $1;
			die "section $activeSection already defined!\n" if exists $sections{$activeSection};
			$sections{$activeSection} = [ ];
		} else {
			die "No active section!\n" unless defined $activeSection;
			s/^\s*//; s/\s*$//;
			s/^"(.*)(?<!\\)"$/$1/;		# handle "" if quoted.
			push @{$sections{$activeSection}}, $_;
		}
	}
	$this->{_sections} = \%sections;
}

sub getSection {
	my $this = shift;
	my ($sectionName) = @_;
	$sectionName = uc $sectionName;
	return ( ) unless exists $this->{_sections}{$sectionName};

	my @sectionData = @{$this->{_sections}{$sectionName}};
	return @sectionData;
}

########################################### SmartGrep #################################

package SmartGrep;

sub rejectArg { return 0;}
sub acceptArg { return 1;}

sub check_named_args($\%$) {
	my($this,$args,$validator) = @_;

	for my $i(keys %$args) {
		die "Unknown key \"$i\"!\n" unless exists $validator->{$i};

		# copy now so in case the user could overwrite our values in handler.
		$this->{$i} = $args->{$i};
		
		my $valueOK;
		if(ref($validator->{$i}) eq "CODE") 
                {
		    my $func = $validator->{$i};
			$valueOK = &$func($args->{$i});
		} 
                else {
			$valueOK = $validator->{$i};
		}
		unless($valueOK) {
			delete $this->{$i};
			die "Wrong value for key \"$i\"!\n";
		}
	}
}



=head1 NAME

ErrorFinder

=head1 SYNOPSIS

 $ef = new SmartGrep ( dataFile => "input.txt",
                       ignoreLineRx=> \@ignoreRx,
                       errorLineRx=> \@errorRE,
                       expectedRx=> \@expectedRX
                     );
 $ef->Scan();
 if($ef->ErrorsFound()>0) {
     $ef->ReportErrors();
 }

=cut

=head2 Methods

=over 4

=item new

creates new object. Possible arguments: B<dataFile>, B<ignoreLineRx>,
B<errorLineRx>. B<expectedRX>.

=cut

sub new {
	my $class = shift;
	my %args=@_;

	my $this = bless {}, $class;
	check_named_args($this, %args,
	  {dataFile=> sub {
		   my $file = shift;
		   open DATAFILE,$file or return 0; #die ;
		   my $dataFileHandle = \*DATAFILE;
		   $this->{dataFileHandle} = $dataFileHandle;
		   return acceptArg;
	   },
	   ignoreLineRx=> acceptArg,
	   errorLineRx=> acceptArg,
	   expectedRx=> sub {
		   my $expectedRx = shift;
		   my @expectedRx;
		   for my $rxrecord (@$expectedRx) {
			   my ($rx,$ec);
			   if(ref($rxrecord) eq "ARRAY") {
				   $rx = $rxrecord->[0];
				   $ec=$rxrecord->[1];
			   } else {
				   $rx = $rxrecord;
				   $ec = 1;
			   };
			   push @expectedRx, { rx=>$rx,
									  expectedCount=>$ec,
									  actualCount=>[ ]
									};
		   }
		   $this->{expectedRx} = \@expectedRx;
		   return acceptArg;
	   }
	  }
					);
	return $this;
}

=item Scan

Scans for the errors in the dataFile.

=cut

sub	Scan {
	my $this = shift;
	my @errorsFound;
	$this->{_errorsFound} = \@errorsFound;
	my $fh = $this->{dataFileHandle};
	
  INPUTLOOP:
	while(<$fh>) {
		for my $rx (@{$this->{ignoreLineRx}}) {
			next INPUTLOOP if $_ =~ m/$rx/;
		}
		for my $rx (@{$this->{errorLineRx}}) {
			if( $_=~ m/$rx/) {
				push @errorsFound, 
				  {file=>$this->{dataFile},
				   lineNo=> $.,
				   lineContent=>$_ };
				last;
			}
		}
		for my $rxrecord (@{$this->{expectedRx}}) {
			if( $_=~ m/$rxrecord->{rx}/ ) {
				push @{$rxrecord->{actualCount}},
				  { lineNo=> $.,
					lineContent=>$_
				  };
			}
		}
	}
	return wantarray?@errorsFound :scalar  @errorsFound;
}

sub ExpectedCountsOK {
	my $this = shift;

	for my $rxrecord (@{$this->{expectedRx}}) {
		return 0 unless scalar @{$rxrecord->{actualCount}} == $rxrecord->{expectedCount};
	}
	return 1;
}

=item ReportErrors

Report Found Errors

=cut

sub ReportErrors {
	my $this = shift;
	return if(! defined $this->{_errorsFound});
	if($this->{_errorsFound}==0) {
		print "No Errors found\n";
		return;
	}

	my $errs = @{$this->{_errorsFound}};
	print "Found $errs errors!\n",
	      "=================\n";
	for my $err (@{$this->{_errorsFound}}) {
		print "$err->{lineNo}: $err->{lineContent}";
	}

	#process expected
	unless($this->ExpectedCountsOK()) {
		print "\n\nExpected regular expressions mismatch!\n";
		for my $rxrecord (@{$this->{expectedRx}}) {
			unless(scalar @{$rxrecord->{actualCount}} == $rxrecord->{expectedCount}) {
				print "Expected $rxrecord->{expectedCount} of '$rxrecord->{rx}' -- found @{[scalar @{$rxrecord->{actualCount}}]} \n",
				  "-----------------------------------------------------------------\n";
				for my $err (@{$rxrecord->{actualCount}}) {
					print "$err->{lineNo}: $err->{lineContent}";
				}
			}
		}
	}
}


sub DESTROY {
	my $this = shift;
	close $this->{dataFileHandle};
}
