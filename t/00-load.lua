local perl = require 'perl'
ok(perl, 'require "perl" is nil!')
type_ok('userdata', perl, 'require "perl" should return a userdatum!')
is(package.loaded.perl, perl, 'require "perl" should return package.loaded.perl)
