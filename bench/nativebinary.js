var fs = require('fs');
var argv = require('optimist').argv;
var Benchmark = require('Benchmark');
var suite = new Benchmark.Suite();
var count = argv.count || 1000;
var multiplier = argv.multiplier || 1;
var testImage = fs.readFileSync('./image.png');
// add tests
suite.add('NativeBinary#Inserts#' + count, function () {
  var arr = {};
  for (var i = 0; i < count; i += 1) {
    arr['key: ' + i] = testImage
  }
});
suite.add('NativeBinary#Retrieves#' + count, function () {
  var arr = {};
  for (var i = 0; i < count; i += 1) {
    arr['key: ' + i] = testImage;
    var key = arr['key: ' + i]
  }
});
suite.add('NativeBinary#RandomRetrieves#' + count, function () {
  var arr = {};
  for (var i = 0; i < count; i += 1) {
    arr['key: ' + i] = testImage
  }
  for (var i = 0; i < count; i += 1) {
    var key = 'key: ' + Math.floor(Math.random() * count);
    var value = arr[key]
  }
});
suite.add('NativeBinary#Misses#' + count, function () {
  var arr = {};
  for (var i = 0; i < count; i += 1) {
    arr['key: ' + i] = testImage;
    var key = arr.abc123;
    if (key !== undefined) {
      throw new Exception('Incorrect')
    }
  }
});
suite.add('NativeBinary#Deletes#' + count, function () {
  var arr = {};
  for (var i = 0; i < count; i += 1) {
    arr['key: ' + i] = testImage;
    delete arr['key: ' + i]
  }
});
suite.add('Native#News#' + count, function () {
  var arr = {}
})
.on('cycle', function (bench) { // add listeners
  console.log(String(bench))
})
.run(false) // run async
