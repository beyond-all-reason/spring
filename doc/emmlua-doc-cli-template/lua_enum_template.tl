---
layout: default
title: {{ type_name }}
parent: Lua API
permalink: lua-api/types/{{ type_name }}
---
{# Use `render_with_liquid: false` when we upgrade to Jekyl >=4.0 #}
{{ "{% raw %}" }}

# enum {{ type_name }}
---
{% if namespace %}
- namespace: {{ namespace }}
{% endif %}

{% if description %}
{{ description }}
{% endif %}

{% for field in fields -%}

### {{ field.name }}

```lua
{{ type_name }}.{{ field.name }} = {{ field.value }}
```

{{ field.description }}

{% endfor %}

{{ "{% endraw %}" }}
