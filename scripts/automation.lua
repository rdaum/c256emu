function c256emu.hex_dump(start_addr, num_bytes)
   buf = c256emu.peekbuf(start_addr, num_bytes)
   for byte=1, #buf, 16 do
      local chunk = buf:sub(byte, byte+15)
      io.write(string.format('%08X  ',start_addr + byte-1))
      chunk:gsub('.', function (c) io.write(string.format('%02X ',string.byte(c))) end)
      io.write(string.rep(' ',3*(16-#chunk)))
      io.write(' ',chunk:gsub('%c','.'),"\n") 
   end
end

