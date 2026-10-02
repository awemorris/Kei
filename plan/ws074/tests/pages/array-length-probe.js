// zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
function check(label, condition) { print((condition ? 'PASS ' : 'FAIL ') + label); }
var a = [], rhs = '2147483648', answer = (a.length = rhs);
check('assignment preserves string RHS', answer === rhs && typeof answer === 'string' && a.length === 2147483648);
check('large length has no invented own elements', Object.keys(a).length === 0 && a[100] === undefined);
var valid = [0, 1, 2147483647, 2147483648, 4294967295];
for (var i=0; i<valid.length; i++) {
  a.length = valid[i];
  check('valid assignment boundary ' + i, a.length === valid[i]);
  check('valid constructor boundary ' + i, Array(valid[i]).length === valid[i] && new Array(valid[i]).length === valid[i]);
}
var invalid = [-1, 1.5, 4294967296, Infinity, -Infinity, NaN, undefined];
for (var i=0; i<invalid.length; i++) {
  a.length = 3;
  var caught = false; try { a.length = invalid[i]; } catch(e) { caught = e instanceof RangeError; }
  check('invalid assignment preserves length ' + i, caught && a.length === 3);
  if (i < 6) {
    caught = false; try { Array(invalid[i]); } catch(e) { caught = e instanceof RangeError; }
    check('invalid numeric constructor ' + i, caught);
  }
}
check('string constructor remains one element', Array('2147483648').length === 1 && Array('2147483648')[0] === '2147483648');
var boxed = new Number(2147483648);
check('boxed number constructor remains one element', Array(boxed).length === 1 && Array(boxed)[0] === boxed);
a.length = null; check('null length conversion', a.length === 0);
a.length = true; check('boolean length conversion', a.length === 1);
var calls = '', source = {valueOf:function() { calls += 'n'; return 2147483648; }};
answer = (a.length = source);
check('two numeric conversions and original RHS identity', calls === 'nn' && answer === source && a.length === 2147483648);
var sentinel = {}, same = false;
try { a.length = {valueOf:function() { throw sentinel; }}; } catch(e) { same = e === sentinel; }
check('coercion exception preserved', same && a.length === 2147483648);
calls = 0; same = false;
try { a.length = {valueOf:function() { calls++; return calls === 1 ? 1 : 2; }}; } catch(e) { same = e instanceof RangeError; }
check('conversion comparison order', same && calls === 2 && a.length === 2147483648);
Object.defineProperty(a,'length',{value:'4294967295'});
check('descriptor accepts full unsigned length', a.length === 4294967295 && Object.getOwnPropertyDescriptor(a,'length').value === 4294967295);
a[0] = 'near'; a[2000000000] = 'far';
check('existing signed sparse index after huge length', a.length === 4294967295 && a[0] === 'near' && a[2000000000] === 'far' && Object.keys(a).length === 2);
a.length = 1;
check('large length contraction removes sparse keys', a.length === 1 && a[0] === 'near' && a[2000000000] === undefined);
a.length = -0;
check('negative zero length normalized', a.length === 0 && 1/a.length === Infinity);
Object.defineProperty(a,'length',{value:2147483648,writable:false});
check('large readonly length descriptor', Object.getOwnPropertyDescriptor(a,'length').writable === false && a.length === 2147483648);
a.length = 4;
check('sloppy readonly assignment refused', a.length === 2147483648);
same = false; try { (function(){ 'use strict'; a.length = 4; })(); } catch(e) { same = e instanceof TypeError; }
check('strict readonly assignment throws', same && a.length === 2147483648);
