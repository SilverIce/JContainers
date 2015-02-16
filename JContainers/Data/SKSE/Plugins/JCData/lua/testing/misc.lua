local misc = {}

function misc.performTest(test_name, unsafe_func)

  if type(test_name) ~= 'string' then return false end

  local succeed, error_msg = xpcall(function()
      --assert(type(test_name) == 'string', "invalid test name type")
      assert(type(unsafe_func) == 'function', "invalid testing function type")
      unsafe_func()
    end,
    debug.traceback)

  print( 'test ' .. test_name .. (succeed and ' succeed' or (' failed: ' .. error_msg)) )
  return succeed
end

return misc
