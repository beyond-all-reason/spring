---
layout: default
title: Synced Commands
parent: Guides
permalink: /guides/synced-commands/
---

# Synced Commands

Game developers and players can issue synced commands from chat, e.g.
("/give unit"), or `Spring.SendCommands`:

Here we provide a list of them, some commands require cheats enabled ("/cheat"):

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
  </tr>
  {% for row in site.data.synced_commands %}
    {% assign row_data = row[1] %}
    <tr>
      <td id="{{ row[0] }}">
        <a href="#{{ row[0] }}">
          {{ row[0] }}
          {% if row_data["cheatRequired"] %} <p class="label label-yellow">Cheat</p> {% endif %}
        </a>
      </td>
      <td>
        <b>{{ row_data["type"] | replace: "std::", "" }}</b> {{ row_data["description"] }}
      </td>
    </tr>
  {% endfor %}
</table>
