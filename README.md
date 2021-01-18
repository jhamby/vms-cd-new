# OpenVMS Freeware "CD" (converted to C++)

[OpenVMS](https://wiki.vmssoftware.com/OpenVMS) has a very confusing
way of changing the current directory, compared to UNIX or Windows.
There's a popular freeware replacement for the CD command, which is
extremely featureful, but has the downside of being written in VAX
macro assembly language, which has to be compiled to native code
on the newer CPU architectures that VMS supports: Alpha, Itanium,
and soon x86-64.

I decided to try to translate the 22-year-old VAX MACRO
[source code](https://www.digiater.nl/openvms/freeware/v80/cd/)
to modern C++, using the same OpenVMS functions, but with a more
idiomatic coding style that will hopefully be educational to
people seeking to learn OpenVMS development.

This is still a work in progress, but I wanted to sync the repo with
the code that I've been working on, to encourage me to finish it.
The original code is checked in under `original-src`. The current
version compiles and echoes its arguments. I'm still translated
and wiring up the functionality, but you can see the C++ wrappers
that I've written so far to abstract the low-level VMS details.

References:

* OpenVMS [documentation](https://vmssoftware.com/products/documentation/)
* OpenVMS [coding examples](http://www.eight-cubed.com/examples.shtml)
