local perl = require 'perl'

assert(perl.require 'Some::Core::Module')
table.insert(package.loaders, perl.require)

--# make sure this runs w/o exception
require 'Another::Core::Module'
