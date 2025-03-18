---
layout: default
title: Weapon Defs
parent: Guides
permalink: /guides/weapon-defs/
---

# Weapon Defs

Here we provide a list of engine defined weapondefs:

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Values</th>
  </tr>
  {% for row in site.data.weapondefs["WeaponDefs"] %}
    {% assign row_data = row[1] %}
    <tr>
      <td id="{{ row[0] }}">
        <a href="#{{ row[0] }}">
          {{ row[0] }}
        </a>
        {% if row_data["deprecated"] %} <p class="label label-red">Deprecated</p> {% endif %}
      </td>
      <td>
        <b>{{ row_data["type"] | replace: "std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >", "string" | xml_escape | textilize }}</b> {{ row_data["description"] }}
      </td>
      <td>
        {% if row_data["fallbackName"] %} Fallback: <code>{{ row_data["fallbackName"] }}</code> <br> {% endif %}
        {% if row_data["defaultValue"] %} Default: <code>{{ row_data["defaultValue"] | join: ", " }}</code> <br> {% endif %}
        {% if row_data["minimumValue"] %} Min: <code>{{ row_data["minimumValue"] }}</code> <br> {% endif %}
        {% if row_data["maximumValue"] %} Max: <code>{{ row_data["maximumValue"] }}</code> <br> {% endif %}
        {% if row_data["scaleValue"] %} Scale Value: <code>{{ row_data["scaleValue"] }}</code> <br> {% endif %}
        {% if row_data["scaleValueString"] %} Scale String: <code>{{ row_data["scaleValueString"] }}</code> <br> {% endif %}
      </td>
    </tr>
  {% endfor %}
</table>
