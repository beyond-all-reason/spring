---
layout: default
title: Weapon Defs
parent: Guides
permalink: /guides/configuration-variables/
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
      <td>
        {{ row[0] }}
        {% if row_data["deprecated"] %} <em>deprecated</em> {% endif %}
      </td>
      <td>
        <b>{{ row_data["type"] | xml_escape | textilize }}</b> {{ row_data["description"] }}
      </td>
      <td>
        {% if row_data["fallbackName"] %} Fallback: {{ row_data["fallbackName"] }} <br> {% endif %}
        {% if row_data["defaultValue"] %} Default: {{ row_data["defaultValue"] }} <br> {% endif %}
        {% if row_data["minimumValue"] %} Min: {{ row_data["minimumValue"] }} <br> {% endif %}
        {% if row_data["maximumValue"] %} Max: {{ row_data["maximumValue"] }} <br> {% endif %}
        {% if row_data["scaleValue"] %} Scale Value: {{ row_data["scaleValue"] }} <br> {% endif %}
        {% if row_data["scaleValueString"] %} Scale String: {{ row_data["scaleValueString"] }} <br> {% endif %}
      </td>
    </tr>
  {% endfor %}
</table>
