# Serial
C++ Data De/Serialization library

# Status
This is still very experimental code; as such use at your own risk. At this
point, it can serialize and deserialize data to/from an in-memory data
structure kelp.

While a functional version of JSON serialization and deserialization is one
of the goals, doing so quickly and supporting an programmer facing API
for things like config file data or memory efficient data importing is also
a goal.

# Features
* C++17 code
* Exception safe
* In-situ building of data in memory during deserialization.
* Full UTF-8 support.
* Intelligent selection of signed and unsigned integral types for numbers
instead of 64-bit doubles for in-memory representation, extending precision
for integral values to 63 signed or 64 unsigned bits.

# Planned Features
* Conversion of strings from UTF-8 to UTF-32 during deserialization.
* Fast serialization of integral values and floating pount values bases on
the Ryu algorithm.
* Support for a more administrator-friendly sytax format, similar to BIND's
config format.
* Support for some form of data schema and semantic validation.
* Useful extentions to the basic JSON specification, specifically extensions
for specifyin unicode characters more than 16 bits long, and the ascii
low-numbered control codes.
* Compressed iostream support.

# Buidling
This code currently is using the ninja build tool and uses ragel for state
machine generation. In addition, it has been written using GCC 9, which has
decent C++17 support at this point. (I believe Clang can also compile it, but
haven't tested that).
