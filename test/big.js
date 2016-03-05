var vows = require('vows');
var assert = require('assert');
var judy = require('../judy');
vows.describe('Judy').addBatch({
  'can set and retrieve large numbers of values': {
    topic: function () {
      var topic = new judy.Judy(1);
      for (var i = 0; i < 10000; i += 1) {
        topic.set('key: ' + i, 'value: ' + i)
      }
      return topic
    },
    'correct value is returned from get': function (topic) {
      for (var i = 0; i < 10000; i += 1) {
        assert.equal(topic.get('key: ' + i), 'value: ' + i)
      }
    }
  },
  'can set and retrieve huge numbers of values': {
    topic: function () {
      var topic = new judy.Judy(10);
      for (var i = 0; i < 100000; i += 1) {
        topic.set('key: ' + i, 'value: ' + i)
      }
      return topic
    },
    'correct value is returned from get': function (topic) {
      for (var i = 0; i < 100000; i += 1) {
        assert.equal(topic.get('key: ' + i), 'value: ' + i)
      }
    }
  },
  'can set and retrieve humungous numbers of values': {
    topic: function () {
      var topic = new judy.Judy(100);
      for (var i = 0; i < 1000000; i += 1) {
        topic.set('key: ' + i, 'value: ' + i)
      }
      return topic
    },
    'correct value is returned from get': function (topic) {
      for (var i = 0; i < 1000000; i += 1) {
        assert.equal(topic.get('key: ' + i), 'value: ' + i)
      }
    }
  },
  'can set and retrieve ginormous numbers of values': {
    topic: function () {
      var topic = new judy.Judy(100);
      for (var i = 0; i < 10000000; i += 1) {
        topic.set('key: ' + i, 'value: ' + i)
      }
      return topic
    },
    'correct value is returned from get': function (topic) {
      for (var i = 0; i < 10000000; i += 1) {
        assert.equal(topic.get('key: ' + i), 'value: ' + i)
      }
    }
  }
}).run()
