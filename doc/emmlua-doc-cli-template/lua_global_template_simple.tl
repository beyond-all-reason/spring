---
layout: default
title: {{ global_name }}
parent: Lua API
permalink: lua-api/globals/{{ global_name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# global {{ global_name }}

{% if display %}
{{ display }}
{% endif %}

{% if description %}
{{ description }}
{% endif %}

{{ "{% endraw %}" }}