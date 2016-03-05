# Judy Arrays for Node.js

[![Build Status](https://travis-ci.org/JerrySievert/node-judy.svg?branch=master)](https://travis-ci.org/JerrySievert/node-judy)

[Judy Arrays](http://judy.sourceforge.net/doc/10minutes.htm "Judy Arrays") allow for fast and memory efficient access to in-memory hash tables.  
Instantiation is slow, but continued use shows speed increases over native `v8::Array` implementation.  
Some code cleanup was done to allow for clean compilation with `g++` or `clang++`.

For more information, see [Faster (sometimes) Associative Arrays with Node.js](http://legitimatesounding.com/blog/Faster_sometimes_Associative_Arrays_with_Node_js.html).

This specific Judy implementation is based on work by [Karl Malbrain](https://github.com/malbrain) released via the New BSD license. ~~The original work is available [Here](http://code.google.com/p/judyarray/)~~.

## In a nutshell

- Implements Judy Arrays via C++
- Adds `set`/`get`/`delete` functionality

## Building

```sh
node-gyp configure build
```

## Testing and Benchmarking
There are a couple of tests, as well as a couple of benchmarks to compare
against native associative arrays.

Testing:

```sh
node tests/big.js
node tests/multiple.js
```

Benchmarking:

```sh
node bench/native.js
node bench/judy.js

node bench/native.js --count=2000
node bench/judy.js --count=2000
```

## Synopsis

Usage:

```javascript
var judy   = require('judy');

var arr = new judy.Judy();
arr.set("some key", "some value");

var value = arr.get("some key");

arr.delete("some key");
```
