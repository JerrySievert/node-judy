var argv = require('optimist').argv
var Benchmark = require('Benchmark')
var judynode = require('../index')
var suite = new Benchmark.Suite()
var count = argv.count || 1000
var multiplier = argv.multiplier || 1
// add tests
suite.add('JudyNode#Inserts#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    judy.set('key: ' + i, randomString(i * multiplier))
  }
})
suite.add('JudyNode#Retrieves#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    var value = randomString(i * multiplier)
    judy.set('key: ' + i, value)
    var key = judy.get('key: ' + i)
    if (key !== value) {
      throw new Exception('Incorrect')
    }
  }
})
suite.add('JudyNode#RandomRetrieves#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    var value = randomString(i * multiplier)
    judy.set('key: ' + i, value)
  }
  for (var i = 0; i < count; i += 1) {
    var key = 'key: ' + Math.floor(Math.random() * count)
    var value = judy.get(key)
  }
})
suite.add('JudyNode#Misses#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    var value = randomString(i * multiplier)
    judy.set('key: ' + i, value)
    var key = judy.get(value)
    if (key !== undefined) {
      throw new Exception('Incorrect')
    }
  }
})
suite.add('JudyNode#Deletes#' + count, function () {
  var judy = new judynode.Judy()
  for (var i = 0; i < count; i += 1) {
    var value = randomString(i * multiplier)
    judy.set('key: ' + i, value)
    judy.delete('key: ' + i)
  }
})
suite.add('JudyNode#News#' + count, function () {
  var judy = new judynode.Judy()
}) // add listeners
  .on('cycle', function (bench) {
    console.log(String(bench))
  }) // run async
  .run(false)
function randomString (length) {
  var parts = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890'
  var str = ''
  for (var x = 0; x < length; x += 1) {
    var i = Math.floor(Math.random() * 62)
    str += parts.charAt(i)
  }
  return str
}
