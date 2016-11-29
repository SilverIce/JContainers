local testing = {}

local misc = jrequire 'testing.misc'

function testing.perform()

  return misc.testMultiple('Basic container tests', jrequire 'testing.basic')
  	and misc.testMultiple('Misc functionality tests', jrequire 'testing.jc-tests')

end

return testing
