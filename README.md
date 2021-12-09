#This is a Serial Library created in C++ for Windows, using Windows API.

The following explanation is good to understand why WriteFile() in overlapped mode is faster than non-overlapped.

This was found as an answer to a question on a website:
https://groups.google.com/g/comp.os.ms-windows.programmer.win32/c/VrmME1pJ9Hw?pli=1

The ordinary writefile will only return once the data has been sent.
This is probably OK for most applications. You have to use 'overlapped'
I/O to make the function return faster, but writing a buffered program
is a different kettle of fish altogether. You may then run into buffer
and handshake problems, as the function will return BEFORE the data has
been sent, which will run for a while depending on the packet size and
UART speed.
At 9600 baud, it takes about 1ms to send 1 byte of info. Increasing the
speed to 38400 will make that 4 times faster. If the PC is fast and the
target can handle it, then you can try 115k baud. At the higher speed
the writefile will return faster, making the use of overlapped I/O less
of an issue. The speed will not increase linearly, due to the speed of
the PC, which is fixed. What you will get on the line, is bytes with
gaps between them, so at some point, increasing the speed of the UART
will not gain you any more real speed.

If you are sending packets, then increase the packet size to say 10
kilobytes, to improve efficiency, but then you need a good error check.
For small packets, an XOR sum is OK, for 1k packets, use CRC16 and for
larger packets, use CRC32.

