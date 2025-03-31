---
layout: default
title: Lua API
nav_order: 5
has_children: true
has_toc: false
permalink: lua-api
render_with_liquid: false
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# index

{% if types %}
## Types
{% for item in types %}
- [{{ item.name }}]({{ item.file }}){% endfor %}
{% endif %}

{% if modules %}
## Modules
{% for item in modules -%}
- [{{ item.name }}]({{ item.file }})
{% endfor %}
{% endif %}

{% if globals %}
## Globals
{% for item in globals -%}
- [{{ item.name }}]({{ item.file }})
{% endfor %}
{% endif %}

{{ "{% endraw %}" }}