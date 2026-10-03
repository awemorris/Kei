// Real callback exceptions must preserve their values and leave the next operation usable.
function checkFailure(name, run) {
 var caught = false;
 try { run(); } catch (e) { caught = e === 'sentinel'; }
 print(name, caught, JSON.stringify({ok:[1,2]}) === '{"ok":[1,2]}', JSON.parse('{"ok":3}').ok === 3);
}
checkFailure('reviver', function(){ JSON.parse('{"a":[1,2]}', function(k,v){if(k==='1')throw 'sentinel';return v;}); });
checkFailure('replacer', function(){ JSON.stringify({a:[1,2]}, function(k,v){if(k==='1')throw 'sentinel';return v;},2); });
checkFailure('toJSON', function(){ JSON.stringify({a:{toJSON:function(){throw 'sentinel';}}},null,2); });
checkFailure('getter', function(){ var o={};Object.defineProperty(o,'a',{enumerable:true,get:function(){throw 'sentinel';}});JSON.stringify(o,null,2); });
checkFailure('list-getter', function(){ var a=['a'];Object.defineProperty(a,'0',{get:function(){throw 'sentinel';}});JSON.stringify({a:1},a); });
checkFailure('gap-coercion', function(){ var n=new Number(2);n.valueOf=function(){throw 'sentinel';};JSON.stringify({a:1},null,n); });
var invalid=['"\\u12"','"\\q"','"unterminated','"\u0001"','-','01','1.','1e+','{"a":}','[1,]','true x'];
var rejected=0;
for(var i=0;i<invalid.length;i++){try{JSON.parse(invalid[i]);}catch(e){if(e instanceof SyntaxError)rejected++;}}
print('invalid',rejected,invalid.length);
var cyclic={a:[]};cyclic.a.push(cyclic);var cycle=false;try{JSON.stringify(cyclic,null,2);}catch(e){cycle=e instanceof TypeError;}
print('cycle-recovery',cycle,JSON.stringify({a:undefined,b:[undefined,NaN]}) === '{"b":[null,null]}');
print('surrogates',JSON.stringify('\ud800') === '"\\ud800"',JSON.stringify('\udc00') === '"\\udc00"',JSON.parse('"\\ud800"').charCodeAt(0) === 0xd800);
