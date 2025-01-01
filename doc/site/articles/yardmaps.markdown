---
layout: post
title: Yardmaps and map squares
parent: Articles
permalink: articles/yardmaps
author: sprunk
---

### Squares
The map is divided into a **discrete grid of 8x8 elmo squares**. Most terrain-related things (heightmap, parts of pathing, resource map aka metalmap, typemap, building masks, radar coverage...) work on these squares. Some of those are discretized further into 16x16 elmo squares, i.e. a 2x2 box of basic game squares. In particular, unit footprints are defined in big squares (also called a footprint square for that reason).

### Footprint
Buildings and mobile units have their footprint (in other words, size on the building grid) defined differently. **Mobile units define it in their move def**, and it has to be a square. In theory you can define a footprint in the unit def, but this is just used for construction.

**Buildings define it in their unit def**. This can be a rectangle, and you can define parts of the footprint to allow pathing (useful for factory exit yards), buildable (for when a building doesn't occupy the whole square), change meaning when the unit activates (for some sort of opening animation) or require a geothermal vent underneath. Such definition is called a **yardmap, and consists of a string arranged visually to depict the building**, with characters conveying some meaning as listed in the table at the bottom.

For example, consider the following yardmap. The `o` depicts the actual body area, while `y` is open space, and it corresponds to a diamond shape within the 5x5 footprint:
```
yyoyy
yoooy
ooooo
yoooy
yyoyy
```

And here's what happens if you try to place another of the same type on top. Notice how the `y` area does not produce conflicts.
![image]({{ site.baseurl }}/assets/guides/article-yardmap-solars-1.png)

Here's how it looks if you try to place things on top:
![image]({{ site.baseurl }}/assets/guides/article-yardmap-solars-2.png)

Essentially, the building is shaped like this (mock-up):
![image]({{ site.baseurl }}/assets/guides/article-yardmap-solars-3.png)

### Yardmap characters

Basic characters:

| letter | buildable if active? | pathable if active? | buildable if INactive? | pathable if INactive? | stackable? | need geo? |
|--------|----------------------|---------------------|------------------------|-----------------------|------------|-----------|
| o      | ❌                    | ❌                   | ❌                      | ❌                     | ❌          | ❌         |
| y      | ✅                    | ✅                   | ✅                      | ✅                     | ❌          | ❌         |
| c      | ✅                    | ✅                   | ❌                      | ❌                     | ❌          | ❌         |
| g      | ❌                    | ❌                   | ❌                      | ❌                     | ❌          | ✅         |
| i      | ❌                    | ❌                   | ✅                      | ✅                     | ❌          | ❌         |
| j      | ❌                    | ❌                   | ❌                      | ❌                     | ✅          | ✅         |
| s      | ❌                    | ❌                   | ❌                      | ❌                     | ✅          | ❌         |
| b      | ✅                    | ❌                   | ✅                      | ❌                     | ❌          | ❌         |
| u      | ❌                    | ✅                   | ❌                      | ✅                     | ❌          | ❌         |
| e      | ❌                    | ⚠️ exit only         | ❌                      | ⚠️ exit only           | ❌          | ❌         |

Others:

 * `h` has a special meaning - it **has to be the first letter and means a high resolution yardmap**,
i.e. there have to be 4x more characters and each will correspond to a "regular" 8x8 elmo game square instead of the larger 16x16 elmo footprint square.
 * `w`, `x`, `f`:  deprecated, same as `o`.
 * **whitespace is ignored**, which you can use to neatly **arrange the rectangle visually**.
 * anything else is ignored as well but may be used in the future.
