--- Extensions to `math`
-- @module Math
-- Note: there are no bit shift. Use those Lua functions instead for 24 bits bitshift
-- 24 bits because only the 24 bits of the mantissa can be easily used in a 32 bit float
-- bitshift functions (<<, >> equivalent)
--
-- shift left
--     local function lsh(value,shift)
--         return (value*(2^shift)) % 2^24
--     end
--
-- shift right
--     local function rsh(value,shift)
--         return math.floor(value/2^shift) % 2^24
--     end

--- Returns x clamped to min and max boundaries.
-- @function math.clamp ( number x, number min, number max )
-- @treturn number clamped

--- Returns the length of the diagonal of an n-dimensional box (or the length of an n-component vector). Rather quick method that does not handle intermediate overflows/underflows nor is made for numerical stability.
-- @function math.diag
-- @number x1
-- @number[opt] x2
-- @number[opt] x3
-- @number[opt] xn and so on
-- @treturn number diagonal

--- Returns erf(x).
-- @function math.erf
-- @number x
-- @treturn number erf

--- Returns the length of hypotenuse of right angle triangle with sides x and y, equivalent to sqrt(x*x + y*y), but has better numerical stability and internally handles intermediate overflows/underflows, but is also slower.
-- @function math.hypot
-- @number x
-- @number y
-- @treturn number sqrt(x*x+y*y)

--- Returns linear interpolation between x and y with ratio a (x+(y-x)*a).
-- @function math.mix
-- @number x
-- @number y
-- @number a
-- @treturn number (x+(y-x)*a)

--- Returns x rounded to n decimals, if n is omitted or <=0, rounds to nearest integer. Note that Spring's Lua interpreter uses 32-bit floats for all numbers so max. precision is ~7 decimal digits.
-- @function math.round
-- @number x
-- @number decimals
-- @treturn number rounded

--- Returns 0 if x ==0, 1 if x > 0, -1 if x < 0 sgn(x).
-- @function math.sgn
-- @number x
-- @treturn number sign

--- Returns smoothstep(edge0, edge1, v).
-- @function math.smoothstep
-- @number edge0
-- @number edge1
-- @number v
-- @treturn number smoothstep

--- Returns the bitwise AND of all arguments. Only use up to 24 bit integers.
-- @function math.bit_and
-- @number a1
-- @number a2
-- @number[opt] a3
-- @number[opt] an
-- @treturn number i

--- Returns the bitwise OR of all arguments. Only use up to 24 bit integers.
-- @function math.bit_or
-- @number a1
-- @number a2
-- @number[opt] a3
-- @number[opt] an
-- @treturn number i

--- Returns the bitwise XOR of all arguments. Only use up to 24 bit integers.
-- @function math.bit_xor
-- @number a1
-- @number a2
-- @number[opt] a3
-- @number[opt] an
-- @treturn number i

--- Returns the bitwise NOT of the 24 bit integer argument.
-- @function math.bit_inv
-- @number a1
-- @treturn number i

--- Set each of the bits of a 24 bit integer. Returns result = result OR (1 << a1) OR (1 << a2) OR ...;)
-- @function math.bit_bits
-- @number a1
-- @number a2
-- @number[opt] a3
-- @number[opt] an
-- @treturn number i
