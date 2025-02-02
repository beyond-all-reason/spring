---
layout: default
title: Lua API
nav_order: 5
has_children: true
has_toc: false
permalink: lua-api
---

# Lua API

{: .note }
> We have recently moved to [Lua Language Server](https://luals.github.io/) for Lua support in IDE. This documentation is generated from its doc export function, which is quite limited. Currently all docs are inluded on a single page, and some information is missing.
>
> See the [Lua Language Server guide](guides/lua-language-server.markdown) for more information.

{% for row in site.data.doc %}

{% assign defines = row["defines"][0] %}

{% comment %}
  Determine the type of the item. This is complicated by functions being defined
  as `variables` which are extending a function.
{% endcomment %}

{% assign type = row.type %}
{% if type == "variable" %}
  {% assign type = defines.extends.type %}
{% endif %}

---

{% comment %}
  --- Header ---

  Render the name of item, including its type.
  Then render the first line of its "view" - the raw code definition. This will
  show the full function signature without its return values (which are on
  subsequent lines).
{% endcomment %}

## {{row["name"]}} <small>{{type}}</small>
{% if defines.type == 'doc.class' %}
  {% comment %} Do nothing, classes just have their name as the view. {% endcomment %}
{% elsif row.type != 'variable' and defines.view %}
```lua
{{ defines.view | newline_to_br | strip_newlines | split: '<br />' | first }}
```
{% elsif defines.extends.view %}
```lua
{{ defines.extends.view | newline_to_br | strip_newlines | split: '<br />' | first }}
```
{% endif %}

{% if defines.file != empty %}
  \[[source](https://github.com/beyond-all-reason/recoil-lua-library/blob/main/library{{ defines.file }}#L{{ defines.start[0] | plus: 1 }}-L{{ defines.finish[0] | plus: 1 }})\]
{% endif %}

{% comment %}
  --- Description ---

  `defines.extends.rawdesc` seems to have the description without inlined doc
  tags. However it is not always available.
{% endcomment %}

{% if defines.extends %}
{{ defines.extends.rawdesc }}
{% else %}
{{ defines.desc }}
{% endif %}

{% if row.fields and row.fields != empty %}
### Fields


{% for field in row.fields %}
- <b>{{ field.name }}</b> `{{ field.extends.view }}` {% if field.rawdesc %} — {{field.rawdesc}} {% endif %}
{% endfor %}

{% endif %}


{% if defines["extends"] %}
{% assign extends = defines["extends"] %}

{% if extends["args"] and extends["args"] != empty %}
## Params


{% for arg in extends.args %}
  {% comment %} Support varargs syntax, which has type "..." and no name {% endcomment %}
  {% if arg.type == "..." %}
    {% assign name = arg.type %}
  {% else %}
    {% assign name = arg.name %}
  {% endif %}
1. <b>{{ name }}</b> `{{ arg.view }}` {% if arg.desc %} — {{ arg.desc }} {% endif %}
{% endfor %}

{% endif %}

{% if extends["returns"] and extends["returns"] != empty %}

### Returns

{% for return in extends["returns"] %}
1. `{{ return["view"] }}` <b>{{return["name"]}}</b> {% if return["rawdesc"] %} — {{return["rawdesc"]}} {% endif %}
{% endfor %}
{% endif %}

{% endif %}

{% endfor %}