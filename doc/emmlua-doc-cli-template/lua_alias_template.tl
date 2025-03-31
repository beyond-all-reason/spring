---
layout: default
title: {{ type_name }}
parent: Lua API
permalink: lua-api/types/{{ type_name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# alias {{ type_name }}
---
{% if namespace %}
- namespace: {{ namespace }}
{% endif %}

{% if origin_type %}
{{ origin_type }}
{% endif %}

{% if description %}
{{ description }}
{% endif %}

{{ "{% endraw %}" }}