#!/usr/bin/perl

# This script runs the manta command read from each line of the input text file
# with the specified camera path and creates a gnuplot of the results.
# An html page is generated with all of the gnuplot images.

# Abe Stephens, October 2005

# Parse args.
$input_file    = "";
$output_prefix = "manta_path_plot";

$path_file     = "";
$path_sync     = 1;
$path_t        = 0; # Use default.
$path_time     = 0;
$path_start    = 0;
$path_last     = 0;
$path_warmup   = 10;
$title         = "Manta Camera Path Benchmark";
$plot_x        = 2;
$plot_y        = 5;
$plot_columns  = 6;
$keep          = 1;
$line          = 0;


$start_time = time();

@plot_label = ( "Frame number", 
                "Transaction number", 
                "Elapse Frame", 
                "Elapse Seconds",
                "Average fps" );


###############################################################################
# Variable replacement.
sub variable_replace {
    my @result = ();

    # Replace in each string.
    foreach (split(/ /,$_[0])) {
        my $string = $_;

        # print "Initial: $string\n";

        # Look for a variable to replace.
        if ($string =~ /\$\{([a-zA-Z\_]+)/) {
            my $variable = $1;

            # print "\t[$variable]\n";

            # Search for the key.
            if ($variables{$variable}) {
                my $value = $variables{$variable};
                $string =~ s/\$\{$variable\}/$value/g;
            }
            else {
                print "Line $line: \${$variable} Not found.\n";
                exit(1);
            }
            # print "Result: $result\n";
        }
        push(@result,$string);
        


    }
    return join(" ",@result);
}


###############################################################################
# Main program.
for ($i=0;$i<@ARGV;++$i) {
    if ($ARGV[$i] eq "-file") {
        $input_file = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-prefix") {
        $output_prefix = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-path") {
        $path_file = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-sync") {
        $path_sync = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-delta_t") {
        $path_t = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-delta_time") {
        $path_time = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-interval") {
        $path_start = $ARGV[++$i];
        $path_last  = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-warmup") {
        $path_warmup = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-title") {
        $title = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-plot") {
        $plot_x = $ARGV[++$i];
        $plot_y  = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-keep") {
        $keep = 1;
    }
    elsif ($ARGV[$i] eq "-nokeep") {
        $keep = 0;
    }
    elsif ($ARGV[$i] eq "-columns") {
        $plot_columns = $ARGV[++$i];
    }    
}

# Check to see if any defaults should be used.
if (($input_file eq "")) {
    print "Usage: \n";
    print "-file       <manta commands file>\n";
    print "-prefix     <output png prefix>\n";
    print "-path       <camera path file>\n";
    print "-sync       <camera path sync>\n";
    print "-delta_t    <camera path delta t>\n";
    print "-delta_time <camera path delta time>\n";
    print "-interval   <m> <n>\n";
    print "-warmup     <n>\n";
    print "-title      <title>\n";
    print "-plot       <x> <y> (default column $plot_x and $plot_y)\n";
    print "-columns    <n> (number of columns in manta output, default 5)\n";
    print "            Column info is passed to gnuplot, first column is 1 not 0.\n";
    print "-[no]keep   (keep temporary files, default on)\n";
    print "\n";
    print "Each line of the command file is either a manta command or \n";
    print "plot: [above args] to change args between runs\n";
    exit(1);
}

# Open the input file.
if (!open( INPUT, "<" , $input_file) ) {
    print "Could not open input file " . $input_file . "\n";
    exit(1);
}

# Output the output html file.
$output_html = $output_prefix . ".html";
if (!open( HTML, ">", $output_html )) {
    print "Could not open output html file " . $output_html . "\n";
    exit(1);
}

print HTML "<head title=\"$title\"></head>\n";
print HTML "<h1>$title</h1>\n";
print HTML "Generated using:<br><font face=\"courier\" size=-1>manta_path_plot.pl " . join(" ",@ARGV) . "</font><p>\n";

# Read in manta commands.
$subtitle = 0;

while (<INPUT>) {

    $input_line = $_;
    
    # Process the command.
    process_command( $input_line );
}

# Make sure some runs were made.
if (@command_array == 0) {
    exit(0);
}

###############################################################################
# Create a graph with all results.
# Pipe commands to gnuplot.

print HTML "<h2>Summary</h2>\n";
print HTML "<pre>\n";

if (!open(GNUPLOT, "|-", "gnuplot")) {
    print "Could not open gnuplot\n";
    exit(1);
}

$png_file = $output_prefix . "-all.png";

@array = split(/\//,$png_file);
$png_file_name = $array[@array-1];

$eps_file = $output_prefix . $line . "-all.eps";

@array = split(/\//,$eps_file);
$eps_file_name = $array[@array-1];
        
@array = split(/\//,$out_file);
$txt_file_name = $array[@array-1];

$plot_file = $output_prefix . $line . "-all.gnuplot";

$plot = "plot ";
for ($i=0; $i<@out_array; ++$i) {

    print HTML "$subtitle_array[$i]: $command_array[$i]\n";

    $plot = $plot . "\"$out_array[$i]\" using $plot_x_array[$i]:$plot_y_array[$i] title \"$subtitle_array[$i]\" ";
    if ($i < (@out_array)-1) {
        $plot = $plot . ", ";
    }
}
$plot = $plot . "\n";

print HTML "</pre>\n";

print GNUPLOT "set data style linespoints\n";
# print GNUPLOT "set term png small color\n";
print GNUPLOT "set term png small\n";
print GNUPLOT "set xlabel \"" . $plot_label[$plot_x-1] . "\"\n";
print GNUPLOT "set ylabel \"" . $plot_label[$plot_y-1] . "\"\n";
print GNUPLOT "set title \"$input_file: $title Summary\"\n";
print GNUPLOT "set output \"$png_file\"\n";
print GNUPLOT $plot;

print GNUPLOT "set term postscript eps enhanced color\n";
print GNUPLOT "set output \"$eps_file\"\n";
print GNUPLOT $plot;    

print GNUPLOT "save '$plot_file'\n";

close(GNUPLOT);

# Output to html file.
print HTML "<img src=\"$png_file_name\" /><br>\n";

print HTML "(<a href=\"$eps_file_name\">eps</a>)";
if ($keep) {
    print HTML "(<a href=\"$txt_file_name\">txt</a>)";
}
print HTML "(<a href=\"$plot_file\">gnuplot</a>)";

print HTML "<hr width=100%><p>\n";

close (HTML);
close (INPUT);

###############################################################################
# Delete the temporary files.
if (!$keep) {
  for ($i=0; $i<@out_array; ++$i) {
    if (!unlink($out_array[$i])) {
        print "Could not delete $out_file\n";
        exit(1);
    }
  }
}

print "manta_path_plot.pl Completed successfully in " . ((time()-$start_time)/60)  ." minutes.\n\n";

###############################################################################
# Process commands.
sub process_command {

    my $input_line = $_[0];
    $_ = $_[0];

    ###########################################################################
    # Check for a plot arg change.
    if (/^plot:(.*)/) {
        
        my $cmd = variable_replace( $1 );

        @plot_args = split(/ /,$cmd);
        my $i;
        for ($i=0;$i<@plot_args;++$i) {

            if ($plot_args[$i] eq "-path") {
                $path_file = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-sync") {
                $path_sync = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-delta_t") {
                $path_t = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-delta_time") {
                $path_time = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-interval") {
                $path_start = $plot_args[++$i];
                $path_last  = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-warmup") {
                $path_warmup = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-plot") {
                $plot_x  = $plot_args[++$i];
                $plot_y  = $plot_args[++$i];
            }
            elsif ($plot_args[$i] eq "-keep") {
                $keep = 1;
            }
            elsif ($plot_args[$i] eq "-nokeep") {
                $keep = 0;
            }
            elsif ($plot_args[$i] eq "-subtitle") {
                ++$i;
                my @strs;
                while (($i < @plot_args) && !($plot_args[$i] =~ /^-/)) {
                    push(@strs,$plot_args[$i++]);
                }
                $subtitle = join(" ",@strs);
            }
        }
    }

    ###########################################################################
    # Set command.
    elsif (/^set:[ \t]*([a-zA-Z\_]+)[ \t]*(.+)/) {
        my $var = $1;
        my $value = $2;

        # Save the variable.
        $variables{$var} = variable_replace($value);
    }

    ###########################################################################
    # System command.
    elsif (/^system:[ \t]*(.*)/) {

        my $command_line = $1;
        chomp($command_line);

        print "SYSTEM: $command_line\n";

        # Parse command line args.
        @system_args = split(/[ ]+/,variable_replace($command_line));

        # Run the command.
        if (system(@system_args)!=0) {
            # Exit on error.
            print "manta_path_plot.pl:\n";
            print "\t$command_line\n";
            print "\tTerminated code: $?\n";

            exit(1);
        }
    }

    ###########################################################################
    # Create a Macro.
    elsif (/^begin_macro:\s*(\w*)\((\w*)\)/) {
        
        # Initialize the macro.
        my $name = $1;
        my $macro = {
            "parameter" => $2,
            "commands" => []
        };

        # Collect input lines for the macro.
        my $macro_line;
        while (!(($macro_line = <INPUT>) =~ /^end_macro:/)) {
            chomp($macro_line);
            push( @{$macro{"commands"}}, $macro_line );
        }

        # Add to the macro hash.
        $macros{$name} = $macro;
    }

    # Run a macro
    elsif (/^macro:\s*(\w*)\(([0-9a-zA-Z_\.-]*)\)/) {

        my $name = $1;
        my $value = $2;

        # Check to see if the macro exists.
        my $macro = $macros{$name};
        if ($macro) {
            
            # print "Begin Macro\n";

            # Add the global variable.
            $variables{$$macro{"parameter"}} = $value;

            # Execute the macro.
            my $i;
            for ($i=0;$i<@{$macro{"commands"}};++$i) {
                
                $str = ${$macro{"commands"}}[$i];
                $cmd = variable_replace( $str );

                process_command( $cmd );
            }

            # Remove the parameter.
            $variables{$$macro{"parameter"}} = 0;

            # print "End Macro\n";
        }
        else {
            # Exit on error.
            print "manta_path_plot.pl:\n";
            print "\t$input_line\n";
            print "\tMacro not found: $name\n";

            exit(1);            
        }
    }

    ###########################################################################
    # Blank lines or comments
    elsif (/^[ \t]*\#/) {
        # Quietly skip
    }
    elsif (/^$/) {
        # Quietly skip
    }
    elsif (/^echo:\s*(.*)/) {
        print variable_replace( $1 ) . "\n";
    }

    ###########################################################################
    # Otherwise process the line as a command.
    else {

        $input_line = variable_replace( $input_line );

        # Check to see if a camera path file was specified.
        # If so append the manta camera path directive.
        if ($path_file ne "") {
            # Form the camera path command.
            $path_command = " -ui \"camerapath( -file $path_file -sync $path_sync -behavior exit -warmup $path_warmup ";
            if ($path_t != 0) {
                $path_command = $path_command . "-delta_t $path_t ";
            }
            if ($path_time != 0) {
                $path_command = $path_command . "-delta_time $path_time ";
            }
            if ($path_start != 0) {
                $path_command = $path_command . "-interval $path_start $path_last ";
            }
            
            $path_command = $path_command . ")\"";
        }

        # Record stat's on the run.
        $min_fps = 999;
        $max_fps = 0;
        $total_fps = 0;
        $total_samples = 0;

        $out_file = $output_prefix . $line . ".temp.txt";

        chomp($input_line);
        $command  = $input_line . $path_command; #  . ">& /dev/null";

        # Open a temporary output file.
        if (!open( TEMP, ">", $out_file )) {
            print "Could not open temp out file: $out_file\n";
            exit(1);
        }

        print "RUNNING COMMAND: $command\n";

        # Open a pipe to manta.
        if (!open( MANTA, "-|", $command )) {
            print "Could not execute line $line: $command\n";
            exit(1);
        }

        push(@command_array, $command);

        # Wait for input
        while (<MANTA>) {
            
            # Skip the first sample.
            if ($total_samples > 0) {

                chomp;
                @column = split(/ /);

                if (@column == $plot_columns) {

                    # print ("Frame: $column[0] fps: $column[4]\n");
                    print TEMP join(" ", @column) . "\n";
                
                    # Keep track of min and max.
                    if ($column[$plot_y-1] < $min_fps) {
                        $min_fps = $column[$plot_y-1];
                    }
                    if ($column[$plot_y-1] > $max_fps) {
                        $max_fps = $column[$plot_y-1];
                    }
            
                    # Keep track of average.
                    $total_fps = $total_fps + $column[$plot_y-1];
                }
                else {
                    $actual = @columns;
                    print "Manta output line $line contained $actual columns $plot_columns expected.\n";
                    --$total_samples;
                }
            }
            $total_samples++;
        }

        # Close the file handles.
        close(MANTA);
        close(TEMP);

        # Pipe commands to gnuplot.
        if (!open(GNUPLOT, "|-", "gnuplot")) {
            print "Could not open gnuplot\n";
            exit(1);
        }

        $png_file = $output_prefix . $line . ".png";

        @array = split(/\//,$png_file);
        $png_file_name = $array[@array-1];

        $eps_file = $output_prefix . $line . ".eps";

        @array = split(/\//,$eps_file);
        $eps_file_name = $array[@array-1];
        
        @array = split(/\//,$out_file);
        $txt_file_name = $array[@array-1];

        $plot_file = $output_prefix . $line . ".gnuplot";

        # Set the line subtitle.
        if ($subtitle eq "") {
            $subtitle = "$input_file:$line";
        }

        print GNUPLOT "set data style linespoints\n";
        print GNUPLOT "set xlabel \"" . $plot_label[$plot_x-1] . "\"\n";
        print GNUPLOT "set ylabel \"" . $plot_label[$plot_y-1] . "\"\n";
        print GNUPLOT "set title \"$input_file command $line\"\n";

#        print GNUPLOT "set term png small color\n";
        print GNUPLOT "set term png small\n";
        print GNUPLOT "set output \"$png_file\"\n";
        print GNUPLOT "plot \"$out_file\" using $plot_x:$plot_y title \"$subtitle\"\n";

        print GNUPLOT "set term postscript eps enhanced color\n";
        print GNUPLOT "set output \"$eps_file\"\n";
        print GNUPLOT "plot \"$out_file\" using $plot_x:$plot_y title \"$subtitle\"\n";
        print GNUPLOT "save '$plot_file'\n";
        
        close(GNUPLOT);

        # Output to html file.
        print HTML "<h2>$subtitle</h2>\n";
        print HTML "<font face=\"courier\" size=-1>$command</font><br>\n";

        if ($total_samples > 0) {
            print HTML "<img src=\"$png_file_name\" /><br>\n";
            
            print HTML "<br><pre>\n";
            print HTML "Min  fps: $min_fps\n";
            print HTML "Max  fps: $max_fps\n";
            print HTML "Median fps: " . (($min_fps+$max_fps)/2) . "\n";

            # Subtract one from total samples since we skipped the first one.
            print HTML "Mean  fps: " . ($total_fps / ($total_samples-1)) . "</pre><br>\n";

            print HTML "(<a href=\"$eps_file_name\">eps</a>)";
            if ($keep) {
                print HTML "(<a href=\"$txt_file_name\">txt</a>)";
            }


        }
        else {
            print HTML "Error: No samples.<p>\n";
        }
        print HTML "<hr width=100%><p>\n";

        push(@subtitle_array,$subtitle);
        push(@out_array,$out_file);

        push(@plot_x_array,$plot_x);
        push(@plot_y_array,$plot_y);

        # Clear the subtitle.
        $subtitle = "";

        ++$line;
    }

}


