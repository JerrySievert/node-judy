# Judy Arrays for Node.js

[Judy Arrays](http://judy.sourceforge.net/doc/10minutes.htm "Judy Arrays")
allow for fast and memory efficient access to in-memory hash tables.  This
specific Judy implementation is based on work by Karl Malbrain released via
the New BSD license.  The original work is available [Here](http://code.google.com/p/judyarray/).
Some code cleanup was done to allow for clean compilation with g++.

Instantiation is slow, but continued use shows speed increases over native
v8::Array implementation.

For more information, see [Faster (sometimes) Associative Arrays with Node.js](http://legitimatesounding.com/blog/).

## In a nutshell

- Implements Judy Arrays via C++
- Adds set/get/delete functionality

## Building
    node-waf configure build

## Testing and Benchmarking
There are a couple of tests, as well as a couple of benchmarks to compare
against native associative arrays.

Testing:

    cd tests
    node native.js
    node judy.js
    
    node native.js --count=2000
    node judy.js --count=2000

## Synopsis

Using:

    var judy   = require('judy');
    
    var arr = new judy.Judy();
    arr.set("some key", "some value");
    
    var value = arr.get("some key");
    
    arr.delete("some key");
