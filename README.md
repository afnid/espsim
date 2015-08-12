# espsim
ESP8266 Simulator (Allows Native Code to Link/Run on Linux)

<blockquote>
Allows native esp8266 code to be be cross-compiled to run native on a host computer (currently linux).

<p>
I am releasing this because it has proven useful for me and has become integral to my development process.

<p>
<ol>
<li>Note, this was only used against a single application and it was done quick & dirty to get it to bootstrap and run.  Mileage will vary.</li>
<li>This is not production level code, there are likely gaps and potential differences in my implementation relative to embedded code.</li>
<li>If your not that experienced with C/C++, you might want to wait and see if this project matures some before trying to get it to port.</li>
<li>Porting to other platforms may be a little involved (network.cpp), I will likely do a mac port, not too motivated on cygwin.</li>
<li>Looking for contributions, with a little work and more testing, this could become useful for a lot more people.</li>
<ol>
</blockquote>

<h2>The Good</h2>

I was able to use this to do some unit/stress testing on some embedded code using valgrind.
Later I added the espconn networking support so I could again use valgrind with much further coverage.
Currently I use this for networking/application code before flashing to an embedded device.

<ul>
<li>Allows esp8266 code to be run with native tools like gdb and valgrind.</li>
<li>Networking is implemented allowing a native esp8266 process to blend with networked devices.</li>
<li>Emulation of flash and rtc memory storage implemented in a way to fail fast on boundary errors.</li>
<li>Limited options for setting the heap size, tracking memory allocations/frees, and tracing network.</li>
<li>Provides an esp.h header file that works on Linux and for esp8266 c++ code</li>
<li>Was developed using the 1.2.0 sdk, and works equally well with 1.3.0.</li>
<li>A C++ compliant esp.h that has all the missing prototypes I ran into so far</li>
</ul>

<h2>The Bad</h2>

There are some issues getting this to work, mostly because this needs to be built using a 32-bit environment..

<ul>
<li>The esp8266 is 32 bit, so compiling on a 64-bit host caused several issues with sizes of longs and pointers.</li>
<li>The system task post arg is defined as a 32 bit value, you can set it to a pointer since they are also 32 bits, not on 64-bit os.</li>
<li>Definitely does not handle simulation of off-chip devices, more suited for testing systems programming code.</li>
<li>Had to make a couple of manual changes to the esp8266 header files to make this work correctly.</li>
<li>Has not been ported to any other platforms other than a 64-bit ubuntu/mint installation.</li>
<li>Need to add this function to your code in order to get tty input: void uart_hook(char ch) {}</li>
<li>valgrind reports some issues in my networking code at shutdown, should clean that up.
<li>Probably needs a ton more options to set/override runtime defaults.
</ul>

<h2>The Ugly</h2>

I run 64-bit Ubuntu/Mint environment system, and has had a lot of packages already installed at this point.
You could use virtualbox to run a 32-bit environment, or what I did was carefully add i386 libraries until I got it to link.
Looking through packages installed I see libc6-dev-i386, libc6-i386, g++-4.8-multilib, valgrind-i386 (removes the 64 bit version!).
Be careful of anything that wants to replace all of your shared libraries with 32 bit versions.

<p>
Maybe it can work on a 64-bit install but the required changes looked a little more daunting.

<p>
The following headers were changed:

<h3>eagle_soc.h</h3>

Around line 45, change the following to stop the gpio calls from segfaulting:

<p>Replace</p>

<code>
#define ETS_UNCACHED_ADDR(addr) (addr)
<br>#define ETS_CACHED_ADDR(addr) (addr)
</code>

<p>with:</p>

<code>
// LINUX
<br>#ifndef ETS_UNCACHED_ADDR
<br>#define ETS_UNCACHED_ADDR(addr) (addr)
<br>#endif
<br>
<br>#ifndef ETS_CACHED_ADDR
<br>#define ETS_CACHED_ADDR(addr) (addr)
<br>#endif
</code>

<h3>c_types.h</h3>

<p>Replace</p>

<code>
typedef signed long         int32_t;
</code>

<p>with:</p>

<code>
<br>#ifndef __uint32_t_defined
<br>typedef signed long         int32_t;
<br>#endif
</code>
