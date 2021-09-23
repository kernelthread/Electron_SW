# Build an Acorn MOS ROM containing a ROM file system
#
# Copyright 2021, Dennis May
# First Published 2021
#
# This file is part of Miscellaneous Electron Software.
#
# Miscellaneous Electron Software is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Miscellaneous Electron Software is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# Miscellaneous Electron Software.  If not, see <https://www.gnu.org/licenses/>.
#

use Getopt::Long;
use Cwd;
use File::Path;
use File::Copy;
use Path::Tiny;
use strict;
use warnings;

my $thisScript = Cwd::abs_path($0);
my $scriptDir = path($thisScript)->parent->stringify;   # The absolute path to this script
my $ExeExt = ($ENV{'ComSpec'}) ? '.exe' : '';   # Executables end with .exe on Windows, no extension on Linux

################################################################################
# Test if a string is a valid Acorn file name
################################################################################
sub ValidAcornName($) {
    my ($n) = @_;
    return 0 if (!$n);
    return 0 if (length($n)>10);
    return 0 if ($n =~ /(\s|\\|\/|[^\x21-\x7e])/);
    return 1;
}


################################################################################
# Create an empty working directory
# If it already exists, delete it and all its contents, then re-create
################################################################################
sub CreateDir(;$) {
    my ($dirname) = @_;
    if (!$dirname) {
        $dirname = "romfs_work";
    }
    if (-d $dirname) {
        print("Removing existing directory $dirname\n");
        rmtree($dirname) or die "ERROR: Can't create directory $dirname\n";
    }
    print("Creating directory $dirname\n");
    mkdir $dirname or die "ERROR: Can't create directory $dirname\n";
    return $dirname;
}


################################################################################
# Replace the paths for included files with absolute paths if necessary
# This is needed because we change directories
################################################################################
sub FixFilePaths($$) {
    my ($argsRef, $origDir) = @_;
    my @includedFiles;
    foreach (@$argsRef) {
        my $acornName;
        my $hostName;
        if (/^([\x21-\x7e]{1,10})\=(.*)$/) {
            $acornName = $1;
            $hostName = $2;
        } else {
            $hostName = $_;
        }
        my $hostPath = path($hostName);
        unless ($hostPath->is_absolute) {
            $hostPath = path($origDir, $hostName);
        }
        $hostName = $hostPath->absolute->stringify;
        if ($acornName) {
            push @includedFiles, "$acornName=$hostName";
        } else {
            push @includedFiles, $hostName;
        }
    }
    return @includedFiles;
}


################################################################################
# Find a file in one of a list of directories
# Return the path of the file in the first directory where it is found
# Die if not found.
################################################################################
sub FindFile($$) {
    my ($filename, $pathListRef) = @_;
    my @checked;
    foreach (@$pathListRef) {
        my $p = path($scriptDir, $_, $filename);
        my $ps = $p->absolute->stringify;
        if (-e $ps) {
            print "Found $ps\n";
            return $ps;
        } else {
            push @checked, $ps;
        }
    }
    my $errMsg = "ERROR: Failed to find `$filename`. Paths checked:\n";
    foreach (@checked) {
        $errMsg .= "  `$_`\n";
    }
    die $errMsg;
}


################################################################################
# Find a file in one of a list of directories, then copy it to the current directory
# Die if the file is not found
################################################################################
sub FindAndCopyFile($$) {
    my ($filename, $pathListRef) = @_;
    my $src = FindFile($filename, $pathListRef);
    print "Copying $src\n";
    copy($src, '.');
}


################################################################################
# Create or copy files needed to build the ROM image
################################################################################
sub CreateFiles($$$$) {
    my ($title, $ver, $copyright, $compress) = @_;

    open TITLE_FILE, ">title.txt" or die "ERROR: Can't open file title.txt for write\n";
    print TITLE_FILE $title;
    close TITLE_FILE;
    open VER_FILE, ">version_string.txt" or die "ERROR: Can't open file version_string.txt for write\n";
    print VER_FILE $ver;
    close VER_FILE;
    open COPYRIGHT_FILE, ">copyright.txt" or die "ERROR: Can't open file copyright.txt for write\n";
    print COPYRIGHT_FILE $copyright;
    close COPYRIGHT_FILE;

    my $pathList1 = [ ".", "../../6502_CODE/ROMFS" ];
    FindAndCopyFile("romfs_rom.asm", $pathList1);
    my $pathList2 = [ "../Common", "../../6502_CODE/Common" ];
    FindAndCopyFile("rom_skeleton.asm", $pathList2);

    if ($compress) {
        my $pathList3 = [ "../Common", "../../6502_CODE/zlib6502" ];
        FindAndCopyFile("romfs_inflate.asm", $pathList3);
    }
}


################################################################################
# Issue a command with a list of arguments using the standard OS command shell
# Die if the command returns non-zero status
################################################################################
sub IssueCommand($$) {
    my ($cmdExe, $argsRef) = @_;
    my $cmd;
    if ($cmdExe =~ /\s/) {
        $cmd .= "\"$cmdExe\"";
    } else {
        $cmd .= $cmdExe;
    }
    foreach (@$argsRef) {
        if (/\s/) {
            $cmd .= " \"$_\"";
        } else {
            $cmd .= " $_";
        }
    }
    print "Issuing command:\n  `$cmd`\n";
    system($cmd)==0 or die "ERROR: Command failed\n";
}


################################################################################
# Generate a file containing ROMFS file data which includes all the specified
# files, with the specified Acorn FS names. The file includes all necessary
# block headers and image terminator.
# Compress the file if that was requested.
################################################################################
sub BuildROMFSImage($$$) {
    my ($compress, $title, $filesRef) = @_;
    my $cmdExe = "build_romfs";
    if (-e "$scriptDir/build_romfs$ExeExt") {
        $cmdExe = "$scriptDir/build_romfs$ExeExt";
    }
    my @args;
    push @args, "0";                            # base address
    if ($compress) {
        push @args, "_romfs_image_raw.bin";     # output filename (pre-compression)
    } else {
        push @args, "_romfs_image.bin";         # output filename
    }
    push @args, "$title";                       # ROM title
    foreach (@$filesRef) {
        push @args, $_;
    }
    IssueCommand($cmdExe, \@args);
    if ($compress) {
        my $cmd2 = "zopfli";
        my @args2;
        push @args2, "--deflate";
        push @args2, "--w256";
        push @args2, "-c";
        push @args2, "_romfs_image_raw.bin";
        push @args2, ">_romfs_image.bin";
        IssueCommand($cmd2, \@args2);
    }
}


################################################################################
# Generate a 16KB Sideways ROM image containing a ROMFS file system with
# all specified files, plus decompressor code if the contained ROMFS image
# has been compressed.
################################################################################
sub BuildROM($$$$) {
    my ($outfn, $outfnbase, $compress, $ver) = @_;
    my $cmdExe = "acme";
    my @args;
    push @args, "-DROM_VERSION=$ver";
    push @args, "-DINCLUDE_ROMFS_DATA=1";
    push @args, "-DROMFS_USE_OFFSET_ADDRESSES=1";
    push @args, "-DROMFS_COMPRESSED_DATA=1" if ($compress);
    push @args, "-o", $outfn;
    push @args, "-r", $outfnbase.".lst";
    push @args, "-l", $outfnbase.".sym";
    push @args, "romfs_rom.asm";
    IssueCommand($cmdExe, \@args);
    my $sz = -s $outfn;
    if ($sz > 16384) {
        die "ERROR: ROM size exceeded the available 16KB\n";
    }
}


################################################################################
# Read the symbol file output by the assembler
################################################################################
sub ReadSymbolFile($) {
    my ($symFileName) = @_;
    my %symbols;
    open FILE, $symFileName or die "ERROR: Can't open $symFileName\n";
    while (<FILE>) {
        if (/^\s*(\w+)\s*\=\s*\$([0-9a-fA-F]+)/) {
            my $name = $1;
            my $hexVal = $2;
            $symbols{$name} = $hexVal;
        }
    }
    close FILE;
    return %symbols;
}

################################################################################
# PROGRAM MAIN FUNCTION#
################################################################################
sub Main() {

    #	PROCESS THE COMMAND LINE
    unless (@ARGV) {
	    Usage();
    }

    #	Process options and check that all are recognised
    my $title;
    my $version;
    my $copyright;
    my $outfn;
    my $compress;
    my $workDir;
    unless (GetOptions(	'title|t=s'	    =>	\$title,
					    'version|v=s'	=>	\$version,
					    'copyright|c=s'	=>	\$copyright,
					    'output|o=s'	=>	\$outfn,
                        'workdir|w=s'   =>  \$workDir,
                        'compress'      =>  \$compress,
	    )) {
	    Usage();
    }

    my $outfnbase;
    if (!$outfn) {
        if ($title =~ /^(\*+)(.*?)(\*+)$/) {
            $outfn = $2;
        } else {
            $outfn = $title;
        }
    }
    if ($outfn =~ /^(.*)\.[^\.]+/) {
        $outfnbase = $1;
    } else {
        $outfnbase = $outfn;
    }

    print "SCRIPT:           $thisScript\n";
    print "SCRIPTDIR:        $scriptDir\n";
    print "ExeExt:           $ExeExt\n";
    print "ROM Title:        $title\n";
    print "ROM Version:      $version\n";
    print "Copyright:        $copyright\n";
    print "Output filename:  $outfn\n";
    print "Output name base: $outfnbase\n";
    print "Compressed:       ".($compress ? "YES" : "NO")."\n";

    if (!$title or !ValidAcornName($title)) {
        Usage("ROM title must be specified and must consist of 10 or fewer\nnon-whitespace printable ASCII characters.");
    }
    if ($version !~ /^\d+$/ or $version < 0 or $version > 255) {
        Usage("ROM version must be specified and must be an integer between 0 and 255.");
    }
    if ($copyright !~ /^\(C\)/) {
        Usage("ROM copyright string must be specified and must begin with (C).");
    }

    print join("\n", @ARGV);
    print "\n";

    my $origDir = getcwd();
    my $dirname = CreateDir($workDir);
    chdir($dirname);
    CreateFiles($title, $version, $copyright, $compress);
    my @includedFiles = FixFilePaths(\@ARGV, $origDir);
    BuildROMFSImage($compress, $title, \@includedFiles);
    BuildROM($outfn, $outfnbase, $compress, $version);

    print "Built ROM image file $dirname/$outfn\n";
    my %symbols = ReadSymbolFile($outfnbase.".sym");
    my $endAddr = $symbols{'ROMFS_DATA_END'};
    if ($endAddr) {
        my $used = hex($endAddr) - 0x8000;
        print "$used bytes used out of 16384\n";
    }
}

sub Usage(;$) {
my ($msg) = @_;
if ($msg) {
    print STDERR "\nERROR: $msg\n";
}

die <<ENDHELP;

Usage:
    perl build_romfs_rom.pl -t <ROM Title> -v <ROM ver> -c <Copyright>
                                 -o <Output file name> [--compress]
                                 [-w <working directory>]
                                      [file1 [file2 ...]]
    where: <ROM Title> is a string of up to 10 characters with no spaces which
                       will be used as the ROM title
           <ROM ver>
           <Copyright> is a string to be used as the ROM copyright statement
                       It may contain spaces, in which case it must be enclosed
                       in quotes. It must start with (C).
           --compress  if specified the file system data is compressed using the
                       'deflate' algorithm with a window size of 256 bytes.
           file1 etc.  Specify files to be included in the ROM FS.

    Included files can be renamed in the ROM using a specification like this:

        ACORNNAME=<host file name>

    where <host file name> is the path name to the file on the host machine, and
    ACORNNAME is the name which the file will appear as on the Electron, which
    must be 10 characters maximum, with no spaces.
    If just the host file name is specified, the ACORNNAME is taken as the name
    of the file (file name plus extension only, no directories) with any non
    printable characters or spaces replaced by underscores, and truncated to 10
    characters.

ENDHELP
}

################################################################################
Main();
