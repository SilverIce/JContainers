local testing = {}

local misc = jrequire 'testing.misc'

function testing.perform()

  return misc.performTest('Basic container tests', jrequire 'testing.basic')
  and misc.performTest('Misc functionality tests', jrequire 'testing.jc-tests')

end

return testing
