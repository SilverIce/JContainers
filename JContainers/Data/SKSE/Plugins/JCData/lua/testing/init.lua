local testing = {}

local misc = jrequire 'testing.misc'

function testing.perform()

  misc.performTest('Basic container tests', jrequire 'testing.basic')
  misc.performTest('Misc functionality tests', jrequire 'testing.jc-tests')

end

return testing
