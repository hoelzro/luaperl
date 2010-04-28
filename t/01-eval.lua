local perl = require 'perl'

is(1, perl.eval('1'), 'perl.eval(\'1\') should be 1!')
is(3.14, perl.eval('3.14'), 'perl.eval(\'3.14\') should be 3.14!')
is('foobar', perl.eval('"foobar"'), 'perl.eval(\'"foobar"\') should be \'foobar\'!')
