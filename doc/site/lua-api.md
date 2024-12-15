---
layout: default
title: Lua API
nav_order: 5
has_children: true
has_toc: false
permalink: lua-api
---

# Lua API

{% comment %}
## Table of Contents

<ul>
{% for row in site.data.doc %}
<li>
{{ row["name"] }} <br>
<p>{{ row["defines"][0]["desc"] }}</p>
</li>
{% endfor %}
</ul>
{% endcomment %}

---

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
{% if defines.extends.view %}
```lua
{{ defines.extends.view | newline_to_br | strip_newlines | split: '<br />' | first }}
```
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
1. <b>{{ arg.name }}</b> `{{ arg.view }}` {% if arg.desc %} — {{arg.desc}} {% endif %}
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