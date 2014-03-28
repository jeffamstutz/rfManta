

$series = 1;
$key = "np";
$plot_type = "minmaxavg";
$outfile = "";
$title = "";
$use_errorbars = 0;
$plot_linear = 1;
$linear_plot_point = 0;
$start_labels = 0;
@use_series;

sub usage {

    print 
        "Usage: make_scaling_plot.pl -file <infile.html>\n" .
        "-file <infile.html>  -- Input file\n" .
        "-key <string>        -- Key to search for on x axis default \"np\"\n" .
        "-series <n>          -- Independent series inteleaved by n lines.\n" .
        "                        This means that the input is interleaved.\n" .
        "                        Default 1.\n" .
        "-use_series <0,2,..> -- Comma delimited string indicates which series\n" .
        "                        to use in output. default all\n". 
        "-labels <label0> <label1> ...\n" .
        "                     -- Specify a label for each series. (required)\n" . 
        "-plottype <string>   -- default minmaxavg\n".
        "-title <string>      -- Specify plot title.".
        "-errorbars           -- Display min and max with error bars\n".
        "-nolinear            -- Don't plot linear for each series.\n" .
        "-linlabels <i>    -- Show linear labels beginning with point i\n";
        "-linplotpt <i>       -- Data point to use for plotting linear scaling\n" .
        "                        Default 0\n";

}

# Form the plot command.
sub plot_minmaxavg {
    
    my @plots, $i;
    my $plot = "";


    # Set the xtics.
    my @tics;
    my $xmin =  999999;
    my $xmax = -999999;
    for (my $p=0;$p<(@{$data[0]});++$p) {
        my $tic = $data[0][$p][0];
        push(@tics,"\"$tic\" ". $tic );

        if ($tic < $xmin) {
            $xmin = $tic;
        }
        if ($tic > $xmax) {
            $xmax = $tic;
        }
    }
    $plot = $plot . "set xtics (" . join( ", ", @tics ) . ")\n";
    
    # Set the x range of the plot.
    my $range = ($xmax - $xmin);

    $plot = $plot . "set xrange [0:($xmax+$range*0.1)]\n";
    $plot = $plot . "set yrange [0:]\n";

    # Begin the plot command.
    $label = "";

    # Usual min/max/average plot.
    for ($i=0;$i<$series;++$i) {

        if (use_series($i)) {

            # Determine linear plots for average.
            my $x0 = 0;
            my $y0 = 0;
            my $x1 = $data[$i][$linear_plot_point][0];
            my $y1 = $data[$i][$linear_plot_point][3];
            
            my $m = ($y1-$y0) / ($x1-$x0);
            
            # Plot the actual data.
            $tmp_file = "$out_file.temp$i.txt";
            push(@plots, 
                 "\"$tmp_file\" using 1:4 title \"$labels[$i]\" with linesp lt 2 lw 3"
                 );
            
            # Check to see if linear scaling should be plotted also
            if ($plot_linear) {
                
                # Add labels for percent of linear at each point.
                for (my $p=$start_labels;$p<(@{$data[$i]});++$p) {
                    my $x = $data[$i][$p][0];
                    my $y = $m*($x-$x0)+$y0;
                    my $percent_of_lin = sprintf("%2.0f",(($data[$i][$p][3]/$y)*100));
                    
                    $label = $label . "set label \" $percent_of_lin%\" at $x,$data[$i][$p][3]\n";
                }
                
                push(@plots, 
                     "$m*(x-$x0)+$y0 notitle with lines 1"
                     );
            }
            
            # Check to see if we should use error bars.
            if ($use_errorbars) {
                push(@plots, 
                     "\"$tmp_file\" using 1:4:2:3 notitle with yerrorbars" 
                     );
            }   
        }
    }

    $plot = "$plot\n$label\nplot " . join( ", ", @plots ) . "\n";
   print "PLOT: $plot\n";

    return $plot;
}

sub use_series {
    my $s = $_[0];
    foreach (@use_series) {
        if ($_ == $i) {
            return 1;
        }
    }
    return 0;
}

# Parse args
for ($i=0;$i<@ARGV;++$i) {
    if ($ARGV[$i] eq "-file") {
        $input_file = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-series") {
        $series = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-useseries") {
        @use_series = split(/\,/, $ARGV[++$i]);
    }
    elsif ($ARGV[$i] eq "-labels") {
        ++$i;
        while (($i < @ARGV) && !($ARGV[$i] =~ /^-/)) {
            push(@labels,$ARGV[$i++]);
        }
    }
    elsif ($ARGV[$i] eq "-key") {
        $key = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-out") {
        $out_file = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-plottype") {
        $plot_type = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-title") {
        $title = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-errorbars") {
        $use_errorbars = 1;
    }
    elsif ($ARGV[$i] eq "-nolinear") {
        $plot_linear = 0;
    }
    elsif ($ARGV[$i] eq "-linplotpt") {
        $linear_plot_point = $ARGV[++$i];
    }
    elsif ($ARGV[$i] eq "-linlabels") {
        $start_labels = $ARGV[++$i];
    }
}

# Check to see if enough subtitles were provided.
if (@labels < $series) {
    print "Must provide a label for each series.\n";
    usage();
    exit(1);
}

if ($input_file eq "") {
    print "Specify and input file\n";
    usage();
    exit(1);
}

# Use the input file as a default output file.
$out_mode = ">";
if ($out_file eq "") {
    $out_file = $input_file;
    $out_mode = ">>";
}

# Determine which series to use.
if (@use_series == 0) {
    for (my $i=0;$i<$series;++$i) {
        push(@use_series,$i);
    }
}
else {
    print "Only using series: " . join(@use_series, ", ") . "\n";
}

# Open input file.
if (!open(INPUT,"<",$input_file)) {
    print "Cannot open $input_file.\n";
    exit(1);
}

# Open a temp files.
for ($i=0;$i<$series;++$i) {
    $tmp_file = "$out_file.temp$i.txt";
    if (!open($TEMP[$i],">",$tmp_file)) {
        print "Cannot open $tmp_file\n";
        exit(1);
    }
    # print "Temporary file: $tmp_file\n";
}

if ($title eq "") {
    # $title = "Processor Scaling: $input_file";
}

$line_number = 0;
while (<INPUT>) {
    $line = $_;
    
    if ($line =~ /$key\s*([0-9\.]+)/) {
        $np = $1;
    }

    if ($line =~ /^Min  fps: ([0-9\.]*)/) {
        $min = $1;
    }
    if ($line =~ /^Max  fps: ([0-9\.]*)/) {
        $max = $1;
    }
    if ($line =~ /^Mean  fps: ([0-9\.]*)/) {
        $mean = $1;


        my $data_index = $line_number/$series;
        my $series_index = $line_number%$series;

        # Save data point.
        $data[$series_index][$data_index] = [$np, $min, $max, $mean];

        # Output.
        print {$TEMP[$series_index]} "$np $min $max $mean\n";
        $line_number ++;

        
    }
};

close (TEMP);
close (INPUT);

# Plot in gnuplot.

if (!open(GNUPLOT, "|-", "gnuplot")) {
    print "Could not open gnuplot\n";
    exit(1);
}

$png_file  = $out_file . ".png";
$eps_file  = $out_file . ".eps";
$plot_file = $out_file . ".gnuplot";

print "Output to $out_file $png_file, $eps_file and $plot_file\n";

$plot = plot_minmaxavg();

# print "Plot command: $plot\n";

# print GNUPLOT "set style data linespoints\n";
print GNUPLOT "set title  \"$title\"\n";
print GNUPLOT "set xlabel \"$key\"\n";
print GNUPLOT "set ylabel \"fps\"\n";

print GNUPLOT "set term png small monochrome\n";
print GNUPLOT "set output \"$png_file\"\n";
print GNUPLOT $plot;

print GNUPLOT "set term postscript eps enhanced monochrome\n";
print GNUPLOT "set output \"$eps_file\"\n";
print GNUPLOT $plot;

print GNUPLOT "save '$plot_file'\n";

close (GNUPLOT);

# Append to the input file.
if (!open(HTML, $out_mode, $out_file)) {
    print "Cannot open $out_file.\n";
    exit(1);
}

print HTML "<h2>Scaling Plot</h2>\n";

print HTML "Generated using:<br><font face=\"courier\" size=-1>manta_scaling_plot.pl " . join(" ",@ARGV) . "</font><p>\n";

print HTML "<img src=\"$png_file\" /></br>\n";
print HTML "(<a href=\"$eps_file\">eps</a>)\n";
print HTML "(<a href=\"$plot_file\">gnuplot</a>)\n";

for ($i=0;$i<$series;++$i) {
    $tmp_file = "$out_file.temp$i.txt";
    print HTML "(<a href=\"$tmp_file\">txt series $i</a>)\n";
}

print HTML "<hr width=100%>\n";

close HTML;
