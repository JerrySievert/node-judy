var fs = require('fs')
var argv = require('optimist').argv
var Benchmark = require('Benchmark')
var judynode = require('../index')
var suite = new Benchmark.Suite()
var count = argv.count || 1000
var testImage = fs.readFileSync('./image.png')
// add tests
suite.add('JudyNodeBinary#Inserts#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, testImage)
  }
})
suite.add('JudyNodeBinary#Retrieves#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, testImage)
    var key = judy.get('key: ' + i)
  }
})
suite.add('JudyNodeBinary#RandomRetrieves#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, testImage)
  }
  for (var i = 0; i < count; i += 1) {
    var key = 'key: ' + Math.floor(Math.random() * count)
    var value = judy.get(key)
  }
})
suite.add('JudyNodeBinary#Misses#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, testImage)
    var key = judy.get('abc123')
    if (key !== undefined) {
      throw new Exception('Incorrect')
    }
  }
})
suite.add('JudyNodeBinary#Deletes#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, testImage)
    judy.delete('key: ' + i)
  }
})
suite.add('JudyNodeBinary#News#' + count, function () {
  var judy = new judynode.Judy()
}) // add listeners
  .on('cycle', function (bench) {
    console.log(String(bench))
  }) // run async
  .run(false)
