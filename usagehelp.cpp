const char* UsageHelp={"\
bettercopy - a command-line file copy tool\r\n\
For source code and license details: https://www.github.com/jgmamxmn/bettercopy\r\n\
\r\n\
Usage: BETTERCOPY <input file> <output file> [options]\r\n\
\r\n\
Options:\r\n\
\r\n\
  -v n|r|s    Specify the type of verification used:\r\n\
                n   None: just copy the file. Don't verify data.\r\n\
                r   Runtime: check data chunk-by-chunk, for data read and\r\n\
                    length written.\r\n\
                s   Successive: after file is totally written, compare source\b\
                    and destination byte-by-byte.\r\n\
\r\n\
   -c <size>  Specify the size, in bytes, of each data chunk.\r\n\
              Default: 256\r\n\
              Higher values may be faster across a network.\r\n\
\r\n\
   -r <count> Specify the number of retries following read/write errors before\r\n\
              bettercopy \"gives up\".\r\n\
              Default: 3\r\n\
              After this number has been reached, bettercopy will perform the\r\n\
              action configurable through -a.\r\n\
\r\n\
   -a p|x|i   Specify what to do if the maximum number of retries continues to\r\n\
              produce a read/write error:\r\n\
                p   Prompt the user for action\r\n\
                x   Exit\r\n\
                i   Ignore the error and continue through the file\r\n\
              Default: p\r\n\
\r\n\
   -o n|l|s|# Control the verbosity of bettercopy:\r\n\
                n   Normal output, including progress bar\r\n\
                l   Limited output (ie updated less frequently)\r\n\
                s   Silent\r\n\
                #   Use an integer to specify the number of seconds between\r\n\
                    updates.\r\n\
              Default: n\r\n\
              Reducing the verbosity can increase performance.\r\n\
\r\n\
"};