local misc = {}

function misc.performTest(test_name, unsafe_func)

  if type(test_name) ~= 'string' then return false end

  local succeed, error_msg = xpcall(function()
      --assert(type(test_name) == 'string', "invalid test name type")
      assert(type(unsafe_func) == 'function', "invalid testing function type")
      unsafe_func()
    end,
    debug.traceback)

  print( 'test', test_name, (succeed and 'succeed' or (' failed: ' .. error_msg)) )
  return succeed
end

local tableLength = function(tbl)
	local len = 0
	for _, _ in pairs(tbl) do
		len = len + 1
	end
	return len
end

function misc.testMultiple(set_name, functions)
	print('Start a set', set_name, 'of', tableLength(functions), 'functions')

	local performTest = misc.performTest
	local all_succeed = true
	for test_name, test_func in pairs(functions) do
		if not performTest(test_name, test_func) then all_succeed = false end
	end
	--print('Start a set', set_name, 'of', #functions, 'functions')
	return all_succeed
end

return misc
