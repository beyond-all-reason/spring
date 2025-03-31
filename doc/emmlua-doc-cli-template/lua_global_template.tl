---
layout: default
title: {{ global_name }}
parent: Lua API
permalink: lua-api/globals/{{ global_name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# global {{ global_name }}

{% if description %}
{{ description }}
{% endif %}

{% if methods %}
## methods

{% for method in methods %}
### {{ method.name }}

{{ method.display }}

{% if method.description %}
{{ method.description }}
{% endif %}
{% endfor %}
{% endif %}

{% if fields %}
## fields

{% for field in fields %}
### {{ field.name }}

{{ field.display }}

{% if field.description %}
{{ field.description }}
{% endif %}
{% endfor %}
{% endif %}

{{ "{% endraw %}" }}