---
layout: default
title: Configuration Variables
parent: Guides
permalink: /guides/configuration-variables/
---

# Configuration Variables

Recoil supports configuration variables that can be set during runtime
(with
[`Spring.{Set,Get}Config*`]({{ site.baseurl }}/ldoc/modules/UnsyncedCtrl.html#Engine_Config)
) or via `springsettings.cfg`.

Here we provide a list of them:

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Values</th>
  </tr>
  {% for row in site.data.configs %}
    {% assign row_data = row[1] %}
    <tr>
      <td>
        <a name="{{ row[0] }}" href="{{ site.gh_edit_repository }}/{{ site.gh_edit_view_mode }}/{{ site.gh_edit_branch }}/{{ row_data["declarationFile"]  | remove: "/spring/" }}#L{{ row_data["declarationLine"] }}">{{ row[0] }}</a>
        {% if row_data["deprecated"] %} <em>deprecated</em> {% endif %}
      </td>
      <td>
        <b>{{ row_data["type"] }}</b> {{ row_data["description"] }}
      </td>
      <td>
        {% if row_data["defaultValue"] %} Default: <code>{{ row_data["defaultValue"] }}</code> <br> {% endif %}
        {% if row_data["minimumValue"] %} Min: <code>{{ row_data["minimumValue"] }}</code> <br> {% endif %}
        {% if row_data["maximumValue"] %} Max: <code>{{ row_data["maximumValue"] }}</code> <br> {% endif %}
        {% if row_data["safemodeValue"] %} Safe Mode: <code>{{ row_data["safemodeValue"] }}</code> <br> {% endif %}
        {% if row_data["headlessValue"] %} Headless: <code>{{ row_data["safemodeValue"] }}</code> {% endif %}
      </td>
    </tr>
  {% endfor %}
</table>
