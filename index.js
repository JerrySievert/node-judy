/*
    © 2011-2013 by Jerry Sievert
*/
var NativeExtension = require('bindings')('judy');
function Judy(size) {
  size = size || 1024;
  if (size > 1024) {
    return undefined
  }
  return new NativeExtension.JudyNode(size)
}
Judy.prototype.get = function (key) {
  return this.get(key)
};
Judy.prototype.set = function (key, value) {
  return this.set(key, value)
};
Judy.prototype.delete = function (key) {
  return this.delete(key)
};
module.exports.Judy = Judy
