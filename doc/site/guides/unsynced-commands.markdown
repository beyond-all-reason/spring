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
  </tr>
  {% for row in site.data.unsynced_commands %}
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
