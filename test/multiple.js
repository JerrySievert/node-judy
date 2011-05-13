var vows   = require('vows');
var assert = require('assert');
var judy   = require('../judy');

vows.describe('Judy').addBatch({
    'can set and retrieve a value': {
        topic: function () {
            var topic = new judy.Judy();
            topic.set("foo", "bar");
            return topic;
        },
        'correct value is returned from get': function (topic) {
            assert.equal(topic.get("foo"), "bar");
        },
        'other gets do not return the same value': function (topic) {
            assert.equal(topic.get("bar"), undefined);
        }
    },

    'can set and retrieve multiple values': {
        topic: function () {
            var topic = new judy.Judy();
            topic.set("foo", "bar");
            topic.set("bar", "baz");
            return topic;
        },
        'correct value is returned from get': function (topic) {
            assert.equal(topic.get("foo"), "bar");
            assert.equal(topic.get("bar"), "baz");
        },
        'other gets do not return the same value': function (topic) {
            assert.equal(topic.get("baz"), undefined);
        }
    },

    'delete works': {
        topic: function() {
            var topic = new judy.Judy();
            topic.set("foo", "bar");
            topic.set("bar", "baz");
            return topic;
        },
        'delete successful': function (topic) {
            topic.delete('bar');
            assert.equal(topic.get('bar'), undefined);
            assert.equal(topic.get('foo'), 'bar');
        }
    },
    
    'multiple copies will not collide': {
        topic: function() {
            var topic = new judy.Judy();
            topic.set("foo", "bar");
            return topic;
        },
        'setting second copy works': function (topic) {
            var comp = new judy.Judy();
            comp.set("foo", "baz");
            assert.equal(topic.get('foo'), 'bar');
            assert.equal(comp.get('foo'), 'baz');
        }
    },
    
    'keys returns expected results': {
        topic: function() {
            var topic = new judy.Judy();
            topic.set("foo", "bar");
            topic.set("bar", "baz");
            return topic;
        },
        'returns two values in the array': function (topic) {
            var keys = topic.keys();
            assert.equal(keys.length, 2);
        },
        'returns correct values in the array': function (topic) {
            var keys = topic.keys();
            if (keys[0] === 'foo') {
                assert.equal(keys[1], 'bar');
            } else {
                assert.equal(keys[1], 'foo');
            }
        }
    }
}).run();