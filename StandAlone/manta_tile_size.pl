#!/usr/bin/perl

use IPC::Open2;

# Perl script for testing different tile sizes.
# Use 
# cat manta_tile_size_1_to_32.txt | unu save -f nrrd | unu reshape -s 32 32 | unu rmap -m ~/data/colormap/rainbow3x12.nrrd | unu resample -s = 512 512 -k box | unu quantize -b 8 | unu save -f png -o tile.png
# to produce a nice colormapped image.
# 
# Abe Stephens abe@sgi.com

# Updated October 2005 to use PromptUI so that scene doesn't need to
# be reloaded between runs.

######################################################################
# Default args.
$np     = 1;
@min    = (1, 1);
@max    = (32, 32);
$file   = "-";
$manta  = "";
$path   = "";
$title  = "Manta tile size plot";
$nrrd_plot = 0;

# Path parameters.
$path   = "";
$path_start = 0;
$path_last  = 0;
$path_sync  = 1;

######################################################################
# Parse args.
for ($i=0;$i<@ARGV;++$i) {
    if ($ARGV[$i] eq "-min") {
        $res = $ARGV[++$i];
        if ($res =~ /(\d+)x(\d+)/) {
            $min[0] = $1;
            $min[1] = $2;
        }
        else {
            print "Bad resolution should be NxN\n";
        }
    }
    elsif ($ARGV[$i] eq "-max") {
        $res = $ARGV[++$i];
        if ($res =~ /(\d+)x(\d+)/) {
            $max[0] = $1;
            $max[1] = $2;
        }
        else {
            print "Bad resolution should be NxN\n";
        }
    }
    elsif ($ARGV[$i] eq "-o") {
        $file = $ARGV[++$i];        
    }
    elsif ($ARGV[$i] eq "-manta") {
        $manta = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-path") {
        $path = $ARGV[++$i];
    }    
    elsif ($ARGV[$i] eq "-interval") {
        $path_start   = $ARGV[++$i];
        $path_last    = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-sync") {
        $sync = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-title") {
        $title = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-plot") {
        $nrrd_plot = 1;
    }
}

# Determine output file.
if ($manta eq "") {

    if ($manta eq "") {
        print "-manta option missing\n";
    }

    if ($path eq "") {
        print "-path option missing\n";
    }

    # Print usage.
    print "Usage: manta_tile_size.pl -o <filename or -> -manta <manta_command> -path <camera_path>\n";
    print "\t-manta <string>   (Command to run usually bin/manta -np ...)\n";
    print "\t-path <file.txt>\n";
    print "\t-interval M N     (default entire path)\n";
    print "\t-sync N           (default 1)\n";
    print "\t-min NxN          (default 1x1)\n";
    print "\t-max NxN          (default 32x32)\n";
    print "\t-o <out_file>     (default stdout)\n";
    print "\t-title <string>   (html plot title)\n";
    print "\t-plot             (use unu to create a plot and html page)\n";

    exit(0);

}
elsif ($file ne "-") {
    if (!open( $out, "+>".$file )) {
        print "Could not open output file.\n";
        exit(0);
    }
}
else {
    $out = STDOUT;
}


# Invoke manta.
$command = "$manta -ui prompt";

if (!open2(MANTA_OUT,MANTA_IN, $command)) {
    print "Could not open manta: $command\n";
    exit(1);
}

# Create the camera path directive.
if ($path ne "") {
    $camera_path = "-ui camerapath( -file $path -sync $path_sync ";
    
    if ($path_last > 0) {
        $camera_path = $camera_path . "-interval $path_start $path_last ";
    }
    $camera_path = $camera_path . ")\n";
}

# Determine the total number of iterations.
$iter = 0;
$size_x = ($max[0]-$min[0]+1);
$size_y = ($max[1]-$min[1]+1);
$total = $size_x * $size_y;

$overall_max = 0.0;
$overall_min = 999.0;

$max_i = 0;
$max_j = 0;
$min_i = 0;
$min_j = 0;

for ($y=$min[1];$y<=$max[1];++$y) {

    for ($x=$min[0];$x<=$max[0];++$x) {
	
        # Create the image traverser directive.
        $imagetraverser = " -imagetraverser \"tiled( -tilesize $x"."x"."$y )\"\n";

        # Set the image traverser.
        print MANTA_IN $imagetraverser;

        # Run the camera path.
        print MANTA_IN $camera_path;

        $total_samples = 0;
        $total_fps     = 0;

        # Collect path output or automator complete method.
        current_run: while (<MANTA_OUT>) {

            chomp;
            $line = $_;

            # See if the output line contains the complete message.
            if ($line =~ /COMPLETE/) {
                last current_run;
            }
            elsif ($line =~ /^Benchmark completed/) {

                # Read the performance of the benchmark.
                if ($line =~ /(\d+\.\d+) frames per second/) {
                    $total_fps = $1;
                    $total_samples = 1;
                }
                else {
                    print "WARNING could not parse Benchmark result: $line\n";
                }

                last current_run;
            }

            # Check to see how many columns are in the output.
            if ($line =~ /^[0-9]+/) {
                
                @column = split(/[ \t]+/,$line);

                # Look for five columns.
                if ((@column == 5) && ($column[4] =~ /^[0-9]+[\.]?[0-9]?/)) {
                    $total_fps = $total_fps + $column[4];
                    $total_samples++;
                }
                
                # Otherwise output a warning.
                else {
                    print "WARNING Unexpected output: $line\n";
                }
            }
        }

        # Output the average fps.
        if ($total_samples > 0) {

            $average_fps = ($total_fps/$total_samples);
            print $out $average_fps . "\n";

            # Update overall min and max.
            if ($average_fps > $overall_max) {
                $overall_max = $average_fps;

                $max_i = $x;
                $max_j = $y;
            }
            elsif ($average_fps < $overall_min) {
                $overall_min = $average_fps;

                $min_i = $x;
                $min_j = $y;
            }

        }
        else {
            print "WARNING No samples collected.\n";
        }
        
        # Output progress
        print "PROGRESS: " . ($iter+1) . " of " . $total . 
             " (" . (($iter+1)/$total*100) . "%)\n";
        ++$iter;
    }
} 

# print MANTA_IN "-quit\n";
close MANTA_OUT;
close MANTA_IN;


# Produce a plot.
if ($file ne "-") {

    # Write out a temporary colormap file.
    $colormap = "$file-colormap.temp.txt";
    if (!open( TEMP, ">$colormap")) {
        print "Could not open $colomap.\n";
        exit( 1 );
    }

    # Output the rainbow colormap.
    print TEMP "0 0 255\n";
    print TEMP "0 102 255\n";
    print TEMP "0 204 255\n";
    print TEMP "0 255 204\n";
    print TEMP "0 255 102\n";
    print TEMP "0 255 0\n";
    print TEMP "102 255 0\n";
    print TEMP "204 255 0\n";
    print TEMP "255 234 0\n";
    print TEMP "255 204 0\n";
    print TEMP "255 102 0\n";
    print TEMP "255 0 0\n";

    close( TEMP );


    # Open html file.
    if (!open( HTML, ">$file.html")) {
        print "Could not open $file.html\n";
        exit(1);
    }

    print HTML "<head title=\"$title\"></head>\n";
    print HTML "<h1>$title</h1>\n";
    print HTML "Generated using:<br><font face=\"courier\" size=-1>manta_path_plot.pl " . join(" ",@ARGV) . "</font><p>\n";    


    # Form the unu command.
    $file_png = "$file.png";
    $command = "cat $file | " . 
               "unu save -f nrrd | " .
               "unu reshape -s $size_x $size_y | ". 
               "unu rmap -m $colormap | " .
               "unu flip -a 2 | " .
               "unu resample -s = 512 512 -k box | ". 
               "unu quantize -b 8 | unu save -f png -o $file_png\n";

    print HTML "<font face=\"courier\" size=-1>plot command: $command</font><p>\n";

    # Execute the command.
    system( $command );

    # Save the colormap.
    $colormap_png = "$file-colormap.png";
    $command = "cat $colormap | " . 
               "unu save -f nrrd | " .
               "unu reshape -s 3 1 12 | " .
               "unu flip -a 2 | " .
               "unu quantize -b 8 | unu save -f png -o $colormap_png\n";

    print $command;
    system( $command );

    $overall_max = sprintf( "%3.2f", $overall_max );
    $overall_min = sprintf( "%3.2f", $overall_min );


    print HTML "<table>\n";
    print HTML "<tr><td>$overall_min fps</td><td rowspan=3><img src=\"$file_png\"></td></tr>\n";
    print HTML "<tr><td><img src=\"$colormap_png\" height=512></td></tr>\n";
    print HTML "<tr><td>$overall_max fps</td></tr>\n";
    print HTML "</table>\n";

    print HTML "<font face=\"courier\" size=-1>\n";
    print HTML "Max average fps: $max_i x $max_j<br>\n";
    print HTML "Min average fps: $min_i x $min_j<br>\n";
    print HTML "</font>\n";
    
    close(HTML);
}







