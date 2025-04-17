---
layout: default
title: Unsynced Commands
parent: Guides
permalink: /guides/unsynced-commands/
---

# Unsynced Commands

Game developers and players can issue unsynced commands from chat, e.g.
("/showmetalmap") or `Spring.SendCommands`.

Here we provide a list of them, some commands require cheats enabled ("/cheat"):

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Arguments</th>
  </tr>
  {% for row in site.data.unsynced_commands %}
    {% assign row_data = row[1] %}
    {% assign args = row_data["arguments"] %}
    <tr>
      <td id="{{ row[0] }}">
        <a href="#{{ row[0] }}">
          {{ row[0] | xml_escape | textilize }}
          {% if row_data["cheatRequired"] %} <p class="label label-yellow">Cheat</p> {% endif %}
        </a>
      </td>
      <td>
        {{ row_data["description"] | xml_escape | textilize }}
      </td>
      <td>
        <dl class="dl-auto">
        {% for arg in args %}
          <dt>
            <code>{{ arg[0] | xml_escape | textilize }}</code>
          </dt>
          <dd>
            {{ arg[1] | xml_escape | textilize }}
          </dd>
        {% endfor %}
        </dl>
      </td>
    </tr>
  {% endfor %}
</table>
