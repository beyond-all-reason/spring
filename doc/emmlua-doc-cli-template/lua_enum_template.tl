---
layout: default
title: {{ type_name }}
parent: Lua API
permalink: lua-api/types/{{ type_name }}
---

# enum {{ type_name }}
---
{% if namespace %}
- namespace: {{ namespace }}
{% endif %}

{% if description %}
{{ description }}
{% endif %}

```cpp
enum {{ type_name }} {
    {% for field in fields -%}
        {{ field.name }} = {{ field.value }}, {{ field.description }}
    {% endfor %}
}
```
