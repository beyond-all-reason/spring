---
layout: post
title: CEG operators
parent: Articles
permalink: articles/ceg-operators
author: sprunk
---

## Basics

You can customize some CEG entries by specifying a series of operators instead of just a number.
This lets you apply limited logic to customize explosions.
Each operation is specified by a single string.

### Raw numbers

A single number means just that number. So for example:
```lua
sizeGrowth = "3",
```
This sets `sizeGrowth` to 3. So far so simple.

Some parameters, especially 3D vectors, accept multiple values. In that case, we separate then with the `,` (comma) operator.
For example:
```lua
pos = "1, 2, 3",
```
This sets the position to x=1, y=2, and z=3.

### Running value

There is an implicit running value, which starts at 0 and on which all the operators work.
A raw number is actually an "operatorless" operator that performs simple addition to that running value.
So, one could think of the examples above as really being `0 +3` for `sizeGrowth` and `0 +1`, `0 +2`, and `0 +3` as the components of the `pos` vector respectively.


To illustrate how this matters, consider this example. Note that there is no `,` between these!
```lua
sizeGrowth = "1 2 3",
```
The result of this is 6. This is because each of these is addition: `0 +1 +2 +3`, which nets 6.

Similarly:
```lua
pos = "1 2, 3 4, 5 6",
```
This sets the components to 3 (0 +1 +2), 7 (0 +3 +4) and 11 (0 +5 +6) respectively.


Putting a bunch of raw numbers next to each other doesn't make much sense, since we could have just written the sum directly, but the principle starts to matter when you mix operators.

### Random (`r`)

The `r` operator also works on the running value, but adds a random value between 0 and the operand.
So, for example, `r4` gives a random value between 0 and 4. Practical hints:
 * very useful to make explosions look less artificial.
 * if you don't want to roll from 0, then just add the offset (remember a raw operatorless number performs addition).
So, `3 r4` gives a value between 3 and 7.
 * a common desire is to roll negative values, for example so that a directional particle can go either way.
In that case, also use the offset. A common idiom is to use, for example, `-15 r30` to roll ±15.
 * the value is distributed uniformly, but with some knowledge of statistics you could tweak it by stacking rolls.
For example `r6` produces a flat uniform distribution, `r3 r3` a sort of triangle, and `r2 r2 r2` something smoother still.
In practice this seems very underused though, and you can't make the distribution asymmetrical via this basic method,
though you can via the more advanced ones below.

### Index (`i`)

The `i` operator multiplies its operand by the index of the particle, and adds this to the running value.
When a generator spawns multiple particles of the same kind, each one is assigned an increasing index: 0, 1, 2, etc.

For example, if an explosion spawns 4 particles and `size = "3 i2"`, then they will have sizes of 3, 5, 7 and 9 respectively.

 * useful for spawning stuff in something resembling a line or cone.
 * useful for scaling explosions via particle count, since the low-index particles will behave the same as before.
 * remember this doesn't multiply the running value, or anything other than the operand.

### Damage (`d`)

The `d` operator multiplies its operand by the "damage" of an explosion.
For example `d0.1` will net 10 for a 100-damage explosion, and 50 for a 500-damage explosion.

Some practical remarks:
 * for regular weapons, this is the "default" damage. Beware if you treat it as the "features" armor class (since they can't have a real armor class)!
 * for CEG trails, damage is the TTL of the projectile. So you can for example make missile trails burn out.
 * for explosions spawned by unit scripts, this defaults to 0 but you can set it to an arbitrary value.
 * existing games prefer to have a separate effect for each similar weapon, so this is quite an uncommon operator, but if made to work could work wonders for consistency.

## Advanced

In addition to the running value, you have an access to a buffer with 16 "slots" whither you can save values for later use.
Other than allowing complex math, these let you reuse a value for multiple components of a vector (across the `,` boundary which normally resets the running value).
There are also some operators that involve more complex values than just addition.

### Yank (`y`), add (`a`), and multiply (`x`)

The `y` operator saves ("yanks") the current running value to the buffer under given index, and resets the running value to 0.
The `a` operator adds to the current running value from given buffer.
The `x` operator multiplies the running value by the value of given buffer.

Examples:
 * `10 r20 y7 5 r10 x7`. Rolls a random value 10-30 (see earlier lesson), saves it to buffer #7 (which resets running value to 0), rolls a different one 5-15, then multiplies it by the value of the contents of buffer #7 (i.e. the previous roll). The `foo yN bar xN` pattern is how you multiply things in general.
 * `r10 y0 a0, 0, a0`. Rolls a value, saves it, loads it right back because of the reset. Reuses the value for the third component of the vector. This is how you can get diagonal values.

### Sinus (`s`)

The `s` operator treats its operand as an amplitude and the current running value as the phase, and _replaces_ the current running value with the result.
For example `3 s2` is about 0.28, because that's `2 * sin(3 radians)`.

 * only really makes sense with sources of unpredictability such as `r`, `i`, or `d`.
 * there is no separate cosinus operator, but you can make a ghetto cosinus via `cos(x) = sin(π/2 + x)`, i.e. just do `1.57 sX` instead of just `sX`.
 * good for making circular or spherical volumetric effects (for non-volumetric there's basic spread parameters like `emitRot`).

### Sawtooth/modulo (`m`) and discretize (`k`)

The `m` operator applies the modulo operator to the running value. The `k` operator truncates the running value to a multiple of the operand. So:
 * `1 k7` is 0, `1 m7` is 1
 * `6 k7` is 0, `6 m7` is 6
 * `7 k7` is 7, `7 m7` is 0
 * `8 k7` is 7, `8 m7` is 1
 * `13 k7` is 7, `13 m7` is 6
 * `14 k7` is 14, `14 m7` is 0

### Power (`p`) and power buffer (`q`)

The `p` operator raises the running value to the operandth power. The `q` operator is similar but takes the power from given buffer. The main use case is probably for getting x² or √x. Examples:
 * `3p4` is 81, since that's 3⁴.
 * `4y7 3q7` is also 81 (and leaves the 7th buffer slot with the value of 4).

## Table

Notation: V is the running value, X is the operand, and B denotes the buffer.

<table>
  <tr>
    <th>operator</th>
    <th>effect</th>
  </tr>
  <tr>
    <td>(none)</td>
    <td>V += X</td>
  </tr>
  <tr>
    <td>r</td>
    <td>V += random(0; X)</td>
  </tr>
  <tr>
    <td>i</td>
    <td>V += X * index</td>
  </tr>
  <tr>
    <td>d</td>
    <td>V += X * damage</td>
  </tr>
  <tr>
    <td>y</td>
    <td>B[X] = V<br/>V = 0</td>
  </tr>
  <tr>
    <td>a</td>
    <td>V += B[X]</td>
  </tr>
  <tr>
    <td>x</td>
    <td>V *= B[X]</td>
  </tr>
  <tr>
    <td>s</td>
    <td>V = X * sin(V)</td>
  </tr>
  <tr>
    <td>m</td>
    <td>V = V % X</td>
  </tr>
  <tr>
    <td>k</td>
    <td>V = floor(V / X) * X</td>
  </tr>
  <tr>
    <td>p</td>
    <td>V = V<sup>X</sup></td>
  </tr>
  <tr>
    <td>q</td>
    <td>V = V<sup>B[X]</sup></td>
  </tr>
  <tr>
    <td>,</td>
    <td>result = V<br/>V = 0</td>
  </tr>
</table>
