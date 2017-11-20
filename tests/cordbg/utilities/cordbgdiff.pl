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

my $data=$ARGV[0] or die "No template file specified\n";
my $template=$ARGV[1] or die "No template file specified\n";

my @ignoreParts=(
				 qr/Version [0-9].[0-9].[0-9]+.[0-9]+/i,
				 qr/Microsoft Corporation \d+-\d+. All rights reserved./i,
				 qr/process -?\d+\/0x[0-9a-f]+ created./i,
				 qr/pid=0x[0-9a-f]+ \(\d+\)/i,
				 qr/^\d+\) *mscorlib\!.*/i,	# ignore all stack outputs from mscorlib - this could change as the code changes there
				 qr/\d+:/i, # IGNORE Line numbers since they may change

				 qr/\+[0-9a-f]+\[native\] \+[0-9a-f]+\[IL\]/,

				 #THIS GENERAL SHOULD BE AT THE END.
				 qr/0x[0-9a-f]+/i,		# IGNORE ALL HEX NUMBERS IN ANY CASE
				 qr/[a-z]:\\([\w\d\._-]+\\)*[\w\d\._-]+/i # IGNORE ALL THE PATHS AND FILE NAMES
				);

my @discardParts=(
       		   	  qr/^Warning: couldn\'t load symbols for.*/i,
				  qr/\[thread 0x[0-9a-f]+\] thread created./i,
				  qr/\[thread 0x[0-9a-f]+\] Thread exited./i,
				  qr/\[thread 0x[0-9a-f]+\] /i, # must be behind [thread xxx] thread created/exited.
				  qr/Error: hr=0x80131301, The debuggee has terminated.*/i # BUG#99639
				 );

my $comp = new Comparer(
						data=>$data,
						template=>$template,
						ignoreParts=> \@ignoreParts,,
						discardParts=> \@discardParts,
						emptyLines=>"ignore_with_spaces",
					   );
if(! $comp->Compare()) {
	$comp->ReportDiff();
	exit 255;
}
exit 0;
						
						
my $source1=$ARGV[0] or die "No input file1 specified\n";
my $source2=$ARGV[1] or die "No input file2 specified\n";
open INPUT1, $source1 or die;
open INPUT2, $source2 or die;

# here all \ must be doubled.
my @subst=
  (
   # ignore version numbers
   'runtime test debugger shell version [0-9].[0-9].[0-9]+.[0-9]+',
   'process \\d+\\/0x[0-9a-f]+ created.',
   'pid=0x[0-9a-f]+ \\(\\d+\\)',
   '0x[0-9a-f]+',		# IGNORE ALL HEX NUMBERS
   '[a-z]:\\\\(\\w+\\\\)*\\w+\\.\\w*' # IGNORE ALL THE PATHS AND FILE NAMES
  );

my @linesIngored=
  (
   # thread creation and destruction is non-deterministic
   '\\[thread 0x[0-9a-f]+\\] thread created.',
   '\\[thread 0x[0-9a-f]+\\] Thread exited.',

   # scenario09 reg command
   '^dr[03] ='
  );

# $out = normalizeLine($line)
sub normalizeLine {
	my($line) = shift;
	$line =~ s///;            #  no ^M at the end 
	  $line =~ s/\s+/ /g;		#  spaces are equal
	$line =~ s/^\s+//;
	$line =~ s/\s+$//;
	$line =~ tr/A-Z/a-z/;		# case is not important
	return $line;
}

# bool = lineShouldBeIgnored($line)
sub lineShouldBeIgnored {
	my $line = shift;
	for my $re (@linesIngored) {
		return 1 if( $line =~ m/$re/);
	}
	return 0;
}
	
my ($origline1,$origline2);
while (1) {
	$origline1 = <INPUT1>;
	$origline2= <INPUT2> || '';
  LinesRead:
	last unless defined $origline1; #if end of file then exit
	chomp(my $line1=$origline1);
	chomp(my $line2=$origline2);

	$line1 = normalizeLine($line1);
	if(lineShouldBeIgnored($line1)) {
		$origline1 = <INPUT1>;
		goto LinesRead;
	}
	
	$line2 = normalizeLine($line2);
	if(lineShouldBeIgnored($line2)) {
		$origline2 = <INPUT2> || '';
		goto LinesRead;
	}

	foreach my $subst (@subst) {
		$line1 =~ s/$subst/\(IGNORED\)/gi;
		$line2 =~ s/$subst/\(IGNORED\)/gi;
	}
	#  print $line1. "\n";
	unless($line1 eq $line2) {
		seek(INPUT1,0,1);		# does nothing needs in order $. tells the possition of INPUT1
		die "ERROR: Output differs at line $. of $source1\n-------\n$source1: $origline1\n\n$source2: $origline2\n";
	}
}

# check if file2 contains only blank lines
while ($line2=<INPUT2>) {
	chomp($line2);
	die "ERROR: Output differs: $source2  is longer at line $. \n" unless($line2 =~ /^\s*$/);
}

package Comparer;
#use strict;
#use warnings;
#use Carp;
sub croak {
	die @_;
}

=head1 NAME

Comparer - Compare files with limitations

=head1 SYNOPSIS

 use Comparer;

 my @ignoreParts = qw( qr/0x[0-9a-f]+/ );

 my $comp = new Comparer( template=> "template.tpl",
                          data=> "output.out",
                          ignoreParts=> \@ignorePartsRX,
                          discardParts=> \@discardPartsRX
                        );
 my $result = $comp->Compare();
 $comp->ReportDiff() unless ($result);

=head1 DESCRIPTION

Compares the data file with the template. IgnoreParts is an array of regular expressions which
contains parts of output and template which should be ignored.

If there is any regular expression matching the line in ignoreLines list, the whole line is being
ignored and next line is read instead of that.

If there is any regular expression matchin the part in discardParts list, the
part is removed before comparision is being done.

=head2 Methods

=over 2

=cut

=item B<new>

Creates new object. Possible arguments are:

=over 2

=item template 

template input file 

=item data

data input file 

=item ignoreParts

regular expressions which differences are ignored

=item discardParts 

regular expressions which parts are discarded.

=item caseSensitive

casesensitivness of the comparision (Default is 1)

=item emptyLines

could be one of I<ignore> (ignores if there are empty lines ""),
I<ignore_with_spaces> (ignores lines, even if they are not completely
empty, but follow patern /^\s*\n$/) and I<noignore> which does not
ignore empty lines. The default value is I<noignore>.

=item allDifferences

inidcates, if the comparision should stop on 1st difference found or
continue searching all differences. Default value is 0 -- stop on
first mismatch.

=back

=cut

# use fields qw(template templateName data dataName ignoreParts discardParts 
#  			  caseSensitive emptyLines allDifferences
#  			  _mismatches
#  			 );

sub new {
	my ($class,%params) = @_;
	my Comparer $this = bless {} , $class;
	open (TEMPLATE, "<$params{template}") or croak "Could not open template - $params{template}";
	open (DATA, "<$params{data}") or croak "Could not open data - $params{data}";
	$this->{template} = \*TEMPLATE;
	$this->{templateName} = $params{template};
	$this->{data} = \*DATA;
	$this->{dataName} = $params{data};

	$this->{ignoreParts} = $params{ignoreParts} || [ ];
	$this->{discardParts} = $params{discardParts} || [ ];
	$this->{caseSensitive} = $params{caseSensitive} || 1;
	
	if(defined $params{emptyLines}) {
		croak "Wrong param emtpyLines" unless(1==grep(m/^$params{emptyLines}$/,
												   qw/ignore noignore ignore_with_spaces/));
		$this->{emptyLines} = $params{emptyLines};
	} else {
		$this->{emptyLines} = "noignore";
	}

	if(defined $params{allDifferences}) {
		croak "Wrong param allDifferences" unless($params{allDifferences}==0 or $params{allDifferences}==1);
		$this->{allDifferences} = $params{allDifferences};
	} else {
		$this->{allDifferences} = 0;
	}
	return $this;
}

=item B<Compare>

Starts comparision. If there is some difference, then 0 is returned.
If the files seems to be identical then 1 is returned.

=cut

sub Compare {
	my Comparer $me = shift;
	my @mismatches;
	$me->{_mismatches} = \@mismatches;
	
	while(1) {
		my ($templateLn,$templateLine,$templateOrig) = $me->GetNormalizedLine($me->{template});
		my ($dataLn,$dataLine,$dataOrig) = $me->GetNormalizedLine($me->{data});

		# end of comparision
		last if(!defined $templateLine and ! defined $dataLine);

		if(!defined $templateLine) {
			push @mismatches , {reason=>"Extra Content",
							   templateLn=>"",
							   templateLine=>"",
							   dataLn=>$dataLn,
							   dataLine=>$dataOrig
							  };
			last
		}

		if(!defined $dataLine) {
			push @mismatches , {reason=>"Extra Content",
							   templateLn=>$templateLn,
							   templateLine=>$templateOrig,
							   dataLn=>"",
							   dataLine=>""
							  };
			last;
		}

		my $matches;
		if(!$me->{caseSensitive}) {
			$matches = ($templateLine cmp $dataLine) == 0;
		} else {
			$matches = (lc($templateLine) cmp lc($dataLine)) == 0;
		};
		if(!$matches) {
			push @mismatches, {reason=>"Different content",
							   templateLn=>$templateLn,
							   templateLine=>$templateOrig,
							   dataLn=>$dataLn,
							   dataLine=>$dataOrig
							  };
			next if($me->{allDifferences});
			last;
		}
	}
	return 0 if(@mismatches);
	return 1;
}

sub GetNormalizedLine {
	my Comparer $this = shift;
	my ($fd) = @_;
	seek($fd,0,1);				# initialize $.
	my $lineNo = $.;
	my ($line,$origLine);
  ReadLine:
	while(1) {
		return undef if(eof($fd));
		$origLine = $line = <$fd>;
		$line =~ s/\cM//;	# Quick fix for wrong sequence of chars at the end of template
		$line =~ s/\cJ//;
		for my $rx (@{$this->{discardParts}}) {
			$line =~ s/$rx//;
		}
		next if($this->{emptyLines} eq "ignore" and $line =~ m/^\n?$/);
		next if($this->{emptyLines} eq "ignore_with_spaces" and $line =~ m/^\s*\n?$/);
		last;
	}

	for my $rx (@{$this->{ignoreParts}}) {
		$line =~ s/$rx/IGNORED/g;
	}
	return wantarray?($lineNo,$line,$origLine):$line;
}


=item B<ReportDiff>

Reports all differencies found between template and output.

=cut

sub ReportDiff {
	my Comparer $this = shift;
	if($this->{_mismatches}) {
		print "Difference(s) Found:\n",
    	      "==================\n";
		for my $mismatch (@{$this->{_mismatches}}) {
			print "$mismatch->{reason}\n";
			print "$this->{templateName}:$mismatch->{templateLn} -\n$mismatch->{templateLine}\n";
			print "$this->{dataName}:$mismatch->{dataLn} -\n$mismatch->{dataLine}\n";
		}
	} else {
		print "No differences found.\n";
	}
}


sub DESTROY {
	my Comparer $this = shift;
	close $this->{template};
	close $this->{data};
}

=back

=cut


1;
__END__
